#include <stdio.h>
#include <stdlib.h>
#include <pancake.h>
#include <helpers.h>
#include <netinet/ip6.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#define SLEEP(x) Sleep(x*1000)
#else
#define SLEEP(x) sleep(x)
#endif

extern struct pancake_port_cfg linux_cfg;
struct pancake_options_cfg my_linux_options = {
	.compression = PANC_COMPRESSION_HCIP,
	.security = PANC_SECURITY_NONE,
};
PANCHANDLE my_pancake_handle;

void my_read_callback(struct ip6_hdr *hdr, uint8_t *payload, uint16_t size)
{

#if 0
	printf("main.c: Looping incoming packet to output again\n");
	pancake_send(my_pancake_handle, hdr, payload, size);
#elif 0
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
		fprintf(stdout, "Sending small packet:\n");
		*payload = i;
		*(payload+1) = 255-i;
		populate_dummy_ipv6_header(hdr, 2);
		SLEEP(timeout);
		ret = pancake_send(my_pancake_handle, hdr, payload, payload_length);
		if (ret != PANCSTATUS_OK) {
			printf("Failed to send!\n");
			/* What to do, what to do? */
		}
	}
#if 0
	fprintf(stdout, "Sending BIG packet:\n");
	for (i = 0; i < 200; i++) {
		payload[i] = i;
	}
	populate_dummy_ipv6_header(hdr, 200);
	payload_length = 200;
	SLEEP(timeout);
	ret = pancake_send(my_pancake_handle, hdr, payload, payload_length);
	if (ret != PANCSTATUS_OK) {
		printf("Failed to send BIG packet!\n");
	}
#endif
}

int main(int argc, char **argv)
{
	char ipstr[INET6_ADDRSTRLEN];
	PANCSTATUS ret;
	
	
	if (argc > 1) {
		strcpy(ipstr, (char *) argv[1]);
		printf("Will connect to :\t%s\n",ipstr);
	} else {
		strcpy(ipstr, "::");
	}

	ret = pancake_init(&my_pancake_handle, &my_linux_options, &linux_cfg, stdout, my_read_callback);
	if (ret != PANCSTATUS_OK) {
		printf("main.c: pancake failed to initialize!\n");
		return EXIT_FAILURE;
	}

	//if (argc > 1) {
		my_test_function();
	//}

	

	pancake_destroy(my_pancake_handle);

	return EXIT_SUCCESS;
}
