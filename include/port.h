#ifndef PORT_H
#define PORT_H
#include <stdint.h>
#include <pancake.h>

struct pancake_dev_cfg {
	PANCSTATUS (*init_func)(void *dev_data);
    PANCSTATUS (*write_func)(void *dev_data, uint8_t *data, uint16_t *length);
};
#endif
