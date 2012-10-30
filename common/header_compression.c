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
            *data &= ~(11 << 0);  // 00                                                    // REMEMBER Carry in-line
    }

    // Go to next byte in compressed header
    data++;

    // Context Identifier Extension (1)
    *data |= (1 << 7);

    // Source Address Compression SAC
    *data &= ~(1 << 6); // Set SAC=0

    // Source adress mode (16 bits address carried in line)
    *data |= (1 << 5);  // Set bit 5=1
    *data &= ~(1 << 4); // Clear bit 4, -> 10                                              // REMEMBER Carry in-line

    // Multicast Compression (destination is NOT an Multicast address) (M=0)
    *data &= ~(1 << 3);

    // Destination Address Compression DAC (DAC=0)
    *data &= ~(1 << 2); // Set DAC=0

    // Destination Address Mode (16 bits address carried in line                           // REMEMBER Carry in-line
    *data |= (1 << 1);  // Set bit 1=1
    *data &= ~(1 << 0); // Clear bit 0, -> 10

    // 2 Bytes in size
    compressed_hdr->size = 2;

    /*
     * Data carried in-line
     */
    // Next header
    data++;     // Byte 3
    *data = hdr->ip6_nxt;
    compressed_hdr->size += 1;

    // Hop limit
    data++;
    *data = hdr->ip6_hlim;
    compressed_hdr->size += 1;

    //Source address
    data++;
    *data = hdr->ip6_src.s6_addr[14];
    data++;
    *data = hdr->ip6_src.s6_addr[15];
    compressed_hdr->size += 2;

    // Destination address
    data++;
    *data = hdr->ip6_dst.s6_addr[14];
    data++;
    *data = hdr->ip6_dst.s6_addr[15];
    compressed_hdr->size += 2;
}

/**
 *
 */
