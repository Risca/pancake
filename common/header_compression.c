#include <pancake.h>
#include <stdio.h>

/**
 * Compresses an IPv6 header
 * This header should be smaller than the input header and atleast the same size
 */
PANCSTATUS pancake_compress_header(struct ip6_hdr *hdr, struct pancake_compressed_ip6_hdr *compressed_hdr)
{
    // Build header
    uint8_t *data = compressed_hdr->hdr_data;
    uint8_t trafic_class = (*data << 3) | (*(data+1) << 3);

    // IP v6 (first 3 bits)
    *data = (0x6 << 4);

    // Trafic class (elided)
    *data |= (0x3 << 3);

    // Next header (full 8-bits carried in-line)
    *data &= ~(1 << 2); // 0 on 6:th bit                                                 // REMEMBER Carry in-line

    // Hop limit
    switch(hdr->ip6_hops) {
        case 255:
            *data |= 0x3;   // 11
            break;
        case 64:
            *data |= (1 << 1);
            *data &= ~(1 << 0);  // 10
            break;
        case 1:
            *data &= ~(1 << 1);
            *data |= (1 << 0);   // 01
            break;

        // Carried in-line
        default:
            *data &= ~(11 << 0);  // 00                                                   // REMEMBER Carry in-line
    }

    // Go to next byte in compressed header
    data++;

    // Context Identifier Extension (1)
    *data |= (1 << 7);

    // Source Address Compression
    *data &= ~(1 << 6);

    // 2 Bytes in size
    compressed_hdr->size = 2;
}
