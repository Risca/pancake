#include <pancake.h>
#include <stdio.h>
#include <in6_addr.h>

/**
 * Creates an 128 bit ipv6 address
 * @address_prefix A 64 bit address prefix, can be link_local prefix for a local address
 * @interface_identifier The indentifier for this device
 * @identifier_length Length of the identifier in bits, typically 48 or 16 bits
 * @address A pointer to the address that is beeing created
 */
PANCSTATUS pancake_get_in6_address(const uint8_t *address_prefix, const uint8_t *interface_identifier, const uint8_t identifier_length, struct in6_addr *address)
{
	uint8_t *interface_id = address->s6_addr;
	interface_id += 8;

	// Copy address prefix
	memcpy(address, address_prefix, 8);

	// Check length of identifier
	switch(identifier_length) {
	// 64 bits exists, just copy over
	case 64:
		// Copy all 64 bits to end of address
		memcpy(interface_id, interface_identifier, 8);
		/* Complement U/L bit */
		*interface_id ^= (1 << 1);
		return PANCSTATUS_OK;

	// 32 bits are like this [16 bit PAN-ID : 16 bit short-address]
	case 32:
		// Converted in this way (in order)
		// 48 bit [PAN : PAN : 00 : 00 : SHORT : SHORT]
		// 64 bit [PAN : PAN : 00 : FF : FE : 00 : SHORT : SHORT]

		// Pan ID
		memcpy(interface_id, interface_identifier, 2);
		interface_identifier += 2;
		break;

	// No PAN-ID exists, put 16 bit zeros instead
	case 16:
		memset(interface_id, 0, 2);
		break;

	default:
		goto err_out;
	}

	// Converted in this way (in order)
	// 48 bit [PAN : PAN : 00 : 00 : SHORT : SHORT]
	// 64 bit [PAN : PAN : 00 : FF : FE : 00 : SHORT : SHORT]

	// Add zeros
	*(interface_id + 2) = 0x00;
	*(interface_id + 3) = 0xff;
	*(interface_id + 4) = 0xfe;
	*(interface_id + 5) = 0x00;

	// Add short address
	memcpy(interface_id + 6, interface_identifier, 2);

	// Set the Universal/Local" (U/L) bit to 0
	*interface_id &= ~(1 << 1);

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
	
}
