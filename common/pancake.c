#include <pancake.h>
#include <stddef.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
#endif

/* Updated to reflect RFC6282 */
#define NALP        0x00
#define ESC         0x40
#define IPv6        0x41
#define LOWPAN_HC1  0x42
#define LOWPAN_BC0  0x50
#define LOWPAN_IPHC 0x7F
#define MESH        0x80
#define FRAG1       0xC0
#define FRAGN       0xE0

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

static uint16_t calculate_frame_overhead(struct pancake_main_dev *dev, struct pancake_compressed_ip6_hdr *hdr)
{
	uint16_t overhead = 0;
	struct pancake_options_cfg *options = dev->options;

#if 0
	/* Link layer overhead (might not be needed) */
	overhead += aMaxFrameOverhead;
#endif 

	/* Overhead due to security */
	overhead += security_overhead[options->security];

	/* Compression overhead (+1 dispatch byte) */
	overhead += hdr->size + 1;

	return overhead;
}

#include "pancake_internals/fragmentation.c"
#include "pancake_internals/reassembly.c"

PANCSTATUS pancake_init(PANCHANDLE *handle, struct pancake_options_cfg *options_cfg, struct pancake_port_cfg *port_cfg, void *dev_data, read_callback_func read_callback)
{
	int8_t ret;
	static uint8_t handle_count = 0;
	struct pancake_main_dev *dev = &devs[handle_count];

	/* Sanity check */
	if (handle == NULL || options_cfg == NULL || port_cfg == NULL || handle_count+1 > PANC_MAX_DEVICES) {
		goto err_out;
	}
	if (port_cfg->write_func == NULL) {
		goto err_out;
	}

	/* Initialize port */
	if (port_cfg->init_func) {
		ret = port_cfg->init_func(dev_data);
		if (ret < 0) {
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

PANCSTATUS pancake_write_test(PANCHANDLE handle)
{
	uint8_t ret;
	struct pancake_main_dev	*dev = &devs[handle];
	struct ip6_hdr hdr = {
		//.ip6_flow	=	htonl(6 << 28),
		//.ip6_plen	=	htons(255),
		.ip6_nxt	=	254,
		.ip6_hops	=	2,
		.ip6_src	=	{
			/* Loopback (::1/128) */
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 1,
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
		compressed_ip6_hdr.hdr_data = (uint8_t *)hdr;
		compressed_ip6_hdr.size = 40;
		compressed_ip6_hdr.dispatch_value = IPv6;
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
	PANCSTATUS ret;
	PANCHANDLE handle;
	struct pancake_main_dev *dev;

	/* Try to get handle */
	handle = pancake_handle_from_dev_data(dev_data);
	if (handle < 0) {
		goto err_out;
	}
	dev = &devs[handle];

	/* Read dispatch value */
	switch (*data) {
	case IPv6:
		break;
	case LOWPAN_HC1:
	case LOWPAN_BC0:
	case LOWPAN_IPHC:
	default:
		switch (*data & 0xF8) {
		case FRAG1:
		case FRAGN:
			pancake_reassemble(dev, src, dst, data, size);
			break;
		default:
			if (*data & 0xC0 == MESH) {
				/* Not implemented yet */
				goto err_out;
			}
			else {
				/* Not a LowPAN frame */
				goto err_out;
			}
		}
	};

	/* From this point on, we assume a lot! */
	hdr = (struct ip6_hdr *)(data + 1);
	payload = data + 1 + 40;
	payload_length = size - (1 + 40);

	/* Relay data to upper levels */
	dev->read_callback(hdr, payload, payload_length);

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}

