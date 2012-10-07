#include <foo.h>
#include <stdint.h>
#include <stdio.h>

static uint8_t rpi_read_func(void);
static uint8_t rpi_write_func(void *dev_data, uint8_t *data, uint16_t *length);

struct foo_main_cfg rpi_cfg = {
    .read_func = rpi_read_func,
    .write_func = rpi_write_func,
};

static uint8_t rpi_read_func(void)
{
	return -1;
}

static uint8_t rpi_write_func(void *dev_data, uint8_t *data, uint16_t *length)
{
	size_t ret;
	FILE *out = (FILE*)dev_data;

	ret = fprintf(out, "%s", (char*)data);
	if (ret < 0) {
		goto err_out;
	}
	*length = ret;

	return 0;
err_out:
	return -1;
}
