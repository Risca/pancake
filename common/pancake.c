#include <pancake.h>
#include <stddef.h>
#include <string.h>

#if PANC_HAVE_PRINTF != 0
#include <stdio.h>
#endif

#ifdef _WIN32
    #include <windows.h>
#endif

/* Updated to reflect RFC6282 */
#define DISPATCH_NALP        0x00
#define DISPATCH_ESC         0x40
#define DISPATCH_IPv6        0x41
#define DISPATCH_HC1         0x42
#define DISPATCH_BC0         0x50
#define DISPATCH_IPHC        0x7F
#define DISPATCH_MESH        0x80
#define DISPATCH_FRAG1       0xC0
#define DISPATCH_FRAGN       0xE0

/* None, 32 bit, 64 bit, 128 bit */
uint16_t security_overhead[] = 
	{0, 9, 13, 21};

struct pancake_main_dev {
	struct pancake_port_cfg		*cfg;
	struct pancake_options_cfg	*options;
	void				*dev_data;
	read_callback_func		read_callback;
};
static struct pancake_main_dev devs[PANC_MAX_DEVICES];

#if PANC_HAVE_PRINTF != 0
#define pancake_printf(...) printf(__VA_ARGS__)
#else
#define pancake_printf(...) {}
#endif

static void print_pancake_error(char *source, PANCSTATUS ret)
{
	switch (ret) {
	case PANCSTATUS_ERR:
		pancake_printf("%s: Undefined error\n", source);
		break;
	case PANCSTATUS_NOMEM:
		pancake_printf("%s: Not enough memory!\n", source);
		break;
	case PANCSTATUS_NOTREADY:
		pancake_printf("%s: Not ready\n", source);
		break;
	}
}

static uint16_t calculate_frame_overhead(struct pancake_main_dev *dev, struct pancake_compressed_ip6_hdr *hdr)
{
	uint16_t overhead = 0;
	struct pancake_options_cfg *options = dev->options;

	/* Overhead due to security */
	overhead += security_overhead[options->security];

	/* Compression overhead (+1 dispatch byte) */
	overhead += hdr->size + 1;

	return overhead;
}

struct pancake_frag_hdr {
	uint16_t size;
	uint16_t tag;
	uint8_t offset;
};

#include "pancake_internals/fragmentation.c"
#include "pancake_internals/reassembly.c"
#include "pancake_internals/header_compression.c"

PANCSTATUS pancake_init(PANCHANDLE *handle, struct pancake_options_cfg *options_cfg, struct pancake_port_cfg *port_cfg, void *dev_data, read_callback_func read_callback)
{
	int8_t ret;
	uint8_t i;
	static uint8_t handle_count = 0;
	struct pancake_main_dev *dev = &devs[handle_count];

	/* Sanity check */
	if (handle == NULL || options_cfg == NULL || port_cfg == NULL || handle_count+1 > PANC_MAX_DEVICES) {
		goto err_out;
	}
	if (port_cfg->write_func == NULL) {
		goto err_out;
	}

	/* Mark reassembly buffers as free */
	for (i = 0; i < PANC_MAX_CONCURRENT_REASSEMBLIES; i++) {
		ra_bufs[i].free = 1;
	}

	/* Initialize port */
	if (port_cfg->init_func) {
		ret = port_cfg->init_func(dev_data);
		if (ret != PANCSTATUS_OK) {
			goto err_out;
		}
	}

	/* Save config and device data, and update handle */
	dev->cfg = port_cfg;
	dev->options = options_cfg;
	dev->dev_data = dev_data;
	dev->read_callback = read_callback;
	*handle = handle_count;
	handle_count++;

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}

#if PANC_TESTS_ENABLED != 0
PANCSTATUS pancake_write_test(PANCHANDLE handle)
{
	uint8_t ret;
	struct pancake_main_dev	*dev = &devs[handle];
	struct ip6_hdr hdr = {
		.ip6_flow	=	htonl(6 << 28),
		.ip6_plen	=	htons(255),
		.ip6_nxt	=	254,
		.ip6_hops	=	255,
		.ip6_src	=	{
			/* Loopback (::1/128) */
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0xff, 0xfe, 0, 0, 1,
		},
		.ip6_dst	=	{
			/* Loopback (::1/128) */
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 1,
		},
	};
	uint16_t length	= sizeof(hdr);

	ret = dev->cfg->write_func(dev->dev_data, NULL, (uint8_t*)&hdr, length);
	if (ret != PANCSTATUS_OK || length != sizeof(hdr)) {
		goto err_out;
	}

	dev->read_callback(NULL, "Write test successful!", 0);

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}
#endif

void pancake_destroy(PANCHANDLE handle)
{
	struct pancake_main_dev *dev = &devs[handle];
	PANCSTATUS ret;

	ret = dev->cfg->destroy_func(dev->dev_data);
	if (ret != PANCSTATUS_OK) {
		/* What to do, what to do? */
	}
}

