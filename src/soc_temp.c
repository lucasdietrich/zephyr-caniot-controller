#include "soc_temp.h"

#include <device.h>
#include <devicetree.h>
#include <drivers/adc.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(soc_temp, LOG_LEVEL_DBG);

#define ADC_NODE DT_NODELABEL(adc1)

#define ADC_TEMP_CHANNEL 18
#define ADC_RESOLUTION 12
#define ADC_GAIN ADC_GAIN_1

static struct adc_channel_cfg channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REF_INTERNAL,
	.acquisition_time = ADC_ACQ_TIME_DEFAULT,
	.channel_id = ADC_TEMP_CHANNEL,
	.differential = 0
};

static int16_t sample_buffer[1];

static struct adc_sequence sequence = {
	.channels    = 0,
	.buffer      = sample_buffer,
	.buffer_size = sizeof(sample_buffer),
	.resolution  = ADC_RESOLUTION,
};

int soc_temp_read(void)
{
	int ret;

	const struct device *adc = DEVICE_DT_GET(ADC_NODE);

	if (device_is_ready(adc) == false) {
		LOG_ERR("ADC (%p) device not ready", adc);
		return -EIO;
	}

	ret = adc_channel_setup(adc, &channel_cfg);
	if (ret != 0) {
		LOG_ERR("Failed to setup ADC channel (%d)", ret);
		return ret;
	}

	SET_BIT(sequence.channels, ADC_TEMP_CHANNEL);

	uint16_t adc_vref = adc_ref_internal(adc);
	LOG_DBG("ADC VREF: %hu", adc_vref);


	ret = adc_read(adc, &sequence);
	if (ret != 0) {
		LOG_ERR("Failed to read ADC channel (%d)", ret);
		return ret;
	}

	int32_t mv_value = sample_buffer[0];

	ret = adc_raw_to_millivolts(adc_vref, ADC_GAIN,
				    ADC_RESOLUTION, &mv_value);
	if (ret != 0) {
		LOG_ERR("Failed to convert ADC value (%d)", ret);
		return ret;
	}

	LOG_INF("ADC value: %d", mv_value);

	return 0;
}