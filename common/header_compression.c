#include <pancake.h>
#include <stdio.h>

uint8_t link_local_prefix[] = {0, 0, 0, 0, 0, 0, 0, 0}; // 64 bits

/**
 * Compresses an IPv6 header
 * This header should be smaller than the input header and atleast the same size
 */
PANCSTATUS pancake_compress_header(struct ip6_hdr *hdr, struct pancake_compressed_ip6_hdr *compressed_hdr)
{
    // Build header
    uint8_t *data = compressed_hdr->hdr_data;

    // IP v6 (first 3 bits)
    *data = (0x6 << 4);

    // Trafic class (trafic+flow elided)
    *data |= (0x3 << 3);

    // Next header (full 8-bits carried in-line)
    *data &= ~(0x1 << 2); // 0 on 5:th bit                                                 // REMEMBER Carry in-line

    // Hop limit
    switch(hdr->ip6_hlim) {
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
            *data &= ~(0x3 << 0);  // 00                                                    // REMEMBER Carry in-line
    }

    // Go to next byte in compressed header
    data++;

    // Context Identifier Extension (0)
    *data &= ~(1 << 7);

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
    data++;     // Byte 4
    *data = hdr->ip6_hlim;
    compressed_hdr->size += 1;

    //Source address
    data++;     // Byte 5
    *data = hdr->ip6_src.s6_addr[14];
    data++;
    *data = hdr->ip6_src.s6_addr[15];
    compressed_hdr->size += 2;

    // Destination address
    data++;     //Byte 6
    *data = hdr->ip6_dst.s6_addr[14];
    data++;     // Byte 7
    *data = hdr->ip6_dst.s6_addr[15];
    compressed_hdr->size += 2;

    return PANCSTATUS_OK;
}

/**
 * Header decompression
 */
