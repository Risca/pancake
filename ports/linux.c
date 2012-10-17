#include <pancake.h>
#include <stdint.h>
#include <stdio.h>

static PANCSTATUS linux_init_func(void *dev_data);
static PANCSTATUS linux_write_func(void *dev_data, uint8_t *data, uint16_t *length);
static void linux_read_func(uint8_t *data, int16_t length);

struct pancake_dev_cfg linux_cfg = {
	.init_func = linux_init_func,
    .write_func = linux_write_func,
};

static PANCSTATUS linux_init_func(void *dev_data)
{
	return PANCSTATUS_OK;
}

static void linux_read_func(uint8_t *data, int16_t length)
{
	pancake_process_data(linux_cfg.handle, data, length);
}

static PANCSTATUS linux_write_func(void *dev_data, uint8_t *data, uint16_t *length)
{
	size_t ret;
	uint8_t bit;
	uint16_t i;
	uint8_t j;
	FILE *out = (FILE*)dev_data;

	fputs("Sending packet to device:\n", out);
	fputs("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n", out);
	for (i=0; i < (*length); i++) {
		fprintf(out, "|");
		for (j=0; j < 8; j++) {
			bit = ( (*data) >> (7-j%8) ) & 0x01;
			fprintf(out, "%i", bit);

			/* Place a space between every bit except last */
			if (j != 7) {
				fprintf(out, " ");
			}
		}
		
		if ( (i+1) % 4 == 0 ) {
			fputs("|\n", out);
			fputs("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n", out);
		}
		data++;
	}

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}
