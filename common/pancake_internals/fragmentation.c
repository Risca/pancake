/*
 * Packet fragmentation
 */

static uint16_t datagram_tag = 0;

PANCSTATUS populate_fragmentation_header(PANCHANDLE handle, struct pancake_frag_hdr *frag_hdr, uint16_t datagram_overhead, uint16_t datagram_size, uint16_t packages_sent)
{
	struct pancake_main_dev *dev = &devs[handle];
	uint16_t bytes_sent_per_packet = aMaxPHYPacketSize-datagram_overhead;

	/* The easy stuff first */
	frag_hdr->size = htons(datagram_size);
	if (packages_sent == 0) {
		frag_hdr->size |= (0x18 << 11);
	}
	else {
		frag_hdr->size |= (0x1C << 11);
	}

	/* Fill in datagram tag */
	frag_hdr->tag = datagram_tag;
	datagram_tag++;

	/* Offset */
	frag_hdr->offset = bytes_sent_per_packet*packages_sent;

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}

