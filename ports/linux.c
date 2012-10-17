#include <pancake.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <netinet/ip6.h>

static PANCSTATUS linux_init_func(void *dev_data);
static PANCSTATUS linux_write_func(void *dev_data, uint8_t *data, uint16_t length);
static PANCSTATUS linux_destroy_func(void *dev_data);
static void linux_read_func(uint8_t *data, int16_t length);

struct pancake_dev_cfg linux_cfg = {
	.init_func = linux_init_func,
    .write_func = linux_write_func,
	.destroy_func = linux_destroy_func,
};

pthread_t my_thread;

static void populate_dummy_ipv6_header(struct ip6_hdr *hdr, uint16_t payload_length)
{
	/* Loopback (::1/128) */
	struct in6_addr addr = {
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 1};

	hdr->ip6_flow	=	htonl(6 << 28);
	hdr->ip6_plen	=	htons(payload_length);
	hdr->ip6_nxt	=	254;
	hdr->ip6_hops	=	2;
	memcpy(hdr+8, &addr, 16);
	memcpy(hdr+24, &addr, 16);
}

static void linux_read_thread(void *dev_data)
{
	uint8_t i, timeout = 1;
	uint8_t data[127];
	uint16_t length = 42;
	PANCSTATUS ret;
	FILE 			*out 		= (FILE *)dev_data;
	struct ip6_hdr 	*hdr 		= (struct ip6_hdr *)data;
	uint8_t			*payload	= data+40;

	/* Send 10 packets with 5 seconds delay */
	for (i=0; i < 10; i++) {
		*payload = i;
		*(payload+1) = 255-i;
		populate_dummy_ipv6_header(hdr, 2);
		sleep(timeout);
		fprintf(out, "linux.c: Patching incoming packet to pancake_process_data()\n");
		ret = pancake_process_data(dev_data, data, length);
		if (ret != PANCSTATUS_OK) {
			/* What to do, what to do? */
		}
	}
}

static PANCSTATUS linux_init_func(void *dev_data)
{
	pthread_create (&my_thread, NULL, (void *) &linux_read_thread, dev_data);
	return PANCSTATUS_OK;
}

static PANCSTATUS linux_write_func(void *dev_data, uint8_t *data, uint16_t length)
{
	size_t ret;
	uint8_t bit;
	uint16_t i;
	uint8_t j;
	FILE *out = (FILE*)dev_data;

	fputs("linux.c: Transmitting the following packet to the ether:\n", out);
	fputs("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n", out);
	for (i=0; i < length; i++) {
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
	if (length % 4 != 0) {
		fputs("|\n+", out);
		for (i=0; i < length % 4; i++) {
			fputs("-+-+-+-+-+-+-+-+", out);
		}
		fputs("\n", out);
	}

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}

static PANCSTATUS linux_destroy_func(void *dev_data)
{
	pthread_join(my_thread, NULL);
	return PANCSTATUS_OK;
}

