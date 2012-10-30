#include <stdio.h>
#include <stdlib.h>
#include <pancake.h>
#include <netinet/ip6.h>

extern struct pancake_dev_cfg linux_cfg;
struct pancake_options_cfg my_options = {
	.compression = PANC_COMPRESSION_NONE,
	.security = PANC_SECURITY_NONE,
};
PANCHANDLE my_pancake_handle;

static void my_read_callback(struct ip6_hdr *hdr, uint8_t *payload, uint16_t size)
{
	if (hdr == NULL) {
		printf("main.c: Got message: %s\n", payload);
		return;
	}
	printf("main.c: Looping incoming packet to output again\n");

	pancake_send(my_pancake_handle, hdr, payload, size);
}

int main(int argc, int **argv)
{
	PANCSTATUS ret;
#if 0
	ret = pancake_init(&my_pancake_handle, &my_options, &linux_cfg, NULL, my_read_callback);
	if (ret != PANCSTATUS_OK) {
		printf("main.c: pancake failed to initialize!\n");
	}


	ret = pancake_write_test(my_pancake_handle);
	if (ret != PANCSTATUS_OK) {
		printf("pancake_write_test failed!\n");
	}
#endif
	pancake_destroy(my_pancake_handle);

	return EXIT_SUCCESS;
}
