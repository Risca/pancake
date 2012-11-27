#ifndef PANC_H
#define PANC_H
#include <config.h>
#include <stdint.h>
#include <netinet/ip6.h>

#define PANCAKE_MAX_HDR_SIZE 25
#define IPv6_MTU 1280
#define aMaxPHYPacketSize 127
#define aMaxFrameOverhead 25
#define aMaxMPDUUnsecuredOverhead aMaxFrameOverhead

typedef int8_t PANCHANDLE;
typedef enum {
	PANCSTATUS_OK,
	PANCSTATUS_ERR,
	PANCSTATUS_NOMEM,
	PANCSTATUS_NOTREADY,
} PANCSTATUS;

/* Placed here to avoid circular include */
#include <port.h>

enum pancake_header_compression {
	PANC_COMPRESSION_NONE = 0,
	PANC_COMPRESSION_HC1,
	PANC_COMPRESSION_IPHC,
};

enum pancake_security {
	PANC_SECURITY_NONE = 0,
	PANC_SECURITY_AESCCM32,
	PANC_SECURITY_AESCCM64,
	PANC_SECURITY_AESCCM128,
};

struct pancake_options_cfg {
	enum pancake_header_compression compression;
	enum pancake_security security;
};

enum pancake_event_type {
	PANC_EVENT_CONNECTION_UPDATE,
	PANC_EVENT_DATA_RECEIVED,
};

enum pancake_event_con_update_status {
  	PANC_CONNECTED,
	PANC_DISCONNECTED,
};

struct pancake_event_con_update {
	enum pancake_event_type type;
	
	enum pancake_event_con_update_status status;
	uint8_t is_device;
	uint8_t is_coordinator;
};

struct pancake_event_data_received {
	enum pancake_event_type type;
	
	struct ip6_hdr *hdr;
	uint8_t *payload;
	uint16_t payload_length;
};

typedef union {
  	enum pancake_event_type type;
	struct pancake_event_con_update connection_update;
	struct pancake_event_data_received data_received;
} pancake_event;
  


typedef void (*event_callback_func)(pancake_event *event);

PANCSTATUS pancake_init(PANCHANDLE *handle, struct pancake_options_cfg *options_cfg, struct pancake_port_cfg *port_cfg, void *dev_data, event_callback_func event_callback);
PANCSTATUS pancake_write_test(PANCHANDLE handle);

void pancake_destroy(PANCHANDLE handle);
#if PANC_TESTS_ENABLED != 0
PANCSTATUS pancake_write_test(PANCHANDLE handle);
PANCSTATUS pancake_reassembly_test(PANCHANDLE handle);
#endif

/* For upper layers */
PANCSTATUS pancake_send(PANCHANDLE handle, struct ip6_hdr *hdr, uint8_t *payload, uint16_t payload_length);
PANCSTATUS pancake_send_packet(PANCHANDLE handle, uint8_t *ip6_packet, uint16_t packet_length);

/* For lower layers */
PANCSTATUS pancake_process_data(void *dev_data, struct pancake_ieee_addr *src, struct pancake_ieee_addr *dst, uint8_t *data, uint16_t size);
PANCSTATUS pancake_connection_update(void *dev_data, struct pancake_event_con_update *connection_update );

/* Header compression */
struct pancake_compressed_ip6_hdr {
	uint8_t *hdr_data;
	uint16_t size;
	uint8_t dispatch_value;
};

PANCSTATUS pancake_compress_header(struct ip6_hdr *hdr, struct pancake_compressed_ip6_hdr *compressed_hdr);

PANCSTATUS pancake_decompress_header(struct pancake_compressed_ip6_hdr *compressed_hdr, struct ip6_hdr *hdr, uint16_t payload_length);
PANCSTATUS pancake_diff_header(struct ip6_hdr *origin_hdr, struct ip6_hdr *decompressed_hdr);

/* Internal */
PANCHANDLE pancake_handle_from_dev_data(void *dev_data);

#endif
