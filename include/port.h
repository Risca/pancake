#ifndef PORT_H
#define PORT_H
#include <stdint.h>
#include <pancake.h>

enum pancake_ieee_addr_mode {
	PANCAKE_IEEE_ADDR_MODE_NONE,
	PANCAKE_IEEE_ADDR_MODE_SHORT,
	PANCAKE_IEEE_ADDR_MODE_EXTENDED,
};

struct pancake_ieee_addr {
	union {
		uint16_t short_addr;
		uint8_t  ext_addr[8]; /* 64 bit */
	} addr;
	enum pancake_ieee_addr_mode addr_mode;
};
#define ieee_short addr.short_addr
#define ieee_ext addr.ext_addr

struct pancake_port_cfg {
	PANCSTATUS (*init_func)(void *dev_data);
	PANCSTATUS (*write_func)(void *dev_data, struct pancake_ieee_addr *dest, uint8_t *data, uint16_t length);
	PANCSTATUS (*destroy_func)(void *dev_data);
};
#endif
