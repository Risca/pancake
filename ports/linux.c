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


#ifdef _WIN32
static WORD colors[] = {
	FOREGROUND_RED | FOREGROUND_INTENSITY,                    /* Red */
	FOREGROUND_GREEN | FOREGROUND_INTENSITY,                  /* Green */
	FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY, /* Yellow */
	FOREGROUND_BLUE | FOREGROUND_INTENSITY,                   /* Blue */
	FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN,      /* White */
};

void color_output(HANDLE hConsole, int COLOR) 
{
	SetConsoleTextAttribute(hConsole, colors[COLOR]);
}

#define FOREGROUND_CYAN FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define FOREGROUND_PINK FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY
#define FOREGROUND_YELLOW FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY

#define FOREGROUND_PURPLE FOREGROUND_BLUE | FOREGROUND_RED
#define FOREGROUND_TURCOSE FOREGROUND_BLUE | FOREGROUND_GREEN
#define FOREGROUND_DARK_YELLOW FOREGROUND_RED | FOREGROUND_GREEN

#else
static int colors[] = {
	31, /* Red */
	32, /* Green */
	33, /* Yellow */
	34, /* Blue */
	37, /* White */
};

void color_output(FILE *handle, PANC_COLOR color)
{
	fprintf(handle, "\033[1;%dm", colors[color]); 
}

#endif

void pancake_pretty_print(FILE *out, uint8_t *bytes, size_t length, struct color_change *color_positions, uint8_t number_of_colors)
{
#ifdef _WIN32
	HANDLE hConsole;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
#else
	FILE * hConsole = out;
#endif
	uint8_t bit;
	uint16_t i;
	uint8_t j;
	struct color_change *current_color_change = color_positions;
	
	if (out == NULL) {
		out = stdout;
	}
	
	// Output what the colors describe
	for(i=0; i<number_of_colors; i++) {
		color_output(hConsole, color_positions[i].color);
		pancake_fprintf(out, "%s\n", color_positions[i].description);
	}
	
	// White
	color_output(hConsole, PANC_COLOR_WHITE);
	
	// Print first line
	if(length >= 4) {
		pancake_fprintf(out, "+-----------+-----------+-----------+-----------+\n");
	}
	else {
		pancake_fprintf(out, "+");
		for (i=0; i < length % 4; i++) {
			pancake_fprintf(out, "-----------+");
		}
		pancake_fprintf(out, "\n");
	}
	
	for (i=0; i < length; i++) {
		// White
		color_output(hConsole, PANC_COLOR_WHITE);
		pancake_fprintf(out, "|");
		
		// Change color
		// TODO: Make this less dangerous
		while(i > 0 && current_color_change->position == i) {
			current_color_change++;
		}
		
		color_output(hConsole, current_color_change->color);
		
		for (j=0; j < 8; j++) {
			
			if (j == 0) {
				pancake_fprintf(out, " ");
			}
			
			bit = ( (*bytes) >> (7-j%8) ) & 0x01;
			pancake_fprintf(out, "%i", bit);

			/* Place a space between every bit except last */
			if (j == 3 || j==7) {
				pancake_fprintf(out, " ");
			}
		}

		if ( (i+1) % 4 == 0 ) {
			// White
			color_output(hConsole, PANC_COLOR_WHITE);
			pancake_fprintf(out, "|\n+-----------+-----------+-----------+-----------+\n");
		}
		bytes++;
	}
	
	if (length % 4 != 0) {
		// White
		color_output(hConsole, PANC_COLOR_WHITE);
		pancake_fprintf(out, "|\n+");
		for (i=0; i < length % 4; i++) {
			pancake_fprintf(out, "-----------+");
		}
		pancake_fprintf(out, "\n");
	}
	
	// Reset to white
	color_output(hConsole, PANC_COLOR_WHITE);
}

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

void populate_dummy_ipv6_header(struct ip6_hdr *hdr, uint16_t payload_length)
{
	/* Loopback (::1/128) */
	uint8_t identifier[2] = {0, 1};
	struct in6_addr addr;
	/*
	 = {
			0xfe, 0x80, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0xff, 0xfe, 0, 0, 1}*/
			
	pancake_get_in6_address(LINK_LOCAL_PREFIX, &identifier, 16, &addr);

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
#if PANC_DEMO_TWO == 0
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
#endif /* Whole function */
}

static PANCSTATUS linux_init_func(void *dev_data)
{
#ifdef _WIN32
	color_output(GetStdHandle(STD_OUTPUT_HANDLE), PANC_COLOR_WHITE);
	win_thread = CreateThread(NULL, 0, &linux_read_thread, dev_data, 0, NULL);
#else // Linux
	pthread_create (&my_thread, NULL, (void *) &linux_read_thread, dev_data);
	color_output(stdout, PANC_COLOR_WHITE);
#endif
	return PANCSTATUS_OK;
}

static PANCSTATUS linux_write_func(void *dev_data, struct pancake_ieee_addr *dest, uint8_t *data, uint16_t length)
{
#if PANC_DEMO_TWO == 0
	FILE *out = (FILE*)dev_data;

	fputs("linux.c: Transmitting the following packet to the ether:\n", out);
	pancake_print_raw_bits(out, data, length);
#endif
	return PANCSTATUS_OK;
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

