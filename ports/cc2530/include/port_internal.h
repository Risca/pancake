#ifndef PORT_INTERNAL_H
#define PORT_INTERNAL_H

#include "stdint.h"
#include "mac_api.h"


#define PORT_MAC_CHANNEL MAC_CHAN_11 
#define PORT_MAC_MAX_RESULTS       5
#define PORT_EBR_PERMITJOINING    TRUE
#define PORT_EBR_LINKQUALITY      1
#define PORT_EBR_PERCENTFILTER    0xFF

#undef POWER_SAVING



//_____ G L O B A L S______________________________
extern uint8_t port_process_mac_event_task_id;
extern macPanDesc_t scan_pan_results[PORT_MAC_MAX_RESULTS];



//_____ F U N C T I O N   D E C L A R A T I O N S______________________________
void port_init_coordinator();

void port_scan_request(uint8_t scan_type, uint8_t scan_duration);

#endif