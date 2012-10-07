#include <foo.h>
#include <stddef.h>
#include <string.h>

struct foo_main_dev {
	struct foo_main_cfg		*cfg;
	void 					*dev_data;
};
static struct foo_main_dev devs[FOO_MAX_DEVICES];

uint8_t foo_init(FOOHANDLE *handle, struct foo_main_cfg *cfg, void *dev_data)
{
	static uint8_t handle_count = 0;
	struct foo_main_dev *dev = &devs[handle_count];

	/* Sanity check */
	if (handle == NULL || cfg == NULL) {
		goto err_out;
	}

	/* Save config and device data, and update handle */
	dev->cfg = cfg;
	dev->dev_data = dev_data;
	*handle = handle_count;

	return 0;
err_out:
	return -1;
}

uint8_t foo_write_test(FOOHANDLE handle)
{
	uint8_t 			ret;
	struct foo_main_dev	*dev	= &devs[handle];
	char 				*str 	= "Hello world!\n";
	uint16_t 			length	= strlen(str);

	ret = dev->cfg->write_func(dev->dev_data, str, &length);
	if (ret < 0 || length != strlen(str)) {
		goto err_out;
	}

	return 0;
err_out:
	return -1;
}
