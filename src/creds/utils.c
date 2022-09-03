#include "utils.h"

const char *flash_cred_status_to_str(flash_cred_status_t status)
{
	switch (status) {
	case FLASH_CRED_VALID:
		return "valid";
	case FLASH_CRED_UNALLOCATED:
		return "unallocated";
	case FLASH_CRED_SIZE_BLANK:
		return "size blank";
	case FLASH_CRED_SIZE_INVALID:
		return "size invalid";
	case FLASH_CRED_CRC_MISMATCH:
		return "crc mismatch";
	case FLASH_CRED_REVOKED:
		return "revoked";
	case FLASH_CRED_NULL:
		return "null";
	default:
		return "<unknown status>";
	}
}

const char *cred_id_to_str(cred_id_t id)
{
	switch (id) {
	case CRED_HTTPS_SERVER_PRIVATE_KEY:
		return "HTTPS Server Private Key";
	case CRED_HTTPS_SERVER_CERTIFICATE:
		return "HTTPS Server Certificate";
	case CRED_HTTPS_CLIENT_CA:
		return "HTTPS Client CA";
	case CRED_AWS_PRIVATE_KEY:
		return "AWS Private Key";
	case CRED_AWS_CERTIFICATE:
		return "AWS Certificate";
	case CRED_AWS_PRIVATE_KEY_DER:
		return "AWS Private Key DER";
	case CRED_AWS_CERTIFICATE_DER:
		return "AWS Certificate DER";
	case CRED_AWS_ROOT_CA1:
		return "AWS Root CA1";
	case CRED_AWS_ROOT_CA3:
		return "AWS Root CA3";
	default:
		return "<unknown id>";
	}
}

const char *cred_format_to_str(cred_format_t format)
{
	switch (format) {
	case CRED_FORMAT_PEM:
		return "pem";
	case CRED_FORMAT_DER:
		return "der";
	case CRED_FORMAT_UNKOWN:
	default:
		return "unknown";
	}
}