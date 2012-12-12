#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <pancake.h>
#include <net/pancake-udp.h>

/**
 * Calculates checksum
 */
static uint16_t checksum(uint16_t *data, uint8_t length)
{
	uint8_t i;
	uint8_t carry = 0;
	uint32_t sum = 0;
	
	// Sum all values
	for(i = 0; i < length; i += 1) {
		sum += data[i];
	}

	/* Can only overflow twice */
	for (i = 0; i < 2; i += 1) {
		// Add carry bits
		carry = (sum >> 16);
		// Clear carry bits
		sum &= 0xffff;
	
		sum += carry;
	}
	
	return (uint16_t) sum;
}

/**
 * Send packet
 */
PANCSTATUS pancake_send_udp_test(PANCHANDLE handle, struct in6_addr *destination_address, uint8_t *payload, uint16_t payload_length) 
{
	printf("%s", "Lol i bollen");
}

/**
 * Recieve UDP-packet + Verify
 */
PANCSTATUS pancake_recieve_udp(struct ip6_hdr *ip6_header, struct pancake_udp_packet *udp_packet, uint8_t *udp_packet_data, uint16_t data_length)
{	
	uint8_t pseudo_header_length = 27 + data_length;
	uint16_t pseudo_header[pseudo_header_length]; 
	uint16_t calculated_checksum;
	uint8_t i;
	
	// Source port
	udp_packet->source_port = (uint16_t) *udp_packet_data << 8 | *(udp_packet_data + 1);
	
	// Destination port
	udp_packet->destination_port = (uint16_t) *(udp_packet_data + 2) << 8 | *(udp_packet_data + 3);
	
	// Length of packet
	udp_packet->length = (uint16_t) *(udp_packet_data + 4) << 8 | *(udp_packet_data + 5);
	
	// Checksum
	udp_packet->checksum = (uint16_t) *(udp_packet_data + 6) << 8 | *(udp_packet_data + 7);
	
	/*
	 * Pseudo header
	 */
	
	// Copy source address
	for(i = 0; i < 8; i += 1) {
		pseudo_header[i] = ip6_header->ip6_src.s6_addr[i]; //(source_address.s6_addr[i] << 8) | source_address.s6_addr[i+1];
	}
	
	// Copy destination address
	for(i = 0; i < 8; i += 1) {
		pseudo_header[i + 8] = ip6_header->ip6_dst.s6_addr[i]; //(destination_address->s6_addr[i] << 8) | destination_address->s6_addr[i+1];
	}
	
	// UDP length
	pseudo_header[16] = (data_length >> 24);
	pseudo_header[17] = (data_length >> 16);
	pseudo_header[18] = (data_length >> 8);
	pseudo_header[19] = (data_length >> 0);
	
	// Zeros
	pseudo_header[20] = 0;
	pseudo_header[22] = 0;
	pseudo_header[22] = 0;
	
	// Next header 
	pseudo_header[23] = ip6_header->ip6_nxt; // The protocol value for UDP: 17
	
	/*
	 * UDP packet
	 */
	// Source port
	pseudo_header[24] = udp_packet->source_port;
	
	// Destination port
	pseudo_header[25] =  udp_packet->destination_port;
	
	// Length
	pseudo_header[26] =  udp_packet->length;
	
	// Checksum (zeroed)
	pseudo_header[27] = 0;
	
	// Payload
	for(i = 8; i < data_length; i += 1) {
		pseudo_header[20 + i] = udp_packet_data[i];
	}
	
	// Calculate checksum
	calculated_checksum = checksum(&pseudo_header, pseudo_header_length);
	
	// Verify checksum
	if(calculated_checksum != udp_packet -> checksum){
		goto err_out;
	}
	
	return PANCSTATUS_OK;
	
err_out:
	return PANCSTATUS_ERR;
}

/**
 * Send UDP-packet
 * @param handle Pancake handle
 * @param destination_address IP6 address to destination
 * @param destination_port Port that destination wants to receve UDP-packets on
 * @param payload The payload to send in the UDP-packet
 * @param payload_length Size of payload in uint8_t bytes
 */
