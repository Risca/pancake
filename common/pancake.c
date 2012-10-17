#include <pancake.h>
#include <stddef.h>
#include <string.h>
#include <netinet/ip6.h>

struct pancake_main_dev {
	struct pancake_dev_cfg		*cfg;
	struct pancake_options_cfg	*options;
	void 					*dev_data;
};
static struct pancake_main_dev devs[PANC_MAX_DEVICES];

PANCSTATUS pancake_init(PANCHANDLE *handle, struct pancake_options_cfg *options_cfg, struct pancake_dev_cfg *dev_cfg, void *dev_data)
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
	dev_cfg->handle = handle_count;
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

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}

PANCSTATUS pancake_process_data(PANCHANDLE handle, uint8_t *data, uint16_t size)
{
	return PANCSTATUS_ERR;
}
