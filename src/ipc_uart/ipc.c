#include "ipc.h"

#include <kernel.h>

#include <device.h>
#include <devicetree.h>
#include <drivers/uart.h>

#include "../drivers/crc/stm32_crc32.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(ipc, LOG_LEVEL_INF);

// TODO monitor usage using CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION

// config
#define IPC_UART_RX_TIMEOUT_MS 50U
#define CONFIG_IPC_MEMSLAB_COUNT 2

// drivers
#define IPC_UART_NODE DT_ALIAS(ipc_uart)
#define CRC_NODE DT_NODELABEL(crc1)

static const struct device *uart_dev = DEVICE_DT_GET(IPC_UART_NODE);
static const struct device *crc_dev = DEVICE_DT_GET(CRC_NODE);

static uint8_t double_buffer[2][IPC_FRAME_SIZE];
static uint8_t *next_buffer = double_buffer[1];

typedef enum {
	/**
	 * @brief Waiting for the first byte of the start delimiter
	 */
	PARSING_CATCH_FRAME = 0, 

	/**
	 * @brief Parsing the start sfd
	 */
	PARSING_FRAME_START_DELIMITER,

	/**
	 * @brief Parsing the data,
	 * waiting for the first byte of the end delimiter
	 */
	PARSING_FRAME_DATA,
	
	/**
	 * @brief Parsing the efd
	 */
	PARSING_FRAME_END_DELIMITER
} parsing_state_t;

// internal : interrupt, thread
static struct {
	parsing_state_t state;
	size_t state_rem_bytes;
	union {
		ipc_frame_t *frame;
		uint8_t *raw;
	};
	size_t filling;
} parsing_ctx = {
	.state = PARSING_CATCH_FRAME,
	.state_rem_bytes = 0U,
	.filling = 0U
};

#define IPC_MEMSLAB_COUNT (CONFIG_IPC_MEMSLAB_COUNT)

static K_MEM_SLAB_DEFINE(frames_slab, IPC_FRAME_SIZE, IPC_MEMSLAB_COUNT, 4);
static K_FIFO_DEFINE(rx_fifo);

static inline int alloc_frame_buf(ipc_frame_t **p_frame) {
	return k_mem_slab_alloc(&frames_slab, (void **) p_frame, K_NO_WAIT);
}

static inline void free_frame_buf(ipc_frame_t **p_frame) {
	if (*p_frame != NULL) {
		k_mem_slab_free(&frames_slab, (void **) p_frame);
		p_frame = NULL;
	}
}

// application
static struct k_msgq *application_msgq = NULL;

/*___________________________________________________________________________*/

#define IPC_STATE_INITIALIZED_BIT 0U
#define IPC_STATE_INITIALIZED_MASK BIT(IPC_STATE_INITIALIZED_BIT)

ATOMIC_DEFINE(ipc_state, 1U);

static bool ipc_is_initialized(void)
{
	return (atomic_get(ipc_state) & IPC_STATE_INITIALIZED_MASK) != 0;
}

int ipc_attach_rx_msgq(struct k_msgq *msgq)
{
	if (ipc_is_initialized() == true) {
		LOG_ERR("IPC already initialized %d", 0);
		return -EINVAL;
	}

	application_msgq = msgq;

	return 0;
}

/*___________________________________________________________________________*/

// convert enum parsing_state to string
static const char *parsing_state_to_str(parsing_state_t state)
{
	switch (state) {
	case PARSING_CATCH_FRAME:
		return "PARSING_CATCH_FRAME";
	case PARSING_FRAME_START_DELIMITER:
		return "PARSING_FRAME_START_DELIMITER";
	case PARSING_FRAME_DATA:
		return "PARSING_FRAME_DATA";
	case PARSING_FRAME_END_DELIMITER:
		return "PARSING_FRAME_END_DELIMITER";
	default:
		return "<UNKNOWN>";
	}
}

// convert uart_event to string
static const char *uart_event_to_str(enum uart_event_type type)
{
	switch (type) {
	case UART_TX_DONE:
		return "UART_TX_DONE";
	case UART_TX_ABORTED:
		return "UART_TX_ABORTED";
	case UART_RX_RDY:
		return "UART_RX_RDY";
	case UART_RX_BUF_REQUEST:
		return "UART_RX_BUF_REQUEST";
	case UART_RX_BUF_RELEASED:
		return "UART_RX_BUF_RELEASED";
	case UART_RX_DISABLED:
		return "UART_RX_DISABLED";
	case UART_RX_STOPPED:
		return "UART_RX_STOPPED";
	default:
		return "<UNKNOWN>";
	}
}

/**
 * @brief Copy data from the buffer to the context frame
 * 
 * @param data 
 * @param len 
 */
static inline void copy(uint8_t *data, size_t len)
{
	memcpy(parsing_ctx.raw + parsing_ctx.filling, data, len);
	parsing_ctx.filling += len;
}

static inline void copy_byte(const char chr)
{
	parsing_ctx.raw[parsing_ctx.filling] = chr;
	parsing_ctx.filling++;
}