PANCSTATUS pancake_send(PANCHANDLE handle, struct ip6_hdr *hdr, uint8_t *payload, uint16_t payload_length)
{
	uint8_t i;
	uint8_t raw_data[aMaxPHYPacketSize - aMaxFrameOverhead];
	uint16_t length;
	struct pancake_main_dev *dev;
	struct pancake_compressed_ip6_hdr compressed_ip6_hdr;
	compressed_ip6_hdr.hdr_data = raw_data;
	uint16_t frame_overhead;
	PANCSTATUS ret;

	/* Sanity check */
	if ( 	handle < 0 ||
			handle > PANC_MAX_DEVICES ||
			hdr == NULL ||
			(payload == NULL && payload_length != 0) ) {
		goto err_out;
	}
	dev = &devs[handle];

	switch (dev->options->compression) {
	case PANC_COMPRESSION_NONE:
		compressed_ip6_hdr.dispatch_value = DISPATCH_IPv6;
		compressed_ip6_hdr.hdr_data = (uint8_t *)hdr;
		compressed_ip6_hdr.size = 40;
		break;
	case PANC_COMPRESSION_HCIP:
		pancake_compress_header(hdr, &compressed_ip6_hdr);
		// TODO; Add dispatch value
		break;
	default:
		/* Not supported... yet */
		goto err_out;
	}

	/* Check if fragmentation is needed */
	frame_overhead = calculate_frame_overhead(dev, &compressed_ip6_hdr);
	if (frame_overhead + aMaxFrameOverhead + payload_length > aMaxPHYPacketSize) {
		/* pancake_send_fragmented() sends the whole payload, but in multiple packets */
		ret = pancake_send_fragmented(dev, raw_data, &compressed_ip6_hdr, payload, payload_length);
		if (ret != PANCSTATUS_OK) {
			goto err_out;
		}
	}
	/* Packet fits inside a single packet, copy header and payload to raw_data and send */
	else {
		/* Dispatch value */
		*raw_data = compressed_ip6_hdr.dispatch_value;
		/* Copy header and data */
		memcpy((void*)(raw_data + 1), (void*)compressed_ip6_hdr.hdr_data, compressed_ip6_hdr.size);
		memcpy((void*)(raw_data + compressed_ip6_hdr.size + 1), (void*)payload, payload_length);

		length = frame_overhead+payload_length;
		pancake_printf("Frame overhead: %u\n", frame_overhead);
		pancake_printf("Payload length: %u\n", payload_length);
		ret = dev->cfg->write_func(dev->dev_data, NULL, raw_data, length);
		if (ret != PANCSTATUS_OK) {
			goto err_out;
		}
	}

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}

PANCSTATUS pancake_send_packet(PANCHANDLE handle, uint8_t *ip6_packet, uint16_t packet_length)
{
	return pancake_send(handle, (struct ip6_hdr *)ip6_packet, ip6_packet+40, packet_length-40);
}

static PANCHANDLE pancake_handle_from_dev_data(void *dev_data)
{
	int i;
	struct pancake_main_dev *dev;
	for (i=0; i<PANC_MAX_DEVICES; i++) {
		dev = &devs[i];
		if (dev->dev_data == dev_data) {
			return i;
		}
	}
	return -1;
}

PANCSTATUS pancake_process_data(void *dev_data, struct pancake_ieee_addr *src, struct pancake_ieee_addr *dst, uint8_t *data, uint16_t size)
{
	struct ip6_hdr *hdr;
	uint8_t *payload;
	uint16_t payload_length;
	PANCSTATUS ret = PANCSTATUS_ERR;
	PANCHANDLE handle;
	struct pancake_main_dev *dev;
	struct pancake_reassembly_buffer *ra_buf = NULL;

	/* Try to get handle */
	handle = pancake_handle_from_dev_data(dev_data);
	if (handle < 0) {
		goto out;
	}
	dev = &devs[handle];

	/* Read dispatch value */
	switch (*data) {
	case DISPATCH_IPv6:
		payload = data + 1 + 40;
		payload_length = size - (1 + 40);
		break;
	case DISPATCH_HC1:
	case DISPATCH_BC0:
	case DISPATCH_IPHC:
	default:
		switch (*data & 0xF8) {
		case DISPATCH_FRAG1:
		case DISPATCH_FRAGN:
			ret = pancake_reassemble(dev, &ra_buf, src, dst, data, size);
			if (ret != PANCSTATUS_OK) {
				goto out;
			}

			payload = ra_buf->data;
			payload_length = (ra_buf->frag_hdr.size & 0x7FF);
			break;
		default:
			if (*data & 0xC0 == DISPATCH_MESH) {
				/* Not implemented yet */
				goto out;
			}
			else {
				/* Not a LowPAN frame */
				pancake_printf("Not a LoWPAN frame?:\n");
				pancake_print_raw_bits(NULL, data, size);
				goto out;
			}
		}
	};

	/* Relay data to upper levels */
	dev->read_callback(hdr, payload, payload_length);
	ret = PANCSTATUS_OK;

out:
	print_pancake_error("pancake_process_data()", ret);
	return ret;
}

