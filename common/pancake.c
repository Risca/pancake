#include <pancake.h>
#include <stddef.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
#endif

struct pancake_main_dev {
	struct pancake_port_cfg		*cfg;
	struct pancake_options_cfg	*options;
	void				*dev_data;
	read_callback_func		read_callback;
};
static struct pancake_main_dev devs[PANC_MAX_DEVICES];

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

/*
PANCSTATUS pancake_update()
{

  	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}
*/

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

	ret = dev->cfg->write_func(dev->dev_data, (uint8_t*)&hdr, length);
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
	uint8_t data[127];
	uint16_t length;
	struct pancake_main_dev *dev;
	PANCSTATUS ret;

	/* Sanity check */
	if ( 	handle < 0 ||
			handle > PANC_MAX_DEVICES ||
			hdr == NULL ||
			(payload == NULL && payload_length != 0) ) {
		goto err_out;
	}
	dev = &devs[handle];

	/* Below this point we assume a lot! */
	memcpy(data, hdr, 40);
	memcpy(data+40, payload, payload_length);
	length = 40 + payload_length;

	ret = dev->cfg->write_func(dev->dev_data, data, length);
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

