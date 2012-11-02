#include <stdio.h>
#include <stdlib.h>
#include <pancake.h>
#include <helpers.h>
#include <netinet/ip6.h>

extern struct pancake_port_cfg linux_cfg;
struct pancake_options_cfg my_linux_options = {
	.compression = PANC_COMPRESSION_NONE,
	.security = PANC_SECURITY_NONE,
};
PANCHANDLE my_pancake_handle;

void my_read_callback(struct ip6_hdr *hdr, uint8_t *payload, uint16_t size)
{
	if (hdr == NULL) {
		printf("main.c: Got message: %s\n", payload);
		return;
	}

#if 0
	printf("main.c: Looping incoming packet to output again\n");
	pancake_send(my_pancake_handle, hdr, payload, size);
#else
	printf("main.c: We received the following packet:\n");
	pancake_print_raw_bits(stdout, payload, size);
#endif
}

int main(int argc, int **argv)
{
	PANCSTATUS ret;

	ret = pancake_init(&my_pancake_handle, &my_linux_options, &linux_cfg, stdout, my_read_callback);
	if (ret != PANCSTATUS_OK) {
		printf("main.c: pancake failed to initialize!\n");
	}

#if 0
	ret = pancake_write_test(my_pancake_handle);
	if (ret != PANCSTATUS_OK) {
		printf("pancake_write_test failed!\n");
	}
#endif

#if 1
	ret = pancake_reassembly_test(my_pancake_handle);
	if (ret != PANCSTATUS_OK) {
		printf("pancake_write_test failed!\n");
	}
#endif
	pancake_destroy(my_pancake_handle);

	return EXIT_SUCCESS;
}
