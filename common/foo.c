#include <foo.h>
#include <stddef.h>
#include <string.h>

struct foo_main_dev {
	struct foo_dev_cfg		*cfg;
	struct foo_opts_cfg		*opts;
	void 					*dev_data;
};
static struct foo_main_dev devs[FOO_MAX_DEVICES];

FOOSTATUS foo_init(FOOHANDLE *handle, struct foo_opts_cfg *opts_cfg, struct foo_dev_cfg *dev_cfg, void *dev_data)
{
	int8_t ret;
	static uint8_t handle_count = 0;
	struct foo_main_dev *dev = &devs[handle_count];

	/* Sanity check */
	if (handle == NULL || opts_cfg == NULL || dev_cfg == NULL || handle_count+1 > FOO_MAX_DEVICES) {
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

	return FOOSTATUS_OK;
err_out:
	return FOOSTATUS_ERR;
}

FOOSTATUS foo_write_test(FOOHANDLE handle)
{
	uint8_t 			ret;
	struct foo_main_dev	*dev	= &devs[handle];
	char 				*str 	= "Hello world!\n";
	uint16_t 			length	= strlen(str);

	ret = dev->cfg->write_func(dev->dev_data, str, &length);
	if (ret != FOOSTATUS_OK || length != strlen(str)) {
		goto err_out;
	}

	return FOOSTATUS_OK;
err_out:
	return FOOSTATUS_ERR;
}
