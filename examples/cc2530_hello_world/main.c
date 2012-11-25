
//_____ I N C L U D E S________________________________________________________
/* Pancake */
#include <pancake.h>
#include <ip6.h>

/* Hal Drivers */
#include "hal_types.h"
#include "hal_key.h"
#include "hal_timer.h"
#include "hal_drivers.h"
#include "hal_led.h"

/* MAC Application Interface */
#include "mac_api.h"

/* OSAL */
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OnBoard.h"
#include "OSAL_PwrMgr.h"

#include <string.h>



//_____ V A R I A B L E   D E C L A R A T I O N S______________________________
extern struct pancake_port_cfg cc2530_cfg;
struct pancake_options_cfg my_options = {
	.compression = PANC_COMPRESSION_NONE,
	.security = PANC_SECURITY_NONE,
};
PANCHANDLE my_pancake_handle;

// Application specific variables
static uint8_t bogus_packet[64];
static uint8_t is_coordinator = FALSE;
static uint8_t is_device = FALSE;


//_____ F U N C T I O N   D E F I N I T I O N S________________________________
static void my_event_callback(pancake_event *event)
{
	switch( event->type ) {
		case PANC_EVENT_CONNECTION_UPDATE: {
			is_coordinator = event->connection_update.is_coordinator;  
			is_device = event->connection_update.is_device;
		  
		  	// When connected as device, send hello world
			if( PANC_CONNECTED == event->connection_update.status && 
			    is_device ) {
				  
				uint8_t hello_world[] = "Hello, World!";
				memcpy(bogus_packet+40, hello_world, sizeof(hello_world));
				PANCSTATUS ret = pancake_send_packet(my_pancake_handle, bogus_packet, sizeof(bogus_packet));
			}
			break;
		}
		case PANC_EVENT_DATA_RECEIVED: {
			if( is_coordinator ) {
		  		// When we get a message, send response
				uint8_t hello_world_response[] = "Well hello there!";
				memcpy(bogus_packet+40, hello_world_response, sizeof(hello_world_response));
		  
				PANCSTATUS ret = pancake_send_packet(my_pancake_handle, bogus_packet, sizeof(bogus_packet));
			}
			break;
		}
	}
	
  
  	
}


int main(int argc, int **argv)
{
	/* Initialize hardware */
	HAL_BOARD_INIT();
	
	PANCSTATUS ret = pancake_init(&my_pancake_handle, &my_options, &cc2530_cfg, NULL, my_event_callback);
	if (PANCSTATUS_OK != ret) {
		goto err_out;
	}
	
	for(;;);
	
#if 0
	ret = pancake_write_test(my_pancake_handle);
	if (PANCSTATUS_OK != ret) {
		printf("pancake_write_test failed!\n");
	}

	pancake_destroy(my_pancake_handle);
#endif
	
	
	
	return 0;
err_out:
  	return -1;
}