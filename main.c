#include <stdio.h>
#include <stdlib.h>
#include <pancake.h>
#include <helpers.h>
#include <netinet/ip6.h>
#include <string.h>
#include <unistd.h>

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

void my_test_function()
{
	PANCSTATUS ret;
	uint8_t i, timeout = 1;
	uint8_t data[127*3];
	uint16_t payload_length = 2;
	struct ip6_hdr 	*hdr 		= (struct ip6_hdr *)(data+1);
	uint8_t			*payload	= data + 1 + 40;

	/* Raw IPv6 packet dispatch value */
	data[0] = 0x41;

	/* Send 3 packets with 1 seconds delay */
	for (i=0; i < 3; i++) {
		*payload = i;
		*(payload+1) = 255-i;
		populate_dummy_ipv6_header(hdr, 2);
#ifdef _WIN32
		Sleep(timeout*1000);
#else
		sleep(timeout);
#endif
		ret = pancake_send(my_pancake_handle, hdr, payload, payload_length);
		if (ret != PANCSTATUS_OK) {
			printf("Failed to send!\n");
			/* What to do, what to do? */
		}
	}

	for (i = 0; i < 200; i++) {
		payload[i] = i;
	}
	populate_dummy_ipv6_header(hdr, 200);
	payload_length = 200;
#ifdef _WIN32
	Sleep(timeout*1000);
#else
	sleep(timeout);
#endif
	ret = pancake_send(my_pancake_handle, hdr, payload, payload_length);
	if (ret != PANCSTATUS_OK) {
		printf("Failed to send big packet!\n");
	}
}

int main(int argc, char **argv)
{
	PANCSTATUS ret;

	ret = pancake_init(&my_pancake_handle, &my_linux_options, &linux_cfg, stdout, my_read_callback);
	if (ret != PANCSTATUS_OK) {
		printf("main.c: pancake failed to initialize!\n");
		return EXIT_FAILURE;
	}

#if 1
	my_test_function();
#else
	ret = pancake_reassembly_test(my_pancake_handle);
	if (ret != PANCSTATUS_OK) {
		printf("main.c: reassembly test failed\n");
	}
#endif

	pancake_destroy(my_pancake_handle);

	return EXIT_SUCCESS;
}
