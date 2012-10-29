/*
 * Packet fragmentation
 */

static uint16_t datagram_tag = 0;

PANCSTATUS populate_fragmentation_header(struct pancake_frag_hdr *frag_hdr, uint16_t dgram_hdr_len, uint16_t dgram_len, uint16_t offset)
{
	/* Clear header */
	memset((void *)frag_hdr, 0, 5);

	/* The easy stuff first */
//	frag_hdr->size = htons(dgram_len);
	if (offset == 0) {
		/* First packet */
		frag_hdr->size = htons(dgram_len | (0x18 << 11));
	}
	else {
		/* Subsequent packets */
		frag_hdr->size = htons(dgram_len | (0x1C << 11));
	}

	/* Fill in datagram tag */
	if (offset == 0) {
		datagram_tag++;
	}
	frag_hdr->tag = htons(datagram_tag);

	/* Offset */
	frag_hdr->offset = offset;

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}

PANCSTATUS pancake_send_fragmented(struct pancake_main_dev *dev, uint8_t *raw_data, struct pancake_compressed_ip6_hdr *comp_hdr, uint8_t *payload, uint16_t payload_len)
{
	PANCSTATUS ret;
	struct pancake_frag_hdr *frag_hdr;
	uint16_t packet_size = aMaxPHYPacketSize - aMaxFrameOverhead;
	uint16_t frame_overhead;
	uint16_t dgram_hdr_len;
	uint16_t dgram_len;
	uint16_t space_available;
	uint16_t offset = 0;
	uint8_t frag_hdr_len;

	/* Calculate some stuff we'll need later */
	frame_overhead = calculate_frame_overhead(dev, comp_hdr);
	dgram_len = comp_hdr->size + payload_len;
	dgram_hdr_len = frame_overhead - comp_hdr->size;
	frag_hdr_len = dgram_hdr_len + 4;
	space_available = packet_size - frag_hdr_len;
	payload_len += comp_hdr->size;

	/* Define the location of the fragmentation header */
	frag_hdr = (struct pancake_frag_hdr *)(raw_data + dgram_hdr_len);

	do {
		/* Fill in fragmentation header */
		populate_fragmentation_header(frag_hdr, dgram_hdr_len, dgram_len, offset);

		/* Special treatment of first packet */
		if (offset == 0) {
			/* Copy (compressed) IPv6 header */
			memcpy((void*)(raw_data + frag_hdr_len), (void*)comp_hdr->hdr_data, comp_hdr->size);
			/* Copy first part of payload */
			memcpy((void*)(raw_data + frag_hdr_len + comp_hdr->size), (void*)payload, space_available - comp_hdr->size);
		}
		else {
			/* Copy payload */
			memcpy((void*)(raw_data + frag_hdr_len), (void*)(payload + (offset - comp_hdr->size)), space_available);
		}

		/* Adjust counters */
		offset      += space_available;
		payload_len -= space_available;

		/* Again first packet treatment */
		if (offset == space_available) {
			/* Subsequent packages have 1 less byte available */
			frag_hdr_len    += 1;
			space_available -= 1;
		}

		/* Time to pay a little visit to the transmission fairy */
		ret = dev->cfg->write_func(dev->dev_data, raw_data, packet_size);
		if (ret != PANCSTATUS_OK) {
			goto err_out;
		}
	} while (payload_len > space_available);

	/* Last packet is a little special */
	/* Fill in fragmentation header */
	populate_fragmentation_header(frag_hdr, dgram_hdr_len, dgram_len, offset);

	/* Copy payload */
	memcpy((void*)(raw_data + frag_hdr_len), (void*)(payload + (offset - comp_hdr->size)), payload_len);

	/* Time to pay a little visit to the transmission fairy */
	ret = dev->cfg->write_func(dev->dev_data, raw_data, frag_hdr_len + payload_len);
	if (ret != PANCSTATUS_OK) {
		goto err_out;
	}

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}
