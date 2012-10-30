#include <stdio.h>

#if PANC_TESTS_ENABLED != 0
static uint8_t fragmented_packet[3][102] = {{
	FRAG1,0xF0,0x00,0x01,/* Fragmentation header */
	IPv6,                /* IPv6 dispatch byte */
	6<<4,0x00,0x00,0x00, /* Raw IPv6 header */
	0x00,0xC8,0xFE,0x02,
	0x00,0x00,0x00,0x00, /* Source address (localhost) */
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x01,
	0x00,0x00,0x00,0x00, /* Destination address (localhost) */
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x01,
	},{
	FRAGN,0xF0,0x00,0x01,/* Fragmentation header */
	0x61,                /* Offset */
	},{
	FRAGN,0xF0,0x00,0x01,/* Fragmentation header */
	0xC2,                /* Offset */
	}};
void populate_fragmented_packet()
{
	uint8_t i;
	uint8_t payload = 0;
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

static PANCSTATUS find_reassembly_buffer(struct pancake_reassembly_buffer **buf, struct pancake_frag_hdr *frag_hdr, struct pancake_ieee_addr *src, struct pancake_ieee_addr *dst)
{
	uint16_t i;
	int r;
	/* Sanity check */
	if (buf == NULL || src == NULL || dst == NULL) {
		goto err_out;
	}

	*buf = NULL;
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

		*buf = &ra_bufs[i];
	}

	/* Buffer not found, let's find a free one */
	if (*buf == NULL) {
		for (i = 0; i < PANC_MAX_CONCURRENT_REASSEMBLIES; i++) {
			if (ra_bufs[i].active == 0) {
				/* Setup a fresh reassembly buffer */
				memset(&ra_bufs[i], 0, sizeof(struct pancake_reassembly_buffer));
				memcpy(&ra_bufs[i].src, src, sizeof(struct pancake_ieee_addr));
				memcpy(&ra_bufs[i].dst, dst, sizeof(struct pancake_ieee_addr));
				memcpy(&ra_bufs[i].frag_hdr, frag_hdr, sizeof(struct pancake_frag_hdr));
				ra_bufs[i].octets_received = 0;
				ra_bufs[i].active = 1;
				*buf = &ra_bufs[i];
				break;
			}
		}
	}

	/* Buffer not found and no free buffers available */
	if (*buf == NULL) {
		goto err_out;
	}

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
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
	struct pancake_reassembly_buffer *buf;
	struct pancake_frag_hdr *frag_hdr;
	PANCSTATUS ret;

	printf("reassembly.c: pancake_reassemble entered\n");

	/* Sanity check */
	if (dev == NULL || src == NULL || dst == NULL || (data == NULL && data_length != 0)) {
		goto err_out;
	}

	/* data should start with a fragmentation header */
	frag_hdr = (struct pancake_frag_hdr *)data;
	if (frag_hdr->size & 0xF8 != FRAG1 && frag_hdr->size & 0xF8 != FRAGN) {
		goto err_out;
	}

	ret = find_reassembly_buffer(&buf, frag_hdr, src, dst);
	if (ret != PANCSTATUS_OK) {
		goto err_out;
	}

	if (buf->frag_hdr.size != buf->octets_received) {
		printf("reassembly.c: not all octets received yet\n");
		goto err_out;
	}

	return PANCSTATUS_OK;
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
	populate_fragmented_packet();

	pancake_process_data(dev->dev_data, &addr, &addr, fragmented_packet[0], 102);
#if 0
	dev->cfg->write_func(dev->dev_data, NULL, fragmented_packet[0], 102);
	dev->cfg->write_func(dev->dev_data, NULL, fragmented_packet[1], 102);
	dev->cfg->write_func(dev->dev_data, NULL, fragmented_packet[2], 51);
#endif
	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}
#endif
