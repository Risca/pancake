#ifndef PANC_H
#define PANC_H
#include <stdint.h>
#include <config.h>
#include <netinet/ip6.h>

typedef int8_t PANCHANDLE;
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

typedef void (*read_callback_func)(struct ip6_hdr *hdr, uint8_t *payload, uint16_t payload_length);

PANCSTATUS pancake_init(PANCHANDLE *handle, struct pancake_options_cfg *options_cfg, struct pancake_dev_cfg *dev_cfg, void *dev_data, read_callback_func read_callback);
PANCSTATUS pancake_write_test(PANCHANDLE handle);
void pancake_destroy(PANCHANDLE handle);

/* For upper layers */
PANCSTATUS pancake_send(PANCHANDLE handle, struct ip6_hdr *hdr, uint8_t *payload, uint16_t payload_length);
PANCSTATUS pancake_send_packet(PANCHANDLE handle, uint8_t *ip6_packet, uint16_t packet_length);

/* For lower layers */
PANCSTATUS pancake_process_data(void *dev_data, uint8_t *data, uint16_t size);
#endif
