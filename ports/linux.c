#include <pancake.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#ifdef _WIN32
    #include <windows.h>
    #include <memory.h>
#else
    #include <pthread.h>
#endif
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <helpers.h>

static PANCSTATUS linux_init_func(void *dev_data);
static PANCSTATUS linux_write_func(void *dev_data, struct pancake_ieee_addr *dest, uint8_t *data, uint16_t length);
static PANCSTATUS linux_destroy_func(void *dev_data);
static void linux_read_func(uint8_t *data, int16_t length);

struct pancake_port_cfg linux_cfg = {
	.init_func = linux_init_func,
    .write_func = linux_write_func,
	.destroy_func = linux_destroy_func,
};

#if PANC_HAVE_PRINTF != 0
#define pancake_fprintf(...) fprintf(__VA_ARGS__)
#else
#define pancake_fprintf(...) {}
#endif

void pancake_print_raw_bits(FILE *out, uint8_t *bytes, size_t length)
{
#if PANC_HAVE_PRINTF != 0
	uint8_t bit;
	uint16_t i;
	uint8_t j;

	if (bytes == NULL) {
		return;
	}

	if (out == NULL) {
		out = stdout;
	}

	fputs("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n", out);
	for (i=0; i < length; i++) {
		fprintf(out, "|");
		for (j=0; j < 8; j++) {
			bit = ( (*bytes) >> (7-j%8) ) & 0x01;
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
		bytes++;
	}
	if (length % 4 != 0) {
		fputs("|\n+", out);
		for (i=0; i < length % 4; i++) {
			fputs("-+-+-+-+-+-+-+-+", out);
		}
		fputs("\n", out);
	}
#endif
}

static void populate_dummy_ipv6_header(struct ip6_hdr *hdr, uint16_t payload_length)
{
	/* Loopback (::1/128) */
	struct in6_addr addr = {
			0xfe, 0x80, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0xff, 0xfe, 0, 0, 1};

	// Version + Traffic Control [ECN(2) + DSCP(6)] + Flow id 26
	hdr->ip6_flow	=	htonl((6 << 28) | (0x1 << 26) | (26 << 0));
	hdr->ip6_plen	=	htons(payload_length);
	hdr->ip6_nxt	=	254;
	hdr->ip6_hops	=	2;

    // Add next bytes
	memcpy((uint8_t *)hdr + 8, &addr, 16);
	memcpy((uint8_t *)hdr + 24, &addr, 16);
}

#ifdef _WIN32
HANDLE win_thread;
#else // Linux
pthread_t my_thread;
#endif

static void linux_read_thread(void *dev_data)
{
	uint8_t i, timeout = 1;
	uint8_t data[127*3];
	uint16_t length = 1 + 40 + 2;
	PANCSTATUS ret;
	FILE 			*out 		= (FILE *)dev_data;
	struct ip6_hdr 	*hdr 		= (struct ip6_hdr *)(data+1);
	uint8_t			*payload	= data + 1 + 40;

	/* Raw IPv6 packet dispatch value */
	data[0] = 0x41;

#if 1
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
		pancake_fprintf(out, "linux.c: Patching incoming packet to pancake_process_data()\n");
		ret = pancake_process_data(dev_data, NULL, NULL, data, length);
		if (ret != PANCSTATUS_OK) {
			/* What to do, what to do? */
		}
	}
#endif

#if 1
	/* Send 1 big packet */
	for (i=0; i < 200; i++) {
		*payload++ = (uint8_t)i;
	}
	populate_dummy_ipv6_header(hdr, 200);
	length = 200 + 1 + 40;
	pancake_fprintf(out, "linux.c: Patching incoming packet to pancake_process_data()\n");
	ret = pancake_process_data(dev_data, NULL, NULL, data, length);
	if (ret != PANCSTATUS_OK) {
		/* What to do, what to do? */
	}
#endif
}

static PANCSTATUS linux_init_func(void *dev_data)
{
    #ifdef _WIN32
	win_thread = CreateThread(NULL, 0, &linux_read_thread, dev_data, 0, NULL);
	#else // Linux
	pthread_create (&my_thread, NULL, (void *) &linux_read_thread, dev_data);
	#endif
	return PANCSTATUS_OK;
}

static PANCSTATUS linux_write_func(void *dev_data, struct pancake_ieee_addr *dest, uint8_t *data, uint16_t length)
{
	size_t ret;
	uint8_t bit;
	uint16_t i;
	uint8_t j;
	FILE *out = (FILE*)dev_data;

	fputs("linux.c: Transmitting the following packet to the ether:\n", out);
	pancake_print_raw_bits(out, data, length);

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}

static PANCSTATUS linux_destroy_func(void *dev_data)
{
    #ifdef _WIN32
    WaitForSingleObject(win_thread, INFINITE);
	#else // Linux
	pthread_join(my_thread, NULL);
	#endif

	return PANCSTATUS_OK;
}

