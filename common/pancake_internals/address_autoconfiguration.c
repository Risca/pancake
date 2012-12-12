#include<netinet/in.h>
#include<port.h>

/**
 * Creates an 128 bit ipv6 address
 * @param A 64 bit address prefix, can be link_local prefix for a local address
 * @param 
 * @param A pointer to the address that is beeing created
 */
PANCSTATUS pancake_get_in6_address(const uint8_t *address_prefix, const struct pancake_radio_id *r_id, struct in6_addr *address)
{
	uint8_t *interface_id = address->s6_addr;
	interface_id += 8;

	// Copy address prefix
	memcpy(address, address_prefix, 8);

	// Check length of identifier
	switch(r_id->type) {
	// 64 bits exists, just copy over
	case PANC_RADIO_ID_EUI64:
		// Copy all 64 bits to end of address
		memcpy(interface_id, r_id->r_EUI64, 8);
		/* Complement U/L bit */
		*interface_id ^= (1 << 1);
		return PANCSTATUS_OK;

	// 32 bits are like this [16 bit PAN-ID : 16 bit short-address]
	case PANC_RADIO_ID_PAN_AND_SHORT_ADDR:
		// Pan ID
		memcpy(interface_id, r_id->r_pan_addr, 2);
		// Short addr
		memcpy(interface_id + 6, r_id->r_short_addr, 2);
		break;

	case PANC_RADIO_ID_SHORT_ADDR_ONLY:
		// No PAN-ID exists, put 16 bit zeros instead
		memset(interface_id, 0, 2);
		// Short addr
		memcpy(interface_id + 6, r_id->r_short_addr, 2);
		break;

	default:
		goto err_out;
	}

	// Converted in this way (in order)
	// 48 bit [PAN : PAN : 00 : 00 : SHORT : SHORT]
	// 64 bit [PAN : PAN : 00 : FF : FE : 00 : SHORT : SHORT]

	// Add "middle" bytes
	*(interface_id + 2) = 0x00;
	*(interface_id + 3) = 0xff;
	*(interface_id + 4) = 0xfe;
	*(interface_id + 5) = 0x00;

	// Set the Universal/Local" (U/L) bit to 0
	*interface_id &= ~(1 << 1);

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}

/**
 * Converts an IPv6 address to ieee address
 */
PANCSTATUS pancake_get_ieee_address_from_ipv6(struct pancake_ieee_addr *ieee_address, struct in6_addr *address)
{
	uint8_t i;
	uint8_t signature[] = {0x00, 0xff, 0xfe, 0x00};
	
	// Check if link local
	for(i = 0; i < 8; i += 1) {
		if(LINK_LOCAL_PREFIX[i] != address->s6_addr[i]) {
			goto err_out;
		}
	}
	
	//[8]-[9] = 00
	
	// Check if 64 bit or 16
	i = memcmp(&address->s6_addr[10], &signature, 4);
	if(i == 0) {
		// Short address
		ieee_address->addr_mode = PANCAKE_IEEE_ADDR_MODE_SHORT;
		
		ieee_address->ieee_short = 0;
		ieee_address->ieee_short = htons((address->s6_addr[14] << 8) | address->s6_addr[15]);
	}
	else {
		// Extended address
		ieee_address->addr_mode = PANCAKE_IEEE_ADDR_MODE_EXTENDED;
		
		for(i = 0; i < 8; i += 1) {
			ieee_address->ieee_ext[i] = address->s6_addr[8 + i];
		}
		/* Complement U/L bit */
		ieee_address->ieee_ext[0] ^= (1 << 1);
	}
	
	return PANCSTATUS_OK;
	
err_out:
	return PANCSTATUS_ERR;
}
