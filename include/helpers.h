#ifndef HELPERS_H
#define HELPERS_H
#if PANC_HELPERS_ENABLED != 0
#include <stdio.h>

void pancake_print_raw_bits(FILE *out, uint8_t *bytes, size_t length);
void pancake_pretty_print(FILE *out, uint8_t *bytes, size_t length, struct color_change *color_positions, uint8_t number_of_colors);
void populate_dummy_ipv6_header(struct ip6_hdr *hdr, uint16_t payload_length);


#endif
#endif
