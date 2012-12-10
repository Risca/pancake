#ifndef PORT_INTERNAL_H
#define PORT_INTERNAL_H

#include "stdint.h"
#include "mac_api.h"



#define PORT_PAN_ID             	0x5449        /* PAN ID, old 0x11CC */
#define PORT_COORD_SHORT_ADDR		0xAABB        /* Coordinator short address */
#define PORT_DEV_SHORT_ADDR			0x0000
#define PORT_MAX_DEVICE_NUM			32
#define PORT_MAC_CHANNEL 			MAC_CHAN_25   /* Changed from 11 to 2 */   
#define PORT_MAC_BEACON_ORDER      	15            /* Setting beacon order to 15 will disable the beacon */
#define PORT_MAC_SUPERFRAME_ORDER  	15            /* Setting superframe order to 15 will disable the superframe */
#define PORT_MAC_MAX_RESULTS       	5
#define PORT_EBR_PERMITJOINING    	TRUE
#define PORT_EBR_LINKQUALITY      	1
#define PORT_EBR_PERCENTFILTER    	0xFF

#undef POWER_SAVING



//_____ G L O B A L S______________________________
extern uint8_t port_process_mac_event_task_id;
extern macPanDesc_t scan_pan_results[PORT_MAC_MAX_RESULTS];
extern uint8_t is_online;
extern uint8_t is_device;
extern uint8_t is_coordinator;



//_____ F U N C T I O N   D E C L A R A T I O N S______________________________
//##### MAC init functions ####################################################
void port_init_coordinator(void);
void port_init_device(void);

//##### MAC send/request functions #################################################
void port_send_associate_request(void);
void port_send_scan_request(uint8_t scan_type, uint8_t scan_duration);
void port_send_associate_response( macCbackEvent_t* pData );
void port_send_data_request(uint8* data, uint8 dataLength, bool directMsg, uint16 dstShortAddr);

//##### MAC receive functions #################################################
void port_beacon_received( macCbackEvent_t* pData );
void port_associate_response_received( macCbackEvent_t* pData );
void port_data_received( macCbackEvent_t* pData );

//##### MAC status notification functions #################################################
void port_data_sent( macCbackEvent_t* pData );

#endif