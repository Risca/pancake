/*
 * Packet fragmentation
 */

static PANCSTATUS populate_fragmentation_header(struct pancake_frag_hdr *frag_hdr, uint16_t dgram_len, uint16_t offset);

static uint16_t datagram_tag = 0;

static PANCSTATUS populate_fragmentation_header(struct pancake_frag_hdr *frag_hdr, uint16_t dgram_len, uint16_t offset)
{
	/* Clear header */
	memset((void *)frag_hdr, 0, 5);

	/* Set size and dispatch value */
	if (offset == 0) {
		/* First packet */
		frag_hdr->size = htons(dgram_len | ((uint16_t)0x18 << 11));
	}
	else {
		/* Subsequent packets */
		frag_hdr->size = htons(dgram_len | ((uint16_t)0x1C << 11));
	}

	/* Fill in datagram tag */
	if (offset == 0) {
		datagram_tag++;
	}
	frag_hdr->tag = htons(datagram_tag);

	if (offset % 8) {
		pancake_printf("Warning, not 8 byte aligned!\n");
	}

	/* Offset */
	frag_hdr->offset = offset/8;

	return PANCSTATUS_OK;
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
static PANCSTATUS pancake_send_fragmented(struct pancake_main_dev *dev, uint8_t *raw_data, struct pancake_ieee_addr *dest_addr, struct pancake_compressed_ip6_hdr *comp_hdr, uint8_t *payload, uint16_t payload_len)
{
	PANCSTATUS ret;
	struct pancake_frag_hdr *frag_hdr;
	uint16_t frame_overhead;
	uint16_t dgram_hdr_len;
	uint16_t dgram_len;
	uint16_t space_available;
	uint16_t offset = 0;
	uint8_t frag_hdr_len = 5;

#if PANC_USE_COLOR != 0
	struct color_change color_positions[] = {
		{
			.color = PANC_COLOR_RED,
			.position = 5,
			.description = "6LoWPAN header",
		},
		{
			.color = PANC_COLOR_GREEN,
			.position = 5+comp_hdr->size,
			.description = "IPv6 Header",
		},
		{
			.color = PANC_COLOR_BLUE,
			.position = comp_hdr->size + payload_len,
			.description = "Payload",
		}
	};
#endif

	/* Calculate some stuff we'll need later */
	frame_overhead = calculate_frame_overhead(dev, comp_hdr);
	dgram_len = comp_hdr->size + payload_len;
	dgram_hdr_len = frame_overhead + frag_hdr_len - comp_hdr->size - 1;
	space_available = aMaxPHYPacketSize - aMaxFrameOverhead - dgram_hdr_len;
	space_available -= space_available % 8;
	payload_len += comp_hdr->size;

	/* Define the location of the fragmentation header */
	frag_hdr = (struct pancake_frag_hdr *)(raw_data + dgram_hdr_len - frag_hdr_len);

	do {
		/* Fill in fragmentation header */
		populate_fragmentation_header(frag_hdr, dgram_len, offset);

		/* Special treatment of first packet */
		if (offset == 0) {
			/* Set (compressed) IPv6 dispatch value */
			*(raw_data + dgram_hdr_len - 1) = comp_hdr->dispatch_value;
			/* Copy (compressed) IPv6 header */
			memcpy((void*)(raw_data + dgram_hdr_len), (void*)comp_hdr->hdr_data, comp_hdr->size);
			/* Copy first part of payload */
			memcpy((void*)(raw_data + dgram_hdr_len + comp_hdr->size), (void*)payload, space_available - comp_hdr->size);
		}
		else {
			/* Copy payload */
			memcpy((void*)(raw_data + dgram_hdr_len), (void*)(payload + (offset - comp_hdr->size)), space_available);
		}

#if PANC_USE_COLOR != 0
		/* Print packet */
		pancake_pretty_print(dev->dev_data, raw_data, dgram_hdr_len + space_available, color_positions, 3);
		if (offset == 0) {
			color_positions[1].position = color_positions[0].position;
		}
#endif

		/* Adjust counters */
		offset      += space_available;
		payload_len -= space_available;

		/* Time to pay a little visit to the transmission fairy */
		ret = dev->cfg->write_func(dev->dev_data, dest_addr, raw_data, dgram_hdr_len + space_available);
		if (ret != PANCSTATUS_OK) {
			goto err_out;
		}
	} while (payload_len > space_available);

	/* Last packet is a little special */
	/* Fill in fragmentation header */
	populate_fragmentation_header(frag_hdr, dgram_len, offset);
	
	/* Copy payload */
	memcpy((void*)(raw_data + dgram_hdr_len), (void*)(payload + (offset - comp_hdr->size)), payload_len);

#if PANC_USE_COLOR != 0
	/* Print packet */
	pancake_pretty_print(dev->dev_data, raw_data, dgram_hdr_len + payload_len, color_positions, 3);
#endif

	/* Time to pay a little visit to the transmission fairy */
	ret = dev->cfg->write_func(dev->dev_data, dest_addr, raw_data, dgram_hdr_len + payload_len);
	if (ret != PANCSTATUS_OK) {
		goto err_out;
	}

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}
