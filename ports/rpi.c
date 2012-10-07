#include <foo.h>
#include <stdint.h>
#include <stdio.h>

static FOOSTATUS rpi_init_func(void *dev_data);
static FOOSTATUS rpi_read_func(void);
static FOOSTATUS rpi_write_func(void *dev_data, uint8_t *data, uint16_t *length);

struct foo_dev_cfg rpi_cfg = {
	.init_func = rpi_init_func,
    .read_func = rpi_read_func,
    .write_func = rpi_write_func,
};

static FOOSTATUS rpi_init_func(void *dev_data)
{
	return FOOSTATUS_OK;
}

static FOOSTATUS rpi_read_func(void)
{
	return FOOSTATUS_ERR;
}

static FOOSTATUS rpi_write_func(void *dev_data, uint8_t *data, uint16_t *length)
{
	size_t ret;
	FILE *out = (FILE*)dev_data;

	ret = fprintf(out, "%s", (char*)data);
	if (ret < 0) {
		goto err_out;
	}
	*length = ret;

	return FOOSTATUS_OK;
err_out:
	return FOOSTATUS_ERR;
}
