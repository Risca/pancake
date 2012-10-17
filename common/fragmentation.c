#include <pancake.h>

extern struct pancake_main_dev devs[PANC_MAX_DEVICES];

static uint16_t datagram_tag = 0;



PANCSTATUS populate_fragmentation_headers(CHAHANDLE handle, struct pancake_frag_hdr **frag_hdrs, uint16_t datagram_size)
{
	uint16_t i;
	uint16_t num_fragments;
	uint16_t total_size = datagram_size;
	uint16_t header_overhead = 0;
	struct pancake_main_dev *dev = &devs[handle];
	struct pancake_options_cfg cfg = dev->cfg;
	struct pancake_frag_hdr hdr;
	struct pancake_frag_hdr *hdrs;

	switch (cfg->compression) {
	case PANC_COMPRESSION_NONE:
		/* IPv6 header overhead */
		header_overhead += 40;
		datagram_size += 40;
		break;
	default:
		/* Not supported yet */
		goto err_out;
	};

	switch (cfg->security) {
	case PANC_SECURITY_NONE:
		break;
	case PANC_SECURITY_AESCCM128:
		header_overhead += 21;
		break;
	case PANC_SECURITY_AESCCM64:
		header_overhead += 13;
		break;
	case PANC_SECURITY_AESCCM32:
		header_overhead += 9;
		break;
	default:
		goto err_out;
	};

	total_size = datagram_size+header_overhead;
	num_fragments = total_size/(102-header_overhead);
//	hdrs = malloc(

	for (i=0; i < num_fragments; i++) {
		hdr = frag_hdrs[i];
		
	}

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}
/*
PANCSTATUS fragment_packet(PANCHANDLE handle, struct ip6_hdr orig_hdr, struct pancake_frag_hdr frag_hdr, uint16_t num)
{
	
}
*/
