#include <stdint.h>

#define FOO_MAX_DEVICES 1

typedef
uint8_t FOOHANDLE;

struct foo_main_cfg {
    uint8_t (*read_func)(void);
    uint8_t (*write_func)(void *dev_data, uint8_t *data, uint16_t *length);
};

uint8_t foo_init(FOOHANDLE *handle, struct foo_main_cfg *cfg, void *dev_data);
uint8_t foo_write_test(FOOHANDLE handle);
