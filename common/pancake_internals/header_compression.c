#include <pancake.h>
#include <stdio.h>
#include <in6_addr.h>

#ifdef _WIN32
    #include <windows.h>
    #include <memory.h>
#endif
#ifdef __linux__
    #include <pthread.h>
#endif

/**
 * Compress ipv6 address
 * @returns length of inline data in bytes
 */
static uint8_t compress_address(struct in6_addr *address, uint8_t *inline_data) 
{
	uint8_t i;
	uint8_t foo = 0;

    // Check if 64 first bits are LINK_LOCAL_PREFIX
	for (i = 0; i < 8; i++) {
		if (LINK_LOCAL_PREFIX[i] != address->s6_addr[i]) {
			foo = 1;
		}
	}
	//	i = memcmp(LINK_LOCAL_PREFIX, address.s6_addr, 8);
	if (foo != 0) {
		// Copy full adress to inline data
		memcpy(inline_data, address->s6_addr, 16);
		return 16;
	}

	// Check if next bits are same as below
	if(
		address->s6_addr[8] == 0x00 ||
		address->s6_addr[9] == 0x00 ||
		address->s6_addr[10] == 0x00 ||
		address->s6_addr[11] == 0xff ||
		address->s6_addr[12] == 0xfe ||
		address->s6_addr[13] == 0x00
	){
		// Destination Address Mode (16 bits address carried in line	// TODO, check if full address can be eligned
		//*data |= (1 << 1);  // Set bit 1=1
		//*data &= ~(1 << 0); // Clear bit 0, -> 10

		// Destination address
		*inline_data = address->s6_addr[14];
		*(inline_data + 1) = address->s6_addr[15];
		return 2;
	}
	else {
		/* We assume right-most 64 bits are EUI-64 addr */
		return 0;
	}

}
/**
 * Compresses an IPv6 header
 * This header should be smaller than the input header and atleast the same size
 */
PANCSTATUS pancake_compress_header(struct ip6_hdr *hdr, struct pancake_compressed_ip6_hdr *compressed_hdr)
{
    // Build header
    uint8_t *data = compressed_hdr->hdr_data;
    uint8_t *inline_data = (data + 2);
    uint8_t traffic_class = ntohl(hdr->ip6_flow) >> 20; 	// bits 20-27
#if 0
    uint8_t i;
    uint8_t ip_len = 16;
#endif
    uint8_t ret;
    
    // Header is 2 Bytes in size
    compressed_hdr->size = 2;

    // IP v6 (first 3 bits, 011)
    *data = (0x6 << 4);
    
    // Check traffic class
    switch(traffic_class & (0x3 << 6)){
		// ECN disabled
		case (0x0 << 6):
			// TODO, check if DSP is used
			
			// Traffic Class and Flow Label are elided.
			*data |= (0x3 << 3);
			break;
		
		// ECN is enabled
		case (0x1 << 6):
		case (0x2 << 6):
		case (0x3 << 6):
			// ECN + 2-bit Pad + Flow Label (3 bytes), DSCP is elided.				// TODO check for DSCP 
			*data &= ~(0x2 << 3);
			*data |= (0x1 << 3);	// 01
			
			// Put data inline
			*inline_data = (uint8_t) (ntohl(hdr->ip6_flow) >> 16);
			
			// Set ECN bits
			*inline_data |= (traffic_class & ((uint32_t)0x03 << 6)) | 0x3f; // Mask and set all bits
			*inline_data &= (traffic_class & ((uint32_t)0x03 << 6)) & 0xc0; // Mask and clear all bits
			
			
			// Next byte (flow label)
			*inline_data &= 0xf0; // clear (4 bits)
			*inline_data |= (uint8_t) (ntohl(hdr->ip6_flow) >> 12);
			inline_data++;
			
			*inline_data = (uint8_t) (ntohl(hdr->ip6_flow) >> 8);
			inline_data++;
			
			// Next byte (flow label)
			*inline_data = (uint8_t) ntohl(hdr->ip6_flow);
			inline_data++;
			
			compressed_hdr->size += 3;
			break;
	}

    // Next header (full 8-bits carried in-line)
    *data &= ~(0x1 << 2); // 0 on 5:th bit
    
    // Add inline data
    *inline_data = hdr->ip6_nxt; 
	inline_data++;
	compressed_hdr->size += 1;

    // Hop limit
    switch(hdr->ip6_hlim) {
        case 255:
            *data |= 0x3;   	// 11
            break;
        case 64:
            *data |= (1 << 1);
            *data &= ~(1 << 0);	// 10
            break;
        case 1:
            *data &= ~(1 << 1);
            *data |= (1 << 0);	// 01
            break;

        // Carried in-line
        default:
            *data &= ~(0x3 << 0);  // 00
            *inline_data = hdr->ip6_hlim;
			inline_data++;
			compressed_hdr->size += 1;
    }

    // Go to next byte in compressed header
    data++;

    // Context Identifier Extension (0)
    *data &= ~(1 << 7);

    // Source Address Compression SAC
    *data &= ~(1 << 6); // Set SAC=0
    
	// Source address
	ret = compress_address(&hdr->ip6_src, inline_data);
	inline_data += ret;
	compressed_hdr->size += ret;
	
	switch(ret) {
		case 16:
			*data &= ~(0x3 << 4); // Clear bits (SAM == 00)
			break;
			
		case 8: 
			*data &= ~(0x2 << 4);	// Clear bit 1
			*data |= (0x1 << 4);	// Set bit 0
			break;
			
		case 2:
			*data |= (1 << 5);  // Set bit 5=1
			*data &= ~(1 << 4); // Clear bit 4, -> 10
			break;
			
		case 0:
			*data |= (0x3 << 4); // Set SAM == 11
			break;
	}
    

	// Multicast Compression (destination is NOT an Multicast address) (M=0)
	*data &= ~(1 << 3);

	// Destination Address Compression DAC (DAC=0)
	*data &= ~(1 << 2); // Set DAC=0
    
	// Destination address
	ret = compress_address(&hdr->ip6_dst, inline_data);
	inline_data += ret;
	compressed_hdr->size += ret;
	
	switch(ret) {
		case 16:
			*data &= ~(0x3 << 0); // Clear bits
			break;
			
		case 8: 
			*data &= ~(0x2 << 0);	// Clear bit 1
			*data |= (0x1 << 0);	// Set bit 0
			break;
			
		case 2:
			*data |= (1 << 1);  // Set bit 1=1
			*data &= ~(1 << 0); // Clear bit 0, -> 10
			break;
		
		case 0:
			*data |= (0x3 << 4); // Set DAM == 11
			break;
	}

    return PANCSTATUS_OK;
#if 0
err_out:
	return PANCSTATUS_ERR;
#endif
not_implemented:
	printf("%s\n", "Header compression: Not implemented");
	return PANCSTATUS_ERR;
}

