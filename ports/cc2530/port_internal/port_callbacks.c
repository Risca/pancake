/* Port */
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



uint16 port_process_event(uint8 taskId, uint16 events)
{
  uint8* pMsg;
  macCbackEvent_t* pData;

  static uint8 index;
  static uint8 sequence;

  if (events & SYS_EVENT_MSG)
  {
    while ((pMsg = osal_msg_receive(MSA_TaskId)) != NULL)
    {
      switch ( *pMsg )
      {
		/* Got an association request from another device. */
        case MAC_MLME_ASSOCIATE_IND:
		  // TO BE IMPLEMENTED
          break;

		/* We are now associated to a cordinator. */
        case MAC_MLME_ASSOCIATE_CNF:
		  // TO BE IMPLEMENTED
          break;

		/* Received on various occations - Probably no need to implement. */
        case MAC_MLME_COMM_STATUS_IND:
          break;

		/* Got a beacon frame */
        case MAC_MLME_BEACON_NOTIFY_IND:
		  // TO BE IMPLEMENTED
          break;

		/* Completed setting up the the coordinator of a PAN */
        case MAC_MLME_START_CNF:
		  // TO BE IMPLEMENTED
          break;

		/* Scan completed */
        case MAC_MLME_SCAN_CNF:
          /* Check if there is any Coordinator out there */
          pData = (macCbackEvent_t *) pMsg;

          /* If there is no other on the channel or no other with sampleBeacon */
          if ((pData->scanCnf.resultListSize == 0) && (pData->scanCnf.hdr.status == MAC_NO_BEACON))
          {
            HalLedBlink (HAL_LED_4, 0, 40, 1000);
          }
          break;

		/* Status of last transmission */
        case MAC_MCPS_DATA_CNF:
          // TO BE IMPLEMENTED
          break;

		/* Data received from the MAC */
        case MAC_MCPS_DATA_IND:
          // TO BE IMPLEMENTED
          break;
      }

      /* Deallocate */
      mac_msg_deallocate((uint8 **)&pMsg);
    }

    return events ^ SYS_EVENT_MSG;
  }
  return 0;
}