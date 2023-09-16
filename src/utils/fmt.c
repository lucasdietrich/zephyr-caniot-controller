#include "fmt.h"

#include <zephyr/kernel.h>

static const char *fmt_unit_to_str(fmt_unit_t unit)
{
	const char *unit_strs[] = {
		[FMT_UNIT_NONE] = "",		  [FMT_UNIT_HZ] = "Hz",
		[FMT_UNIT_KHZ] = "kHz",		  [FMT_UNIT_MHZ] = "MHz",
		[FMT_UNIT_GHZ] = "GHz",		  [FMT_UNIT_PERCENT] = "%",
		[FMT_UNIT_CELSIUS] = "C",	  [FMT_UNIT_MILLIAMPS] = "mA",
		[FMT_UNIT_MILLIWATTS] = "mW", [FMT_UNIT_MILLIVOLTS] = "mV",
		[FMT_UNIT_DBM] = "dBm",
	};

	if (unit >= ARRAY_SIZE(unit_strs)) {
		unit = FMT_UNIT_NONE;
	}

	return unit_strs[unit];
}

ssize_t
fmt_encode_value(char *buf, size_t buf_size, fmt_val_t *value, const fmt_enc_cfg_t *cfg)
{
	ssize_t written = 0;
	ssize_t ret		= -EINVAL;

	switch (cfg->type) {
	case FMT_ENC_INT32:
		ret = snprintf(buf, buf_size, "%d", value->svalue);
		break;
	case FMT_ENC_UINT32:
		ret = snprintf(buf, buf_size, "%u", value->uvalue);
		break;
	case FMT_ENC_FLOAT_DIGITS:
		ret = snprintf(buf, buf_size, "%.*f", (int)cfg->digits, value->fvalue);
		break;
	case FMT_ENC_EXP:
		ret = snprintf(buf, buf_size, "%e", value->fvalue);
		break;
	case FMT_ENC_EXP_DIGITS:
		ret = snprintf(buf, buf_size, "%.*e", (int)cfg->digits, value->fvalue);
		break;
	case FMT_ENC_FLOAT:
	default:
		ret = snprintf(buf, buf_size, "%f", value->fvalue);
		break;
	}

	if (ret > 0) {
		if ((size_t)ret < buf_size) {
			written = ret;

			/* Append unit */
			const char *unit_str = fmt_unit_to_str(cfg->unit);
			if (unit_str[0] != '\0') {
				ret = snprintf(buf + written, buf_size - written, " %s", unit_str);

				if (ret > 0) {
					if ((size_t)ret < buf_size - written) {
						ret = written + ret;
					} else {
						ret = -ENOMEM;
					}
				}
			}
		} else {
			ret = -ENOMEM;
		}
	}

	return ret;
}