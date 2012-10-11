#include <pancake.h>
#include <stddef.h>
#include <string.h>

struct pancake_main_dev {
	struct pancake_dev_cfg		*cfg;
	struct pancake_opts_cfg		*opts;
	void 					*dev_data;
};
static struct pancake_main_dev devs[PANC_MAX_DEVICES];

PANCSTATUS pancake_init(PANCHANDLE *handle, struct pancake_opts_cfg *opts_cfg, struct pancake_dev_cfg *dev_cfg, void *dev_data)
{
	int8_t ret;
	static uint8_t handle_count = 0;
	struct pancake_main_dev *dev = &devs[handle_count];

	/* Sanity check */
	if (handle == NULL || opts_cfg == NULL || dev_cfg == NULL || handle_count+1 > PANC_MAX_DEVICES) {
		goto err_out;
	}
	if (dev_cfg->read_func == NULL || dev_cfg->write_func == NULL) {
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
	dev->opts = opts_cfg;
	dev->dev_data = dev_data;
	*handle = handle_count;
	handle_count++;

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}

PANCSTATUS pancake_write_test(PANCHANDLE handle)
{
	uint8_t 			ret;
	struct pancake_main_dev	*dev	= &devs[handle];
	char 				*str 	= "Hello world!\n";
	uint16_t 			length	= strlen(str);

	ret = dev->cfg->write_func(dev->dev_data, str, &length);
	if (ret != PANCSTATUS_OK || length != strlen(str)) {
		goto err_out;
	}

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}
