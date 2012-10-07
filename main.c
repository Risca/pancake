#include <stdio.h>
#include <stdlib.h>
#include <foo.h>

extern struct foo_dev_cfg rpi_cfg;
struct foo_opts_cfg my_rpi_opts = {
	.compression = FOO_COMPRESSION_NONE,
	.security = FOO_SECURITY_NONE,
};
FOOHANDLE my_foo_handle;
struct foo_opts_cfg my_rpi_opts_two = {
	.compression = FOO_COMPRESSION_NONE,
	.security = FOO_SECURITY_NONE,
};
FOOHANDLE my_foo_handle_two;

int main(int argc, int **argv)
{
	FOOSTATUS ret;

	ret = foo_init(&my_foo_handle, &my_rpi_opts, &rpi_cfg, stdout);
	if (ret != FOOSTATUS_OK) {
		printf("foo failed to initialize!\n");
	}
	ret = foo_init(&my_foo_handle_two, &my_rpi_opts_two, &rpi_cfg, stderr);
	if (ret != FOOSTATUS_OK) {
		printf("foo failed to initialize!\n");
	}

	ret = foo_write_test(my_foo_handle);
	if (ret != FOOSTATUS_OK) {
		printf("foo_write_test failed!\n");
	}
	ret = foo_write_test(my_foo_handle_two);
	if (ret != FOOSTATUS_OK) {
		printf("foo_write_test failed!\n");
	}
	return EXIT_SUCCESS;
}
