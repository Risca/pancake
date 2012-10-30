
//_____ I N C L U D E S________________________________________________________
/* Port internal API */
#include "port_internal.h"
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



//_____ P R O T O T Y P E S ___________________________________________________
static PANCSTATUS cc2530_init_func(void *dev_data);
static PANCSTATUS cc2530_write_func(void *dev_data, uint8_t *data, uint16_t length);
static PANCSTATUS cc2530_destroy_func(void *dev_data);



//_____ V A R I A B L E   D E C L A R A T I O N S______________________________
struct pancake_port_cfg cc2530_cfg = {
  .init_func = cc2530_init_func,
  .write_func = cc2530_write_func,
  .destroy_func = cc2530_destroy_func,
};

// Defines global task id
uint8 port_process_event_task_id



//_____ F U N C T I O N   D E F I N I T I O N S________________________________
//##### Port specific implementations of main functions #######################
static PANCSTATUS cc2530_init_func(void *dev_data)
{
	/* Initialze the HAL driver */
	HalDriverInit();
	
	/* Initialize MAC */
	MAC_Init();
	
	/* Initialize the operating system */
	osal_init_system();
	
	/* Enable interrupts */
	HAL_ENABLE_INTERRUPTS();
	
	/* Setup Keyboard callback */
	//HalKeyConfig(MSA_KEY_INT_ENABLED, MSA_Main_KeyCallback);
	
	/* Blink LED on startup */
	HalLedBlink (HAL_LED_4, 0, 40, 200);
	
	
	
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