static void reset_parsing_ctx(void)
{
	parsing_ctx.state = PARSING_CATCH_FRAME;
	parsing_ctx.filling = 0U;
}

static void handle_received_chunk(const uint8_t *data, size_t size)
{
	int ret;

	while (size > 0) {
		LOG_DBG("%s", parsing_state_to_str(parsing_ctx.state));
		
		switch (parsing_ctx.state) {
		case PARSING_CATCH_FRAME:
		{
			/* looking for the first byte of the start frame delimiter */
			uint8_t *const p = (uint8_t *)
				memchr(data, IPC_START_FRAME_DELIMITER_BYTE, size);
			if (p != NULL) {
				/* if start frame delimiter is not at the very
				 * beginning of the chunk, we discard first bytes
				 */
				if (p != data) {
					LOG_WRN("Discarded %u B",
						(size_t)(p - data));

					/* adjust frame beginning */
					size -= (p - data);
					data = p;
				}

				/* try allocate a frame buffer */
				ret = alloc_frame_buf(&parsing_ctx.frame);
				if (ret != 0) {
					LOG_ERR("Failed to allocate buf %d", ret);
					goto discard;
				}

				/* prepare context for the next state */
				parsing_ctx.state_rem_bytes =
					IPC_START_FRAME_DELIMITER_SIZE - 1U;
				parsing_ctx.state = PARSING_FRAME_START_DELIMITER;
				copy_byte(*data);
				data++;
				size--;
			} else {
				/* not found */
				goto discard;
			}
			break;
		}
		case PARSING_FRAME_START_DELIMITER:
		{
			while (parsing_ctx.state_rem_bytes > 0U && size > 0U) {
				if (data[0] == IPC_START_FRAME_DELIMITER_BYTE) {
					copy_byte(*data);
					parsing_ctx.state_rem_bytes--;
					data++;
					size--;

				} else {
					goto discard;
				}
			}

			if (parsing_ctx.state_rem_bytes == 0U) {
				parsing_ctx.state = PARSING_FRAME_DATA;
				parsing_ctx.state_rem_bytes = IPC_FRAME_SIZE
					- IPC_START_FRAME_DELIMITER_SIZE
					- IPC_END_FRAME_DELIMITER_SIZE;
			}
			break;
		}
		case PARSING_FRAME_DATA:
		{
			while (parsing_ctx.state_rem_bytes > 0U && size > 0U) {
				parsing_ctx.state_rem_bytes--;
				copy_byte(*data);
				data++;
				size--;
			}

			if (parsing_ctx.state_rem_bytes == 0U) {
				parsing_ctx.state = PARSING_FRAME_END_DELIMITER;
				parsing_ctx.state_rem_bytes = IPC_END_FRAME_DELIMITER_SIZE;
			}
			break;
		}
		case PARSING_FRAME_END_DELIMITER:
		{
			while (parsing_ctx.state_rem_bytes > 0U && size > 0U) {
				if (data[0] == IPC_END_FRAME_DELIMITER_BYTE) {
					parsing_ctx.state_rem_bytes--;
					copy_byte(*data);
					data++;
					size--;
				} else {
					/* not found */
					goto discard;
				}
			}

			if (parsing_ctx.state_rem_bytes == 0U) {
				__ASSERT(parsing_ctx.filling == IPC_FRAME_SIZE,
					 "Invalid frame size");

				/* Pass the detected frame to the processing thread by
				* the FIFO
				* Note: As queuing a FIFO requires to write an address
				*  at the beginning of the queued data, the start
				*  frame delimiter will be overwritten.
				*  However we don't need it anymore as it has been already
				*  checked above.
				*/
				k_fifo_put(&rx_fifo, parsing_ctx.frame);

				/* reset context for next frame */
				reset_parsing_ctx();
			}
			break;
		}
		}
	}
	return;

discard:
	LOG_WRN("\t %s : Discarding %u B from state ",
		log_strdup(parsing_state_to_str(parsing_ctx.state)), size);

	free_frame_buf(&parsing_ctx.frame);
	reset_parsing_ctx();
	return;
}

#if defined(CONFIG_IPC_TEST_LOOPBACK)
static K_SEM_DEFINE(ipc_tx_sem, 1, 1);

static inline void ipc_tx_done(void)
{
	k_sem_give(&ipc_tx_sem);
}
#else 
static inline void ipc_tx_done(void) { }
#endif 

static void uart_callback(const struct device *dev,
			  struct uart_event *evt,
			  void *user_data)
{
	LOG_DBG("%s", uart_event_to_str(evt->type));

	switch (evt->type) {
	case UART_TX_DONE:
		ipc_tx_done();
		break;
	case UART_TX_ABORTED:
		break;
	case UART_RX_RDY:
	{
		LOG_DBG("buf %x + %x : %u", (uint32_t) evt->data.rx.buf,
			evt->data.rx.offset, evt->data.rx.len);
		LOG_HEXDUMP_DBG(evt->data.rx.buf + evt->data.rx.offset,
				evt->data.rx.len, "RAW");

		handle_received_chunk(evt->data.rx.buf + evt->data.rx.offset,
				      evt->data.rx.len);
		break;
	}
	case UART_RX_BUF_REQUEST:
	{
		uart_rx_buf_rsp(dev, next_buffer, IPC_FRAME_SIZE);
		break;
	}
	case UART_RX_BUF_RELEASED:
	{
		next_buffer = evt->data.rx_buf.buf;
		break;
	}
	case UART_RX_DISABLED:
	{
		LOG_WRN("RX disabled %d", 0);
		break;
	}
	case UART_RX_STOPPED:
	{
		LOG_WRN("RX stopped %d", evt->data.rx_stop.reason);
		break;
	}
	}
}

