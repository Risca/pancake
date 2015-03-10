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
	.compression = PANC_COMPRESSION_IPHC,
	.security = PANC_SECURITY_NONE,
};
PANCHANDLE my_pancake_handle;

static void my_read_callback(pancake_event *event)
{
	struct pancake_event_data_received *data_event;
	struct ip6_hdr *hdr;
	uint8_t *payload;
	uint16_t size;

	if (event->type != PANC_EVENT_DATA_RECEIVED) {
		printf("main.c: Unhandled event\n");
		return;
	}

	data_event = &event->data_received;
	hdr = data_event->hdr;
	payload = data_event->payload;
	size = data_event->payload_length;
#if 0
	printf("main.c: Looping incoming packet to output again\n");
	pancake_send(my_pancake_handle, hdr, payload, size);
#else
	printf("main.c: We received the following packet:\n");
	pancake_print_raw_bits(stdout, (uint8_t *)hdr, 40);
	pancake_print_raw_bits(stdout, payload, size);
#endif
}

void my_test_function()
{
	PANCSTATUS ret;
	uint8_t i, timeout = 1;
	uint8_t data[127*3];
	uint16_t payload_length = 2;
	struct ip6_hdr 	*hdr 		= (struct ip6_hdr *)(data);
	uint8_t			*payload	= data + 1 + 40;

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
#if 1
	fprintf(stdout, "Sending BIG packet:\n");
	for (i = 0; i < 200-20; i++) {
		payload[i] = i;
	}
	populate_dummy_ipv6_header(hdr, 200-20);
	payload_length = 200-20;
	SLEEP(timeout);
	ret = pancake_send(my_pancake_handle, hdr, payload, payload_length);
	if (ret != PANCSTATUS_OK) {
		printf("Failed to send BIG packet!\n");
	}
#endif
}

int main(int argc, char **argv)
{
	PANCSTATUS ret;

	ret = pancake_init(&my_pancake_handle, &my_linux_options, &linux_cfg, NULL, my_read_callback);

	if (ret != PANCSTATUS_OK) {
		printf("main.c: pancake failed to initialize!\n");
		return EXIT_FAILURE;
	}


#if 1
	my_test_function();
#elif 0
	ret = pancake_reassembly_test(my_pancake_handle);
	if (ret != PANCSTATUS_OK) {
		printf("main.c: reassembly test failed\n");
	}
#else
	ret = pancake_compression_test(my_pancake_handle);
	if (ret != PANCSTATUS_OK) {
		printf("main.c: decompression test failed\n");
	}
#endif

	pancake_destroy(my_pancake_handle);

	return EXIT_SUCCESS;
}
