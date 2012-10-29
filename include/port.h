#ifndef PORT_H
#define PORT_H
#include <stdint.h>
#include <pancake.h>

struct pancake_port_cfg {
	PANCSTATUS (*init_func)(void *dev_data);
        PANCSTATUS (*write_func)(void *dev_data, uint8_t *data, uint16_t length);
	PANCSTATUS (*destroy_func)(void *dev_data);
};
#endif
