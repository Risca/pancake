
//_____ I N C L U D E S________________________________________________________
/* Port internal API */
#include "port_internal.h"
#include "port_callbacks.h"
#include "pancake.h"

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

#include "stdint.h"
#include "ioCC2530.h"

//_____ P R O T O T Y P E S ___________________________________________________
//##### Public #####
static PANCSTATUS cc2530_init_func(void *dev_data);
static PANCSTATUS cc2530_write_func(void *dev_data, uint8_t *data, uint16_t length);
static PANCSTATUS cc2530_destroy_func(void *dev_data);

//##### Private #####
static void update_timer_init(void);



//_____ V A R I A B L E   D E C L A R A T I O N S______________________________
struct pancake_port_cfg cc2530_cfg = {
  .init_func = cc2530_init_func,
  .write_func = cc2530_write_func,
  .destroy_func = cc2530_destroy_func,
};

uint32_t timer_counter = 0;
uint8_t active_event_call = FALSE;

//##### Globals #####
uint8_t port_process_mac_event_task_id;
macPanDesc_t scan_pan_results[PORT_MAC_MAX_RESULTS];



//_____ F U N C T I O N   D E F I N I T I O N S________________________________
//##### Port specific implementations of main functions #####
static PANCSTATUS cc2530_init_func(void *dev_data)
{
	/* Initialze the HAL driver */
	HalDriverInit();
	
	/* Initialize MAC */
	MAC_Init();
	
	/* Initialize the operating system. This will in turn run osalInitTasks(), 
	 * which will initialize all tasks. */
	osal_init_system();
	
	/* Enable interrupts */
	HAL_ENABLE_INTERRUPTS();
	
	/* Setup Keyboard callback */
	HalKeyConfig(FALSE, port_process_key_event);
	
	update_timer_init();
	
	port_scan_request( MAC_SCAN_ACTIVE, 3 );
	
	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}

static PANCSTATUS cc2530_write_func(void *dev_data, uint8_t *data, uint16_t length) 
{
  
	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}

static PANCSTATUS cc2530_destroy_func(void *dev_data)
{
  
	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}



//##### Port internal functions ###############################################
static void update_timer_init(void)
{
	T4IE = 1;
	T4CTL = 0xF8; // Start free-running clock with 128 div
	T4CCTL0 = 0; 
	T4CCTL1 = 0;
	IRCTL = 0; 
}


HAL_ISR_FUNCTION( Timer4_ISR, T4_VECTOR )
{ 
	HAL_ENTER_ISR();
	if( T4OVFIF & 0x01 )
	{
		timer_counter++;
		if( timer_counter > 0 ) {
			timer_counter = 0;
			if( FALSE == active_event_call ) {
				active_event_call = TRUE;
				
				HalLedSet( HAL_LED_3, HAL_LED_MODE_TOGGLE );
				osal_run_system();
				
				active_event_call = FALSE;
			}
		}
	}
	HAL_EXIT_ISR();
}