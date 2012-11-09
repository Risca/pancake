#include <stdio.h>
#include <stdlib.h>
#include <pancake.h>
#include <helpers.h>
#include <netinet/ip6.h>
#include <string.h>

extern struct pancake_port_cfg linux_sockets_cfg;
struct pancake_options_cfg my_linux_sockets_options = {
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

#if 1
void populate_dummy_ipv6_header(struct ip6_hdr *hdr, uint16_t payload_length)
{
	/* Loopback (::1/128) */
	struct in6_addr addr = {
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 1};

	hdr->ip6_flow	=	htonl(6 << 28);
	hdr->ip6_plen	=	htons(payload_length);
	hdr->ip6_nxt	=	254;
	hdr->ip6_hops	=	2;

    // Add next bytes
	memcpy((uint8_t *)hdr + 8, &addr, 16);
	memcpy((uint8_t *)hdr + 24, &addr, 16);
}
#endif

int main(int argc, int **argv)
{
	char ipstr[INET6_ADDRSTRLEN];
	uint8_t i, timeout = 1;
	uint8_t data[127*3];
	uint16_t payload_length = 2;
	PANCSTATUS ret;
	struct ip6_hdr 	*hdr 		= (struct ip6_hdr *)(data+1);
	uint8_t			*payload	= data + 1 + 40;

	/* Raw IPv6 packet dispatch value */
	data[0] = 0x41;

	if (argc > 1) {
		strcpy(ipstr, (char *) argv[1]);
		printf("Will connect to :\t%s\n",ipstr);
	} else {
		strcpy(ipstr, "::");
	}

	ret = pancake_init(&my_pancake_handle, &my_linux_sockets_options, &linux_sockets_cfg, ipstr, my_read_callback);
	if (ret != PANCSTATUS_OK) {
		printf("main.c: pancake failed to initialize!\n");
		return EXIT_FAILURE;
	}

	if (argc > 1) {
		/* Send 3 packets with 1 seconds delay */
		for (i=0; i < 3; i++) {
			*payload = i;
			*(payload+1) = 255-i;
			populate_dummy_ipv6_header(hdr, 2);
			sleep(timeout);
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
		sleep(timeout);
		ret = pancake_send(my_pancake_handle, hdr, payload, payload_length);
		if (ret != PANCSTATUS_OK) {
			printf("Failed to send big packet!\n");
		}
	}

	pancake_destroy(my_pancake_handle);

	return EXIT_SUCCESS;
}