PANCSTATUS pancake_decompress_header(struct pancake_compressed_ip6_hdr *compressed_hdr, struct ip6_hdr *hdr)
{
    // Build header
    uint8_t *data = compressed_hdr->hdr_data;
    uint8_t *inline_data = data + 2;	// Inline data starts att byte 3
    uint8_t ip_len = 16;
    uint8_t i;

    // Check first 3 bits, should be (011)
    if ((*data & (0x7 << 5)) != (0x6 << 4)) {
        printf("First three bits 0x%x is not an 6LoWPAN header\n", *data & (0x7 << 5));
        return PANCSTATUS_ERR;
    }

    // Check flow label
    switch(*data & (0x3 << 3)) {
        // ECN + DSCP + 4-bit Pad + Flow Label (4 bytes)					// NOT IMPLEMENTED
        case (0x0 << 3):
            return PANCSTATUS_ERR;
            break;

        //ECN + 2-bit Pad + Flow Label (3 bytes), DSCP is elided.			// NOT IMPLEMENTED
        case (0x1 << 3):
            return PANCSTATUS_ERR;
            break;

        // ECN + DSCP (1 byte), Flow Label is elided.						// NOT IMPLEMENTED
        case (0x2 << 3):
            return PANCSTATUS_ERR;
            break;

        // Traffic Class and Flow Label are elided.
        case (0x3 << 3):

            hdr->ip6_flow = (uint32_t) 0;
            break;
    }

    // Next header
    if((*data & (0x1 << 2)) == 0) {											// REMEMBER: Check if flow is carried in line
		// Full 8 bits carried in line
		hdr->ip6_nxt = *(data + 2); // Byte 3
    }
    else {
		// Next header is compressed using LOWPAN_NHC						// NOT IMPLEMENTED
		return PANCSTATUS_ERR;
    }

    // Hop limit
    switch(*data & (0x3 << 0)) {
        // The Hop Limit field is carried in-line.
    	case 0x0:
            hdr->ip6_hlim = (uint8_t) *(data + 3);
            break;

        // The Hop Limit field is compressed and the hop limit is 1.
    	case 0x1:
           hdr->ip6_hlim = (uint8_t) 1;
            break;

        // The Hop Limit field is compressed and the hop limit is 64.
        case 0x2:
            hdr->ip6_hlim = (uint8_t) 64;
            break;

        // The Hop Limit field is compressed and the hop limit is 255.
        case 0x3:
            hdr->ip6_hlim = (uint8_t) 255;
            break;
	}
	
	// Next byte (byte 2)
	data++;

	// Context Identifier Extension (if 1)
	if ((*data & (0x1 < 7)) == (0x1 << 7)) {
		// An additional 8-bit Context Identifier Extension field
		// immediately follows the Destination Address Mode (DAM) field.
		return PANCSTATUS_ERR;
	}
	
	// Source Address Compression 
	if ((*data & (0x1 << 6)) == (0x0 << 6)) {
		
		// Source Address Mode (SAC=0)
		switch(*data & (0x3 << 4)) {
			// 128 bits. The full address is carried in-line
			case (0x0 << 4):
				return PANCSTATUS_ERR;
				break;
			
			// 64 bits. The first 64-bits of the address are elided.
            // The value of those bits is the link-local prefix padded with
            // zeros.  The remaining 64 bits are carried in-line.
			case (0x1 << 4):
				return PANCSTATUS_ERR;
				break;
				
			// 16 bits. The first 112 bits of the address are elided.
            // The value of the first 64 bits is the link-local prefix
            // padded with zeros.  The following 64 bits are 0000:00ff:
            // fe00:XXXX, where XXXX are the 16 bits carried in-line.
			case (0x2 << 4):
			
				// First 64 bits from link-local prefix
				for(i = 0; i < 8; i += 1) {
					hdr->ip6_src.s6_addr[i] = link_local_prefix[i];
				}
				
				hdr->ip6_src.s6_addr[8] = (uint8_t) 0x0;
				hdr->ip6_src.s6_addr[9] = (uint8_t) 0x0;
				hdr->ip6_src.s6_addr[10] = (uint8_t) 0x0;
				hdr->ip6_src.s6_addr[11] = (uint8_t) 0xff;
				hdr->ip6_src.s6_addr[12] = (uint8_t) 0xfe;
				hdr->ip6_src.s6_addr[13] = (uint8_t) 0x0;
				
				// 16 bit source adress
				hdr->ip6_src.s6_addr[14] = (uint8_t) *(inline_data + 2);
				hdr->ip6_src.s6_addr[15] = (uint8_t) *(inline_data + 3);
				break;
				
			// 0 bits. The address is fully elided.  The first 64 bits
            // of the address are the link-local prefix padded with zeros.
            // The remaining 64 bits are computed from the encapsulating
            // header (e.g., 802.15.4 or IPv6 source address) as specified
            // in Section 3.2.2.
			case (0x3 << 4):
				return PANCSTATUS_ERR;
				break;
		}
	} 
	else {
		// uses context-based compression
		return PANCSTATUS_ERR;
	}
	
	// Destination is Multicast adress
	if ((*data & (0x1 << 3) == 0x0) {
		// Destination is not multicast address
	}
	else {
		// Destination is a multicast adress
	}

	return PANCSTATUS_OK;
}

/**
 * Header diff check
 * Checks if headers are the same
 */
PANCSTATUS pancake_diff_header(struct ip6_hdr *origin_hdr, struct ip6_hdr *decompressed_hdr)
{
    uint8_t *ip6_src_origin;
    uint8_t *ip6_src_decomp;
    uint8_t *ip6_dst_origin;
    uint8_t *ip6_dst_decomp;
    uint8_t ip_len = 16;
    uint8_t i = 0;
    PANCSTATUS status = PANCSTATUS_OK;

    printf("%s", "Diff headers origin/decompressed\n");

    // Check flow bits
    if (origin_hdr->ip6_flow != decompressed_hdr->ip6_flow) {
        printf("Flow ID not equal, origin: %x, decompressed: %x\n", origin_hdr->ip6_flow, decompressed_hdr->ip6_flow);
        status = PANCSTATUS_ERR;
    }

    // Check payload length
    if (origin_hdr->ip6_plen != decompressed_hdr->ip6_plen) {
        printf("Payload length not equal, origin: %x, decompressed: %x\n", origin_hdr->ip6_plen, decompressed_hdr->ip6_plen);
        status = PANCSTATUS_ERR;
    }

    // Check next header
    if (origin_hdr->ip6_nxt != decompressed_hdr->ip6_nxt) {
        printf("Next header not equal, origin: %x, decompressed: %x\n", origin_hdr->ip6_nxt, decompressed_hdr->ip6_nxt);
        status = PANCSTATUS_ERR;
    }

    // Check hop limit
    if (origin_hdr->ip6_hlim != decompressed_hdr->ip6_hlim) {
        printf("Hop limit not equal, origin: %x, decompressed: %x\n", origin_hdr->ip6_hlim, decompressed_hdr->ip6_hlim);
        status = PANCSTATUS_ERR;
    }

    // Check source address
    ip6_src_origin = origin_hdr->ip6_src.s6_addr;
    ip6_src_decomp = decompressed_hdr->ip6_src.s6_addr;

    for(i = 0; i < ip_len; i += 1) {
        if (ip6_src_origin[i] != ip6_src_decomp[i]) {
            printf("Source adress not equal, error at byte %u\n", i + 1);

            // Print addresses
            pancake_print_addr(&origin_hdr->ip6_src);
            printf("%s", "\n");
            pancake_print_addr(&decompressed_hdr->ip6_src);
            printf("%s", "\n");

            pancake_addr_error_line(i);

            status = PANCSTATUS_ERR;
            break;
        }
    }

    // Check Destination address
    ip6_dst_origin = origin_hdr->ip6_dst.s6_addr;
    ip6_dst_decomp = decompressed_hdr->ip6_dst.s6_addr;

    for(i = 0; i < ip_len; i += 1) {
        if (ip6_dst_origin[i] != ip6_dst_decomp[i]) {
            printf("Destination adress not equal, error at byte %u\n", i + 1);

            // Print addresses
            pancake_print_addr(&origin_hdr->ip6_dst);
            printf("%s", "\n");
            pancake_print_addr(&decompressed_hdr->ip6_dst);
            printf("%s", "\n");

            pancake_addr_error_line(i);

            status = PANCSTATUS_ERR;
            break;
        }
    }

    return status;
}