static int ipc_initialize(void)
{
	int ret;

	if (device_is_ready(crc_dev) == false) {
		LOG_ERR("CRC device not ready = %d", 0);
		return -1;
	}

	if (device_is_ready(uart_dev) == false) {
		LOG_ERR("IPC UART device not ready = %d", 0);
		return -1;
	}

	atomic_set_bit(ipc_state, IPC_STATE_INITIALIZED_BIT);

	ret = uart_callback_set(uart_dev, uart_callback, NULL);
	if (ret != 0) {
		LOG_ERR("uart_callback_set() failed %d", ret);
		return -1;
	}

	ret = uart_rx_enable(uart_dev,
			     double_buffer[0],
			     IPC_FRAME_SIZE,
			     IPC_UART_RX_TIMEOUT_MS);
	if (ret != 0) {
		LOG_ERR("uart_rx_enable() failed %d", ret);
		return -1;
	}

	return 0;
}

/*___________________________________________________________________________*/

// thread
static void ipc_thread(void *_a, void *_b, void *_c);

K_THREAD_DEFINE(ipc_thread_id, 0x400, ipc_thread, NULL, NULL, NULL, K_PRIO_PREEMPT(8), 0, 0);

static inline uint32_t ipc_frame_crc32(ipc_frame_t *frame)
{
	uint32_t *const start = (uint32_t *)&frame->seq;
	size_t len = sizeof(frame->data) + sizeof(frame->seq); // multiple of 4

	return crc_calculate(crc_dev, start, len >> 4);
}

static bool verify_frame_crc32(ipc_frame_t *frame)
{
	const uint32_t crc32 = ipc_frame_crc32(frame);
	const bool result = crc32 == frame->crc32;

	if (result == false) {
		LOG_ERR("CRC32 mismatch: %x != %x", crc32, frame->crc32);
	}

	return result;
}

static void ipc_log_frame(const ipc_frame_t *frame)
{
	LOG_INF("IPC frame: %u B, seq = %x, data size = %u, sfd = %x, efd = %x crc32=%x",
		IPC_FRAME_SIZE, frame->seq, frame->data.size, frame->start_delimiter,
		frame->end_delimiter, frame->crc32);
	LOG_HEXDUMP_DBG(frame->data.buf, frame->data.size, "IPC frame data");
}

static void ipc_thread(void *_a, void *_b, void *_c)
{
	ipc_frame_t *frame;
	uint32_t last_seq = 0U;

	ipc_initialize();

	for (;;) {
		frame = (ipc_frame_t *) k_fifo_get(&rx_fifo, K_FOREVER);

		ipc_log_frame(frame);

		if (frame->seq > last_seq + 1) {
			LOG_WRN("Seq gap %u -> %u, %u frames lost", last_seq,
				frame->seq, frame->seq - last_seq - 1);
		} else if (frame->seq < last_seq) {
			LOG_WRN("Seq rollback to %u %u, peer probably reseted",
				last_seq, frame->seq);
		}

		last_seq = frame->seq;

		/* verify frame */
		verify_frame_crc32(frame);

		/* dispatch frame */
		if (application_msgq != NULL) {
			k_msgq_put(application_msgq, frame, K_NO_WAIT);
		}

		free_frame_buf(&frame);
	}	
}

/*___________________________________________________________________________*/

#if defined(CONFIG_IPC_TEST_LOOPBACK)

// thread
static void tx_thread(void *_a, void *_b, void *_c);

K_THREAD_DEFINE(tx_thread_id, 0x400, tx_thread, NULL, NULL, NULL, K_PRIO_PREEMPT(8), 0, 0);

static void build_tx_frame(ipc_frame_t *frame)
{
	frame->start_delimiter = IPC_START_FRAME_DELIMITER;
	frame->seq = 0U;
	frame->data.size = IPC_MAX_DATA_SIZE;
	frame->crc32 = 0xBBBBBBBBLU;
	frame->end_delimiter = IPC_END_FRAME_DELIMITER;

	for (uint32_t i = 0; i < IPC_MAX_DATA_SIZE; i++) {
		frame->data.buf[i] = i;
	}
}

static void tx_thread(void *_a, void *_b, void *_c)
{
	static ipc_frame_t frame;

	build_tx_frame(&frame);

	k_sleep(K_SECONDS(2));

	for (;;) {
		k_sem_take(&ipc_tx_sem, K_FOREVER);

		frame.seq++;

		uart_tx(uart_dev, (const uint8_t *) &frame, IPC_FRAME_SIZE, 0);

		k_sleep(K_SECONDS(5));
	}	
}

#endif