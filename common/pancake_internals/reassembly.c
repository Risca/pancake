#include <stdio.h>

#if PANC_TESTS_ENABLED != 0
static uint8_t fragmented_packet[3][102];

void populate_fragmented_packets()
{
	uint8_t i;
	uint8_t payload = 0;
	struct pancake_frag_hdr *frag_hdr;
	struct ip6_hdr *ip6;
	
	/* Fill in fragmentation headers */
	frag_hdr = (struct pancake_frag_hdr *) &fragmented_packet[0][0];
	frag_hdr->size = htons(240 | (DISPATCH_FRAG1 << 8));
	frag_hdr->tag = htons(1);

	frag_hdr = (struct pancake_frag_hdr *) &fragmented_packet[1][0];
	frag_hdr->size = htons(240 | (DISPATCH_FRAGN << 8));
	frag_hdr->tag = htons(1);
	frag_hdr->offset = 0x61;

	frag_hdr = (struct pancake_frag_hdr *) &fragmented_packet[2][0];
	frag_hdr->size = htons(240 | (DISPATCH_FRAGN << 8));
	frag_hdr->tag = htons(1);
	frag_hdr->offset = 0xC2;

	/* Fill in IPv6 header */
	fragmented_packet[0][4] = DISPATCH_IPv6;
	ip6 = (struct ip6_hdr *) &fragmented_packet[0][5];
	ip6->ip6_flow = htonl(6 << 28);
	ip6->ip6_plen = htons(255);
	ip6->ip6_nxt  = 254;
	ip6->ip6_hops = 2;
	for (i = 0; i < 15; i++) {
		ip6->ip6_src.s6_addr[i] = 0;
	}
	ip6->ip6_src.s6_addr[i] = 1;
	for (i = 0; i < 15; i++) {
		ip6->ip6_dst.s6_addr[i] = 0;
	}
	ip6->ip6_dst.s6_addr[i] = 1;

	/* Fill in payload */
	for (i = 45; i < 102; i++) {
		fragmented_packet[0][i] = payload++;
	}
	for (i = 5; i < 102; i++) {
		fragmented_packet[1][i] = payload++;
	}
	for (i = 5; i < 51; i++) {
		fragmented_packet[2][i] = payload++;
	}
}
#endif

struct pancake_reassembly_buffer {
	uint8_t data[IPv6_MTU];
	struct pancake_ieee_addr src;
	struct pancake_ieee_addr dst;
	struct pancake_frag_hdr frag_hdr;
	uint16_t octets_received;
	uint8_t active;
};
static struct pancake_reassembly_buffer ra_bufs[PANC_MAX_CONCURRENT_REASSEMBLIES];

static struct pancake_frag_hdr * to_pancake_frag_hdr(uint8_t *raw)
{
	struct pancake_frag_hdr * hdr;
	uint8_t dispatch_value;

	if (raw == NULL) {
		goto err_out;
	}
	hdr = (struct pancake_frag_hdr *)raw;

	/* Convert byte order */
	hdr->size	=	ntohs(hdr->size);
	hdr->tag	=	ntohs(hdr->tag);

	dispatch_value = ((hdr->size >> 8) & 0xF8);
	if (dispatch_value != DISPATCH_FRAG1 && dispatch_value != DISPATCH_FRAGN) {
		goto err_out;
	}

	return hdr;
err_out:
	return NULL;
}

static struct pancake_reassembly_buffer * find_reassembly_buffer(struct pancake_frag_hdr *frag_hdr, struct pancake_ieee_addr *src, struct pancake_ieee_addr *dst)
{
	struct pancake_reassembly_buffer *buf = NULL;
	uint8_t dispatch_value;
	uint16_t i;
	int r;

	/* Sanity check */
	if (src == NULL || dst == NULL) {
		goto err_out;
	}

	/* Check what kind of fragmentation packet this is */
	dispatch_value = ((frag_hdr->size >> 8) & 0xF8);
	if (dispatch_value == DISPATCH_FRAG1) {
		for (i = 0; i < PANC_MAX_CONCURRENT_REASSEMBLIES; i++) {
			if (ra_bufs[i].active == 0) {
				/* Setup a fresh reassembly buffer */
				memset(&ra_bufs[i], 0, sizeof(struct pancake_reassembly_buffer));
				memcpy(&ra_bufs[i].src, src, sizeof(struct pancake_ieee_addr));
				memcpy(&ra_bufs[i].dst, dst, sizeof(struct pancake_ieee_addr));
				memcpy(&ra_bufs[i].frag_hdr, frag_hdr, sizeof(struct pancake_frag_hdr));
				ra_bufs[i].octets_received = 0;
				ra_bufs[i].active = 1;
				buf = &ra_bufs[i];
				printf("Used new buffer!\n");
				break;
			}
		}
	}
	/* DISPATCH_FRAGN */
	else {
		/* Check for existing buffer */
		for (i = 0; i < PANC_MAX_CONCURRENT_REASSEMBLIES; i++) {
			/* Is buffer in use? */
			if (ra_bufs[i].active == 0) {
				continue;
			}

			/* Compare IEEE 802.15.4 source address */
			r = memcmp(&ra_bufs[i].src, src, sizeof(struct pancake_ieee_addr));
			if (r != 0) {
				continue;
			}

			/* Compare IEEE 802.15.4 destination address */
			r = memcmp(&ra_bufs[i].dst, dst, sizeof(struct pancake_ieee_addr));
			if (r != 0) {
				continue;
			}
			/* Compare datagram size (mask out dispatch byte) */
			if ((ra_bufs[i].frag_hdr.size & 0x7FF) != (frag_hdr->size & 0x7FF)) {
				continue;
			}

			/* Compare datagram tag */
			if (ra_bufs[i].frag_hdr.tag != frag_hdr->tag) {
				continue;
			}

			buf = &ra_bufs[i];
		}
	}

