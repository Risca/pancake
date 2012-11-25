#if PANC_TESTS_ENABLED != 0
extern void pancake_print_raw_bits(FILE *out, uint8_t *bytes, size_t length);
static uint8_t fragmented_packet[3][102];

static void populate_fragmented_packets(void);

static void populate_fragmented_packets(void)
{
	uint8_t i;
	uint8_t payload = 0;
	struct pancake_frag_hdr *frag_hdr;
	struct ip6_hdr *ip6;
	
	/* Fill in fragmentation headers */
	frag_hdr = (struct pancake_frag_hdr *) &fragmented_packet[0][0];
	frag_hdr->size = htons(240 | (DISPATCH_FRAG1 << 8));
	frag_hdr->tag = 0;

	frag_hdr = (struct pancake_frag_hdr *) &fragmented_packet[1][0];
	frag_hdr->size = htons(240 | (DISPATCH_FRAGN << 8));
	frag_hdr->tag = 0;
	frag_hdr->offset = 0x0C;

	frag_hdr = (struct pancake_frag_hdr *) &fragmented_packet[2][0];
	frag_hdr->size = htons(240 | (DISPATCH_FRAGN << 8));
	frag_hdr->tag = 0;
	frag_hdr->offset = 0x18;

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
	for (i = 45; i < 101; i++) {
		fragmented_packet[0][i] = payload++;
	}
	for (i = 5; i < 101; i++) {
		fragmented_packet[1][i] = payload++;
	}
	for (i = 5; i < 53; i++) {
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
	uint8_t free;
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

static PANCSTATUS find_reassembly_buffer(struct pancake_reassembly_buffer **ra_buf, struct pancake_frag_hdr *frag_hdr, struct pancake_ieee_addr *src, struct pancake_ieee_addr *dst)
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
			if (ra_bufs[i].free != 0) {
				/* Setup a fresh reassembly buffer */
				memset(&ra_bufs[i], 0, sizeof(struct pancake_reassembly_buffer));
				memcpy(&ra_bufs[i].src, src, sizeof(struct pancake_ieee_addr));
				memcpy(&ra_bufs[i].dst, dst, sizeof(struct pancake_ieee_addr));
				memcpy(&ra_bufs[i].frag_hdr, frag_hdr, sizeof(struct pancake_frag_hdr));
				ra_bufs[i].octets_received = 0;
				ra_bufs[i].free = 0;
				buf = &ra_bufs[i];
				break;
			}
		}

		if (buf == NULL) {
			goto err_nomem;
		}
	}
	/* DISPATCH_FRAGN */
	else {
		/* Check for existing buffer */
		for (i = 0; i < PANC_MAX_CONCURRENT_REASSEMBLIES; i++) {
			/* Is buffer in use? */
			if (ra_bufs[i].free != 0) {
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

		if (buf == NULL) {
			goto err_out;
		}
	}

	*ra_buf = buf;
	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
err_nomem:
	return PANCSTATUS_NOMEM;
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
static PANCSTATUS pancake_reassemble(struct pancake_main_dev *dev, struct pancake_reassembly_buffer **ra_buf, struct pancake_ieee_addr *src, struct pancake_ieee_addr *dst, uint8_t *data, uint16_t data_length)
{
	struct pancake_reassembly_buffer *buf = NULL;
	struct pancake_frag_hdr *frag_hdr;
	uint8_t frag_dispatch;
	PANCSTATUS ret;

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

	ret = find_reassembly_buffer(&buf, frag_hdr, src, dst);
	if (ret != PANCSTATUS_OK) {
		return ret;
	}

	/* Copy data */
	if (frag_dispatch == DISPATCH_FRAG1) {
		/* First packet */
		memcpy(&buf->data[0], (data+5), data_length);
	}
	else {
		/* Subsequent packets */
		memcpy(&buf->data[frag_hdr->offset*8], (data+5), data_length);
	}
	buf->octets_received += (data_length - 5);

	/* Check if we're done */
	if ((buf->frag_hdr.size & 0x7FF) != buf->octets_received) {
		return PANCSTATUS_NOTREADY;
	}

	/* Buffer fully populated, return it and mark it free */
	buf->free = 1;
	*ra_buf = buf;

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}

#if PANC_TESTS_ENABLED != 0
PANCSTATUS pancake_reassembly_test(PANCHANDLE handle)
{
	PANCSTATUS ret = PANCSTATUS_ERR;
	uint8_t i;
	int res;
	struct pancake_main_dev *dev = &devs[handle];
	struct pancake_reassembly_buffer *buf;
	struct pancake_ieee_addr addr = {
		.ieee_ext = {
			0x00,0x00,0x00,0x00,
			0x00,0x00,0x00,0x01,
		},
		.addr_mode = PANCAKE_IEEE_ADDR_MODE_EXTENDED,
	};

	pancake_printf("pancake_reassembly_test(): Initiating reassembly test\n");

	populate_fragmented_packets();
	pancake_printf("Packet #1:\n");
	pancake_print_raw_bits(NULL, fragmented_packet[0], 101);
	pancake_printf("Packet #2:\n");
	pancake_print_raw_bits(NULL, fragmented_packet[1], 101);
	pancake_printf("Packet #3:\n");
	pancake_print_raw_bits(NULL, fragmented_packet[2], 53);
	/* Memory test */
	for (i = 0; i < PANC_MAX_CONCURRENT_REASSEMBLIES; i++) {
		/* Start fresh and change tag (LSB) */
		populate_fragmented_packets();
		fragmented_packet[0][3] = i;

		ret = pancake_reassemble(dev, &buf, &addr, &addr, fragmented_packet[0], 101);
		if (ret == PANCSTATUS_NOMEM || ret == PANCSTATUS_ERR) {
			goto err_out;
		}
	}

	/* Reassemble all packets */
	for (i = 0; i < PANC_MAX_CONCURRENT_REASSEMBLIES; i++) {
		/* Start fresh and change tag (LSB) */
		populate_fragmented_packets();
		fragmented_packet[1][3] = i;
		fragmented_packet[2][3] = i;

		/* Check middle packet reassembly */
		ret = pancake_reassemble(dev, &buf, &addr, &addr, fragmented_packet[1], 101);
		if (ret != PANCSTATUS_NOTREADY) {
			goto err_out;
		}

		/* Check last packet reassembly */
		ret = pancake_reassemble(dev, &buf, &addr, &addr, fragmented_packet[2], 53);
		if (ret != PANCSTATUS_OK) {
			goto err_out;
		}

		/* Check data */
		ret = PANCSTATUS_ERR;
		res = memcmp(buf->data, &fragmented_packet[0][5], 96);
		if (res != 0) {
			goto err_out;
		}
		res = memcmp(buf->data + 96, &fragmented_packet[1][5], 96);
		if (res != 0) {
			goto err_out;
		}
		res = memcmp(buf->data + 2*96, &fragmented_packet[2][5], 48);
		if (res != 0) {
			goto err_out;
		}
	}
	pancake_printf("Reassembled packet:\n");
	pancake_print_raw_bits(NULL, buf->data, buf->octets_received);

	pancake_printf("pancake_reassembly_test(): Reassembly test successful\n");
	return PANCSTATUS_OK;
err_out:
	return ret;
}
#endif
