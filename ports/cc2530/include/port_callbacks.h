#ifndef PORT_CALLBACKS_H
#define PORT_CALLBACKS_H

#include "hal_types.h"

uint16 port_process_mac_event(uint8 taskId, uint16 events);
void port_process_key_event(uint8 keys, uint8 state);

#endif