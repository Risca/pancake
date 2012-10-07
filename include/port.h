#ifndef PORT_H
#define PORT_H
#include <stdint.h>
#include <foo.h>

struct foo_dev_cfg {
	FOOSTATUS (*init_func)(void *dev_data);
    FOOSTATUS (*read_func)(void);
    FOOSTATUS (*write_func)(void *dev_data, uint8_t *data, uint16_t *length);
};
#endif
