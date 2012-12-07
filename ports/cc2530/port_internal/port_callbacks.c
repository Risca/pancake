
//_____ I N C L U D E S________________________________________________________
/* Port */
#include "port_callbacks.h"
#include "port_internal.h"

/* Hal Driver includes */
#include "hal_types.h"
#include "hal_key.h"
#include "hal_timer.h"
#include "hal_drivers.h"
#include "hal_led.h"
#include "hal_adc.h"
#include "hal_lcd.h"

/* OS includes */
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OSAL_PwrMgr.h"

/* Application Includes */
#include "OnBoard.h"

/* MAC Application Interface */
#include "mac_api.h"
#include "mac_main.h"

#include "stdint.h"

//_____ F U N C T I O N   D E F I N I T I O N S________________________________
uint16 port_process_mac_event(uint8 taskId, uint16 events)
{
  uint8* pMsg;
  macCbackEvent_t* pData;

  if (events & SYS_EVENT_MSG)
  {
    while ((pMsg = osal_msg_receive(port_process_mac_event_task_id)) != NULL)
    {
	  pData = (macCbackEvent_t *) pMsg;
	  
      switch ( *pMsg )
      {
		/* Got an association request from another device. */
        case MAC_MLME_ASSOCIATE_IND:
		  port_send_associate_response( pData );
          break;

		/* We are now associated to a cordinator. */
        case MAC_MLME_ASSOCIATE_CNF:
		  port_associate_response_received();
          break;

		/* Received on various occations - Probably no need to implement. */
        case MAC_MLME_COMM_STATUS_IND:
          break;

		/* Got a beacon frame */
        case MAC_MLME_BEACON_NOTIFY_IND:
		  port_beacon_received( pData );
          break;

		/* Completed setting up the the coordinator of a PAN */
        case MAC_MLME_START_CNF:
          /* Set some indicator for the Coordinator */
          if (pData->startCnf.hdr.status == MAC_SUCCESS)
          {
            HalLedSet (HAL_LED_4, HAL_LED_MODE_ON);
			is_online = TRUE;
          }
          break;

		/* Scan completed */
        case MAC_MLME_SCAN_CNF:
          /* Check if there is any Coordinator out there */
          pData = (macCbackEvent_t *) pMsg;

          /* If there is no other on the channel or no other with sampleBeacon */
          if ((pData->scanCnf.resultListSize == 0) && (pData->scanCnf.hdr.status == MAC_NO_BEACON)) {
            // Start coordinator
			port_init_coordinator();
          }
		  else {
			// Start device and try to associate
			port_init_device();
			port_send_associate_request();
		  }
		
			
          break;

		/* Status of last transmission */
        case MAC_MCPS_DATA_CNF:
          // TO BE IMPLEMENTED
          break;

		/* Data received from the MAC */
        case MAC_MCPS_DATA_IND:
          port_data_received( pData );
          break;
      }

      /* Deallocate */
      mac_msg_deallocate((uint8 **)&pMsg);
    }

    return events ^ SYS_EVENT_MSG;
  }
  return 0;
}



void port_process_key_event(uint8 keys, uint8 state)
{
	if ( keys & HAL_KEY_SW_1 )
	{
		port_send_scan_request( MAC_SCAN_ACTIVE, 3 );
	  	//HalLedSet( HAL_LED_2, HAL_LED_MODE_ON );
	}

}