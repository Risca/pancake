#include <pancake.h>

struct pancake_reassembly_buffer {
	uint8_t data[IPv6_MTU];
	struct pancake_ieee_addr src;
	struct pancake_ieee_addr dst;
	uint16_t tag;
	uint8_t active;
};
struct pancake_reassembly_buffer reassemblies[PANC_MAX_CONCURRENT_REASSEMBLIES];

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
PANCSTATUS pancake_reassemble(struct pancake_main_dev *dev, struct pancake_ieee_addr *src, struct pancake_ieee_addr *dst, uint8_t *data, uint16_t data_length)
{
#if 1
	if (dev == NULL || src == NULL || dst == NULL || (data == NULL && data_length != 0)) {
		goto err_out;
	}
#endif

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}