PANCSTATUS pancake_send_udp(PANCHANDLE handle, struct in6_addr *destination_address, uint16_t destination_port, uint8_t *payload, uint16_t payload_length)
{
	struct in6_addr source_address = {
		0, 0, 0, 0, 
		0, 0, 1, 2,
		3, 0, 0, 0,
		0, 0, 0, 1,
	};
	
	// Psedo header for checksum calculation
	uint8_t pseudo_header_length = 27 + payload_length; /* 20 */
	uint16_t pseudo_header[pseudo_header_length]; 
	uint32_t udp_length = 2 + 2 + 2 + 2 + payload_length;
	uint16_t source_port = 1337;
	uint8_t packet_payload[udp_length];
	uint8_t i;
	uint8_t *udp_header_pointer;
	PANCSTATUS ret;
	
	struct pancake_udp_packet udp_header = {
		.source_port = source_port,
		.destination_port = destination_port,
		.length = udp_length,
	};
	
	struct ip6_hdr ip6_header = {
		.ip6_dst = destination_address,
		.ip6_dst = source_address,
		.ip6_ctlun.ip6_un1 = {
			.ip6_un1_flow = 0,
			.ip6_un1_plen = udp_length,
			.ip6_un1_nxt = 17, /* UDP */
			.ip6_un1_hlim = 2,
		},
	};
	
	/*
	 * Pseudo header
	 */
	
	// Copy source address
	for(i = 0; i < 8; i += 1) {
		pseudo_header[i] = source_address.s6_addr[i]; //(source_address.s6_addr[i] << 8) | source_address.s6_addr[i+1];
	}
	
	// Copy destination address
	for(i = 0; i < 8; i += 1) {
		pseudo_header[i + 8] = destination_address->s6_addr[i]; //(destination_address->s6_addr[i] << 8) | destination_address->s6_addr[i+1];
	}
	
	// UDP length
	pseudo_header[16] = (udp_length >> 24);
	pseudo_header[17] = (udp_length >> 16);
	pseudo_header[18] = (udp_length >> 8);
	pseudo_header[19] = (udp_length >> 0);
	
	// Zeros
	pseudo_header[20] = 0;
	pseudo_header[22] = 0;
	pseudo_header[22] = 0;
	
	// Next header 
	pseudo_header[23] = ip6_header.ip6_nxt; // The protocol value for UDP: 17
	
	/*
	 * UDP packet
	 */
	// Source port
	pseudo_header[24] = source_port;
	
	// Destination port
	pseudo_header[25] = destination_port;
	
	// Length
	pseudo_header[26] = udp_length;
	
	// Checksum (zeroed)
	pseudo_header[27] = 0;
	
	// Payload
	for(i = 0; i < payload_length; i += 1) {
		pseudo_header[28 + i] = payload[i];
	}
	
	/*
	 * Set checksum to UDP packet
	 */
	udp_header.checksum = checksum(&pseudo_header, pseudo_header_length);
	
	// UDP-header to packet_payload
	udp_header_pointer = &udp_header;
	for(i = 0; i < 8; i += 1) {
		packet_payload[i] = (uint8_t) *udp_header_pointer;
		udp_header_pointer++;
	}
	
	// Copy Payload
	for(i = 0; i < payload_length; i += 1) {
		packet_payload[8 + i] = payload[i];
	}
	
	#if 1
	/* Print packet */
	printf("Source port: %d\n", udp_header.source_port);
	printf("Destination port: %d\n", udp_header.destination_port);
	printf("Checksum: %d\n", udp_header.checksum);
	printf("UDP-packet length: %d\n", udp_header.length);
	
	/* Print  UDP-payload */
	printf("UDP-payload: ");
	for(i = 8; i < udp_length; i += 1) {
		printf("%d, ", packet_payload[i]);
	}
	printf("\n");
	#endif;
	
	/*
	 * Send normal packet
	 */
	ret = pancake_send(handle, &ip6_header, &packet_payload, (uint16_t) udp_length);
	
	if(ret != PANCSTATUS_OK) {
		goto err_out;
	}
	
	return PANCSTATUS_OK;
	
err_out:
	return PANCSTATUS_ERR;
}
