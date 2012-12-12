#ifndef PANCAKE-UDP_H_INCLUDED
#define PANCAKE-UDP_H_INCLUDED

PANCSTATUS pancake_send_udp(PANCHANDLE handle, struct in6_addr *destination_address, uint16_t destination_port, uint8_t *payload, uint16_t payload_length);


/* UDP packet */
struct pancake_udp_packet {
	uint16_t source_port;
	uint16_t destination_port;
	uint16_t length;
	uint16_t checksum;
	uint8_t *payload;
};
#endif // PANCAKE-UDP_H_INCLUDED
