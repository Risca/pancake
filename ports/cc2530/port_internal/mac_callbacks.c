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

/* Callbacks from TIMAC - Must be implemented */
void MAC_CbackEvent(macCbackEvent_t *pData)
{

  macCbackEvent_t *pMsg = NULL;

  uint8 len = msa_cbackSizeTable[pData->hdr.event];

  switch (pData->hdr.event)
  {
      case MAC_MLME_BEACON_NOTIFY_IND:

      len += sizeof(macPanDesc_t) + pData->beaconNotifyInd.sduLength +
             MAC_PEND_FIELDS_LEN(pData->beaconNotifyInd.pendAddrSpec);
      if ((pMsg = (macCbackEvent_t *) osal_msg_allocate(len)) != NULL)
      {
        /* Copy data over and pass them up */
        osal_memcpy(pMsg, pData, sizeof(macMlmeBeaconNotifyInd_t));
        pMsg->beaconNotifyInd.pPanDesc = (macPanDesc_t *) ((uint8 *) pMsg + sizeof(macMlmeBeaconNotifyInd_t));
        osal_memcpy(pMsg->beaconNotifyInd.pPanDesc, pData->beaconNotifyInd.pPanDesc, sizeof(macPanDesc_t));
        pMsg->beaconNotifyInd.pSdu = (uint8 *) (pMsg->beaconNotifyInd.pPanDesc + 1);
        osal_memcpy(pMsg->beaconNotifyInd.pSdu, pData->beaconNotifyInd.pSdu, pData->beaconNotifyInd.sduLength);
      }
      break;

    case MAC_MCPS_DATA_IND:
      pMsg = pData;
      break;

    default:
      if ((pMsg = (macCbackEvent_t *) osal_msg_allocate(len)) != NULL)
      {
        osal_memcpy(pMsg, pData, len);
      }
      break;
  }

  if (pMsg != NULL)
  {
    osal_msg_send(port_process_event_task_id, (uint8 *) pMsg);
  }
}


/* Callbacks from TIMAC - Must be implemented */
uint8 MAC_CbackCheckPending(void)
{
  return (0);
}