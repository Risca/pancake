#include <pancake.h>
#include <stddef.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
#endif

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

#include "fragmentation.c"

static uint16_t calculate_frame_overhead(struct pancake_main_dev *dev, struct pancake_compressed_ip6_hdr *hdr)
{
	uint16_t overhead = 0;
	struct pancake_options_cfg *options = dev->options;

	/* Link layer overhead (might not be needed) */
	overhead += aMaxFrameOverhead;

	/* Overhead due to security */
	overhead += security_overhead[options->security];

	/* Compression overhead */
	overhead += hdr->size;

	return overhead;
}

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
	uint8_t data[aMaxPHYPacketSize];
	uint16_t length;
	struct pancake_main_dev *dev;
	struct pancake_compressed_ip6_hdr compressed_ip6_hdr;
	struct pancake_frag_hdr *frag_hdr;
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
#if 0
		compressed_ip6_hdr.hdr_data = hdr;
#endif
		compressed_ip6_hdr.size = 40;
		break;
	default:
		/* Not supported... yet */
		goto err_out;
	}

	frame_overhead = calculate_frame_overhead(dev, &compressed_ip6_hdr);

	if (frame_overhead+payload_length > aMaxPHYPacketSize) {
		frame_overhead += 4;
		for (i=0; frame_overhead+payload_length > aMaxPHYPacketSize; i++) {
			/* Fragment and send packages */
			if (i == 0) {
				frag_hdr = (struct pancake_frag_hdr *)(data + (frame_overhead-4));
			}
			else {
				frag_hdr = (struct pancake_frag_hdr *)(data + (frame_overhead-5));
			}
			populate_fragmentation_header(handle, frag_hdr, frame_overhead - compressed_ip6_hdr.size, payload_length+compressed_ip6_hdr.size, i);
			length = frame_overhead+payload_length; /* Should be 127 bytes */
			ret = dev->cfg->write_func(dev->dev_data, data, length);
			if (ret != PANCSTATUS_OK) {
				goto err_out;
			}
			if (i == 0) {
				frame_overhead += 1;
			}
			payload_length -= (aMaxPHYPacketSize - frame_overhead);
		}
	}

	length = frame_overhead+payload_length;
	ret = dev->cfg->write_func(dev->dev_data, NULL, data, length);
	if (ret != PANCSTATUS_OK) {
		goto err_out;
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

#include <stdio.h>
PANCSTATUS pancake_process_data(void *dev_data, uint8_t *data, uint16_t size)
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

	/* From this point on, we assume a lot! */
	hdr = (struct ip6_hdr *)data;
	payload = data+40;
	payload_length = size-40;

	/* Relay data to upper levels */
	dev->read_callback(hdr, payload, payload_length);

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}

