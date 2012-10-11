#ifndef PANC_H
#define PANC_H
#include <stdint.h>
#include <config.h>

typedef uint8_t PANCHANDLE;
typedef enum {
	PANCSTATUS_OK,
	PANCSTATUS_ERR,
} PANCSTATUS;

/* Placed here to avoid circular include */
#include <port.h>

enum pancake_header_compression {
	PANC_COMPRESSION_NONE,
	PANC_COMPRESSION_HC1,
	PANC_COMPRESSION_HCIP,
};

enum pancake_security {
	PANC_SECURITY_NONE,
	PANC_SECURITY_AESCCM128,
	PANC_SECURITY_AESCCM64,
	PANC_SECURITY_AESCCM32,
};

struct pancake_options_cfg {
	enum pancake_header_compression compression;
	enum pancake_security security;
};

PANCSTATUS pancake_init(PANCHANDLE *handle, struct pancake_options_cfg *options_cfg, struct pancake_dev_cfg *dev_cfg, void *dev_data);
PANCSTATUS pancake_write_test(PANCHANDLE handle);
#endif
