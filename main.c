#include <stdio.h>
#include <stdlib.h>
#include <foo.h>

extern struct foo_main_cfg rpi_cfg;
FOOHANDLE my_foo_handle;

int main(int argc, int **argv)
{
	uint8_t ret;

	ret = foo_init(&my_foo_handle, &rpi_cfg, stdout);
	if (ret < 0) {
		printf("foo failed to initialize!\n");
	}

	ret = foo_write_test(my_foo_handle);
	if (ret < 0) {
		printf("foo_write_test failed!\n");
	}
	return EXIT_SUCCESS;
}
