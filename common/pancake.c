#include <pancake.h>
#include <stddef.h>
#include <string.h>

struct pancake_main_dev {
	struct pancake_dev_cfg		*cfg;
	struct pancake_options_cfg	*options;
	void						*dev_data;
	read_callback_func			read_callback;
};
static struct pancake_main_dev devs[PANC_MAX_DEVICES];

PANCSTATUS pancake_init(PANCHANDLE *handle, struct pancake_options_cfg *options_cfg, struct pancake_dev_cfg *dev_cfg, void *dev_data, read_callback_func read_callback)
{
	int8_t ret;
	static uint8_t handle_count = 0;
	struct pancake_main_dev *dev = &devs[handle_count];

	/* Sanity check */
	if (handle == NULL || options_cfg == NULL || dev_cfg == NULL || handle_count+1 > PANC_MAX_DEVICES) {
		goto err_out;
	}
	if (dev_cfg->write_func == NULL) {
		goto err_out;
	}

	/* Initialize port */
	if (dev_cfg->init_func) {
		ret = dev_cfg->init_func(dev_data);
		if (ret < 0) {
			goto err_out;
		}
	}

	/* Save config and device data, and update handle */
	dev->cfg = dev_cfg;
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
		.ip6_flow	=	htonl(6 << 28),
		.ip6_plen	=	htons(255),
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

	ret = dev->cfg->write_func(dev->dev_data, (uint8_t*)&hdr, &length);
	if (ret != PANCSTATUS_OK || length != sizeof(hdr)) {
		goto err_out;
	}

	dev->read_callback(NULL, "Write test successful!", 0);

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}


PANCSTATUS pancake_send(PANCHANDLE handle, struct ip6_hdr *hdr, uint8_t *payload, uint16_t payload_length)
{
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

PANCSTATUS pancake_process_data(void *dev_data, uint8_t *data, uint16_t size)
{
	PANCHANDLE handle = pancake_handle_from_dev_data(dev_data);

	/* way to get a handle */
	if (handle < 0) {
		goto err_out;
	}

err_out:
	return PANCSTATUS_ERR;
}
