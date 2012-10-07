#ifndef FOO_H
#define FOO_H
#include <stdint.h>
#include <config.h>

typedef uint8_t FOOHANDLE;
typedef enum {
	FOOSTATUS_OK,
	FOOSTATUS_ERR,
} FOOSTATUS;

/* Placed here to avoid circular include */
#include <port.h>

enum foo_header_compression {
	FOO_COMPRESSION_NONE,
	FOO_COMPRESSION_HC1,
	FOO_COMPRESSION_HCIP,
};

enum foo_security {
	FOO_SECURITY_NONE,
	FOO_SECURITY_AESCCM128,
	FOO_SECURITY_AESCCM64,
	FOO_SECURITY_AESCCM32,
};

struct foo_opts_cfg {
	enum foo_header_compression compression;
	enum foo_security security;
};

FOOSTATUS foo_init(FOOHANDLE *handle, struct foo_opts_cfg *opts_cfg, struct foo_dev_cfg *dev_cfg, void *dev_data);
FOOSTATUS foo_write_test(FOOHANDLE handle);
#endif
