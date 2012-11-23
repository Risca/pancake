#ifndef HELPERS_H
#define HELPERS_H
#if PANC_HELPERS_ENABLED != 0
#include <stdio.h>

/* Color change struct */
typedef enum {
	PANC_COLOR_RED = 0,
	PANC_COLOR_GREEN = 1,
	PANC_COLOR_YELLOW,
	PANC_COLOR_BLUE,
	PANC_COLOR_WHITE,
} PANC_COLOR;

struct color_change {
	PANC_COLOR color;
	uint8_t position;
	char* description;
};

void pancake_print_raw_bits(FILE *out, uint8_t *bytes, size_t length);
void pancake_pretty_print(FILE *out, uint8_t *bytes, size_t length, struct color_change *color_positions, uint8_t number_of_colors);
void populate_dummy_ipv6_header(struct ip6_hdr *hdr, uint16_t payload_length);


#endif
#endif