/**
 * Header decompression
 */
PANCSTATUS pancake_decompress_header(struct ip6_hdr *hdr, uint8_t * data, uint16_t data_length)
{
    // Where inline data starts
    uint8_t *inline_data = data + 2;	// byte 3
    uint8_t ip_len = 16;
    uint8_t hdr_length = 2;
    uint32_t flow = 0;
    uint8_t i;

	/* Sanity check */
	if (hdr == NULL || data == NULL || data_length == 0) {
		goto err_out;
	}

	/* Skip dispatch byte if present */
	if (data[0] == DISPATCH_IPHC) {
		data++;
		inline_data++;
		data_length--;
	}

    // Check first 3 bits, should be (011)
    if ((*data & (0x7 << 5)) != (0x6 << 4)) {
        printf("First three bits 0x%x is not an 6LoWPAN header\n", *data & (0x7 << 5));
        return PANCSTATUS_ERR;
    }

    // Check flow label
    switch(*data & (0x3 << 3)) {
        // ECN + DSCP + 4-bit Pad + Flow Label (4 bytes)					// NOT IMPLEMENTED
        case (0x0 << 3):
            goto not_implemented;
            break;

        //ECN + 2-bit Pad + Flow Label (3 bytes), DSCP is elided.			// NOT IMPLEMENTED
        case (0x1 << 3):
			
			// Set version back
			flow = ((uint32_t)6 << 28); // Version
			flow |= ((*inline_data & ((uint32_t)0x3 << 6)) << 20); // ECN 2 bits
			
			// Flow ID
			flow |= (*inline_data & ((uint32_t)0xf)) << 16;
			flow |= *(inline_data + 1) << 8;
			flow |= *(inline_data + 2);
			hdr->ip6_flow = htonl((uint32_t)flow);
			
			// Increment inline data
			inline_data += 3;
			hdr_length += 3;
        
            //return PANCSTATUS_ERR;
            break;

        // ECN + DSCP (1 byte), Flow Label is elided.						// NOT IMPLEMENTED
        case (0x2 << 3):
            goto not_implemented;
            break;

        // Traffic Class and Flow Label are elided.
        case (0x3 << 3):

            hdr->ip6_flow = (uint32_t) 0;
            break;
    }

    // Next header
    if((*data & (0x1 << 2)) == 0) {											// REMEMBER: Check if flow is carried in line
		// Full 8 bits carried in line
		hdr->ip6_nxt = *inline_data; // Byte 3
		inline_data++;
		hdr_length++;
    }
    else {
		// Next header is compressed using LOWPAN_NHC
		goto not_implemented;												// NOT IMPLEMENTED
    }

    // Hop limit
    switch(*data & (0x3 << 0)) {
		// The Hop Limit field is carried in-line.
		case 0x0:
			hdr->ip6_hlim = (uint8_t) *inline_data;
			inline_data++;
			hdr_length++;
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
		goto not_implemented;													// NOT IMPLEMENTED
	}
	
	// Source Address Compression 
	if ((*data & (0x1 << 6)) == (0x0 << 6)) {
		
		// Source Address Mode (SAC=0)
		switch(*data & (0x3 << 4)) {
			// 128 bits. The full address is carried in-line
			case (0x0 << 4):
				for(i = 0; i < ip_len; i += 1) {
					hdr->ip6_src.s6_addr[i] = *(inline_data + i);
				}
				inline_data += ip_len;
				hdr_length += ip_len;
				break;
			
			// 64 bits. The first 64-bits of the address are elided.
            // The value of those bits is the link-local prefix padded with
            // zeros.  The remaining 64 bits are carried in-line.
			case (0x1 << 4):
				
				// First 64 bits from link-local prefix
				for(i = 0; i < (ip_len/2); i += 1) {
					hdr->ip6_src.s6_addr[i] = LINK_LOCAL_PREFIX[i];
				}
				
				// Least 64 bits from inline data
				for(i = (ip_len/2); i < ip_len; i += 1) {
					hdr->ip6_src.s6_addr[i] = *inline_data;
					inline_data++;
					hdr_length++;
				}
				
				break;
				
			// 16 bits. The first 112 bits of the address are elided.
            // The value of the first 64 bits is the link-local prefix
            // padded with zeros.  The following 64 bits are 0000:00ff:
            // fe00:XXXX, where XXXX are the 16 bits carried in-line.
			case (0x2 << 4):
			
				// First 64 bits from link-local prefix
				memcpy(hdr->ip6_src.s6_addr, LINK_LOCAL_PREFIX, 8);
				
				// Static
				hdr->ip6_src.s6_addr[8] = (uint8_t) 0x0;
				hdr->ip6_src.s6_addr[9] = (uint8_t) 0x0;
				hdr->ip6_src.s6_addr[10] = (uint8_t) 0x0;
				hdr->ip6_src.s6_addr[11] = (uint8_t) 0xff;
				hdr->ip6_src.s6_addr[12] = (uint8_t) 0xfe;
				hdr->ip6_src.s6_addr[13] = (uint8_t) 0x0;
				
				// 16 bit source adress
				hdr->ip6_src.s6_addr[14] = (uint8_t) *inline_data;
				hdr->ip6_src.s6_addr[15] = (uint8_t) *(inline_data + 1);
				inline_data += 2;
				hdr_length += 2;
				break;
				
			// 0 bits. The address is fully elided.  The first 64 bits
            // of the address are the link-local prefix padded with zeros.
            // The remaining 64 bits are computed from the encapsulating
            // header (e.g., 802.15.4 or IPv6 source address) as specified
            // in Section 3.2.2.
			case (0x3 << 4):
				goto not_implemented;											// NOT IMPLEMENTED
				break;
		}
	} 
	else {
		// uses context-based compression
		goto not_implemented;													// NOT IMPLEMENTED
	}
	
	// Destination is Multicast adress
	if ((*data & (1 << 3)) == 0x0) {
		// Destination is not multicast address
		
		// Context Identifier Extension (if 1)
		if ((*data & (1 < 2)) == (0x1 << 2)) {
			// An additional 8-bit Context Identifier Extension field
			// immediately follows the Destination Address Mode (DAM) field.
			goto not_implemented;												// NOT IMPLEMENTED
		}
		
		// Destination Address Mode (Multicast=0)(DAC=0)
		switch(*data & (0x3 << 0)) {
			
			// 128 bits.  The full address is carried in-line.
			case 0x0:
				// Copy whole adress to inline data
				for(i=0; i < ip_len; i += 1) {
					hdr->ip6_dst.s6_addr[i] = *(inline_data + i);
				}
				inline_data += ip_len;
				hdr_length += ip_len;
				
				break;
			
			// 64 bits.  The first 64-bits of the address are elided.
            // The value of those bits is the link-local prefix padded with
            // zeros.  The remaining 64 bits are carried in-line.
			case 0x1:
				// First 64 bits from link-local prefix
				for(i = 0; i < (ip_len/2); i += 1) {
					hdr->ip6_dst.s6_addr[i] = LINK_LOCAL_PREFIX[i];
				}
				
				// Least 64 bits from inline data
				for(i = (ip_len/2); i < ip_len; i += 1) {
					hdr->ip6_dst.s6_addr[i] = *inline_data;
					inline_data++;
					hdr_length++;
				}
				break;
				
			// 16 bits.  The first 112 bits of the address are elided.
            // The value of the first 64 bits is the link-local prefix
            // padded with zeros.  The following 64 bits are 0000:00ff:
            // fe00:XXXX, where XXXX are the 16 bits carried in-line.
			case 0x2:
				// First 64 bits from link-local prefix
				for(i = 0; i < 8; i += 1) {
					hdr->ip6_dst.s6_addr[i] = LINK_LOCAL_PREFIX[i];
				}
				
				// Static
				hdr->ip6_dst.s6_addr[8] = (uint8_t) 0x0;
				hdr->ip6_dst.s6_addr[9] = (uint8_t) 0x0;
				hdr->ip6_dst.s6_addr[10] = (uint8_t) 0x0;
				hdr->ip6_dst.s6_addr[11] = (uint8_t) 0xff;
				hdr->ip6_dst.s6_addr[12] = (uint8_t) 0xfe;
				hdr->ip6_dst.s6_addr[13] = (uint8_t) 0x0;
				
				// 16 bit destination adress
				hdr->ip6_dst.s6_addr[14] = (uint8_t) *inline_data;
				hdr->ip6_dst.s6_addr[15] = (uint8_t) *(inline_data + 1);
				inline_data += 2;
				hdr_length += 2;
				break;
				
			// 0 bits.  The address is fully elided.  The first 64 bits
            // of the address are the link-local prefix padded with zeros.
            // The remaining 64 bits are computed from the encapsulating
            // header (e.g., 802.15.4 or IPv6 destination address) as
            // specified in Section 3.2.2.
			case 0x3:
				goto not_implemented;		 									// NOT IMPLEMENTED
				break;
		}
		
	}
	else {
		// Destination is a multicast adress
		goto err_out;															// NOT IMPLEMENTED
	}

	hdr->ip6_plen = htons(data_length-hdr_length);

	return PANCSTATUS_OK;
	
err_out:
	return PANCSTATUS_ERR;
	
not_implemented:
	printf("%s\n", "Header decompression: Not implemented");
	return PANCSTATUS_ERR; 
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

    // Check flow bits
    if (origin_hdr->ip6_flow != decompressed_hdr->ip6_flow) {
        printf("Flow ID not equal, origin: %x, decompressed: %x\n", ntohl(origin_hdr->ip6_flow), ntohl(decompressed_hdr->ip6_flow));
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

#if PANC_TESTS_ENABLED != 0
PANCSTATUS pancake_compression_test(PANCHANDLE handle)
{
	PANCSTATUS ret = PANCSTATUS_ERR;
	uint8_t i;
	int res;
	char * test_string = "Hello world!";
	uint8_t raw_data[1 + 40 + strlen(test_string)]; // 5 byte wiggle room
	uint8_t hdr_data[40];
	struct pancake_main_dev *dev = &devs[handle];
	struct ip6_hdr original_hdr, hdr;
	struct pancake_compressed_ip6_hdr compressed_hdr;
	compressed_hdr.hdr_data = hdr_data;

	populate_dummy_ipv6_header(&original_hdr, strlen(test_string));

	ret = pancake_compress_header(&original_hdr, &compressed_hdr);
	if (ret != PANCSTATUS_OK) {
		goto err_out;
	}

	raw_data[0] = DISPATCH_IPHC;
	memcpy(raw_data+1, compressed_hdr.hdr_data, compressed_hdr.size);

	ret = pancake_decompress_header(&hdr, raw_data, 1 + compressed_hdr.size + strlen(test_string));
	if (ret != PANCSTATUS_OK) {
		goto err_out;
	}

	res = memcmp(&original_hdr, &hdr, 40);
	if (res != 0) {
		pancake_diff_header(&original_hdr, &hdr);
		goto err_out;
	}

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}
#endif
