#include <pancake.h>
#include <stdint.h>
#include <stdio.h>

static PANCSTATUS linux_init_func(void *dev_data);
static PANCSTATUS linux_read_func(void);
static PANCSTATUS linux_write_func(void *dev_data, uint8_t *data, uint16_t *length);

struct pancake_dev_cfg linux_cfg = {
	.init_func = linux_init_func,
    .read_func = linux_read_func,
    .write_func = linux_write_func,
};

static PANCSTATUS linux_init_func(void *dev_data)
{
	return PANCSTATUS_OK;
}

static PANCSTATUS linux_read_func(void)
{
	return PANCSTATUS_ERR;
}

static PANCSTATUS linux_write_func(void *dev_data, uint8_t *data, uint16_t *length)
{
	size_t ret;
	FILE *out = (FILE*)dev_data;

	ret = fprintf(out, "%s", (char*)data);
	if (ret < 0) {
		goto err_out;
	}
	*length = ret;

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}
