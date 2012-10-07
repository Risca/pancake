#include <stdio.h>
#include <stdlib.h>
#include <pancake.h>

extern struct pancake_dev_cfg rpi_cfg;
struct pancake_opts_cfg my_rpi_opts = {
	.compression = PANC_COMPRESSION_NONE,
	.security = PANC_SECURITY_NONE,
};
PANCHANDLE my_pancake_handle;
struct pancake_opts_cfg my_rpi_opts_two = {
	.compression = PANC_COMPRESSION_NONE,
	.security = PANC_SECURITY_NONE,
};
PANCHANDLE my_pancake_handle_two;

int main(int argc, int **argv)
{
	PANCSTATUS ret;

	ret = pancake_init(&my_pancake_handle, &my_rpi_opts, &rpi_cfg, stdout);
	if (ret != PANCSTATUS_OK) {
		printf("pancake failed to initialize!\n");
	}
	ret = pancake_init(&my_pancake_handle_two, &my_rpi_opts_two, &rpi_cfg, stderr);
	if (ret != PANCSTATUS_OK) {
		printf("pancake failed to initialize!\n");
	}

	ret = pancake_write_test(my_pancake_handle);
	if (ret != PANCSTATUS_OK) {
		printf("pancake_write_test failed!\n");
	}
	ret = pancake_write_test(my_pancake_handle_two);
	if (ret != PANCSTATUS_OK) {
		printf("pancake_write_test failed!\n");
	}
	return EXIT_SUCCESS;
}