	return buf;
err_out:
	return NULL;
}

/*
 * First packet:
 * +-----------------------------------------------------------------+
 * | Datagram Header (fixed length)             | Datagram           |
 * +-----------------+-------+-------+----------+--------------------+
 * | Header overhead | F Typ | F Hdr | IPv6 Typ | IPv6 Hdr | Payload |
 * +-----------------+-------+-------+----------+--------------------+
 * 
 * Subsequent packets:
 * +-----------------------------------------------------------------+
 * | Datagram Header (fixed length)             | Datagram           |
 * +-----------------+-------+------------------+--------------------+
 * | Header overhead | F Typ | F Hdr (+offset)  | ...yload           |
 * +-----------------+-------+------------------+--------------------+
 */ 
static struct pancake_reassembly_buffer * pancake_reassemble(struct pancake_main_dev *dev, struct pancake_ieee_addr *src, struct pancake_ieee_addr *dst, uint8_t *data, uint16_t data_length)
{
	struct pancake_reassembly_buffer *buf = NULL;
	struct pancake_frag_hdr *frag_hdr;
	uint8_t frag_dispatch;
	PANCSTATUS ret;

	printf("reassembly.c: pancake_reassemble entered\n");

	/* Sanity check */
	if (dev == NULL || src == NULL || dst == NULL || (data == NULL && data_length != 0)) {
		goto err_out;
	}

	/* data should start with a fragmentation header */
	frag_hdr = to_pancake_frag_hdr(data);
	if (frag_hdr == NULL) {
		goto err_out;
	}
	frag_dispatch = ((frag_hdr->size >> 8) & 0xF8);

	buf = find_reassembly_buffer(frag_hdr, src, dst);
	if (buf == NULL) {
		goto err_out;
	}

	/* Copy data */
	if (frag_dispatch == DISPATCH_FRAG1) {
		memcpy(&buf->data[0], (data+5), data_length);
	}
	else {
		memcpy(&buf->data[frag_hdr->offset], (data+5), data_length);
	}
	buf->octets_received += (data_length - 5);

#if 0
	printf("buf->frag_hdr.size == 0x%04X\n", buf->frag_hdr.size & 0x7FF);
	printf("buf->octets_received == 0x%04X\n", buf->octets_received);
#endif

	if ((buf->frag_hdr.size & 0x7FF) != buf->octets_received) {
		printf("reassembly.c: not all octets received yet\n");
		goto err_out;
	}

	return buf;
err_out:
	return NULL;
}

#if PANC_TESTS_ENABLED != 0
PANCSTATUS pancake_reassembly_test(PANCHANDLE handle)
{
	struct pancake_main_dev *dev = &devs[handle];
	struct pancake_ieee_addr addr = {
		.ieee_ext = {
			0x00,0x00,0x00,0x00,
			0x00,0x00,0x00,0x01,
		},
		.addr_mode = PANCAKE_IEEE_ADDR_MODE_EXTENDED,
	};
	printf("reassembly.c: Initiating reassembly test\n");

	populate_fragmented_packets();
	pancake_process_data(dev->dev_data, &addr, &addr, fragmented_packet[0], 102);
	/* process_data alters our data, repopulate it */
	populate_fragmented_packets();
	/* change tag number for first fragmented packet */
	fragmented_packet[0][3] = 2;
	pancake_process_data(dev->dev_data, &addr, &addr, fragmented_packet[0], 102);
	/* tag == 1 for 2nd and 3rd packet still */
	pancake_process_data(dev->dev_data, &addr, &addr, fragmented_packet[1], 102);
	pancake_process_data(dev->dev_data, &addr, &addr, fragmented_packet[2], 51);
	/* repopulate 2nd and 3rd (and 1st) packet again */
	populate_fragmented_packets();
	/* change tag */
	fragmented_packet[1][3] = 2;
	fragmented_packet[2][3] = 2;
	pancake_process_data(dev->dev_data, &addr, &addr, fragmented_packet[1], 102);
	pancake_process_data(dev->dev_data, &addr, &addr, fragmented_packet[2], 51);

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}
#endif
