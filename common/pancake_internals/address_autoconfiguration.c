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
PANCSTATUS pancake_get_in6_address(uint8_t *address_prefix, uint8_t *interface_identifier, uint8_t identifier_length, struct in6_addr *address)
{
	uint8_t i;
	uint8_t pan_id[2];
	uint8_t *address_ptr = &address->s6_addr;
	
	// Copy address prefix
	for(i = 0; i < 8; i++) {
		address->s6_addr[i] = *(address_prefix + i);
	}
	
	// Start on interface identifier
	address_ptr += 8;
	
	// Check length of identifier
	switch(identifier_length) {
		
		// 64 bits exists, just copy over
		case 64:
			// Copy all 64 bits to end of address
			for(i = 0; i < 8; i++) {
				*(address_ptr + i) = *(interface_identifier + i);
			}
			break;
			
		// 32 bits are like this [16 bit PAN-ID : 16 bit short-address]
		case 32:
			// Converted in this way (in order)
			// 48 bit [PAN : PAN : 00 : 00 : SHORT : SHORT]
			// 64 bit [PAN : PAN : 00 : FF : FE : 00 : SHORT : SHORT]
			
			// Save short address
			pan_id[0] = (*interface_identifier + 0);
			pan_id[1] = (*interface_identifier + 1);
			interface_identifier += 2;
			
			//! Fall through
		
		// No PAN-ID exists, put 16 bit zeros instead 
		case 16:
			pan_id[0] = 0x00;
			pan_id[1] = 0x00;
			
			//! Fall through
		
		// For both 32 and 16 bit, copy identifier
		case 32 || 16:
			// Converted in this way (in order)
			// 48 bit [PAN : PAN : 00 : 00 : SHORT : SHORT]
			// 64 bit [PAN : PAN : 00 : FF : FE : 00 : SHORT : SHORT]
			
			// Copy PAN-ID
			for(i = 0; i < 2; i++) {
				*(address_ptr + i) = pan_id[i];
			}
			
			// Add zeros
			*(address_ptr + 2) = 0x00;
			*(address_ptr + 3) = 0xff;
			*(address_ptr + 4) = 0xfe;
			*(address_ptr + 5) = 0x00;
			
			// Add short address
			for(i = 0; i < 2; i++) {
				*(address_ptr + 5 + i) = *(interface_identifier + i);
			}
			break;
		
		default: 
			goto err_out;
	}
	
	// Set the Universal/Local" (U/L) bit to 0
	address->s6_addr[0] &= ~(1 << 1); 

	return PANCSTATUS_OK;
	
err_out:
	return PANCSTATUS_ERR;
	
}
