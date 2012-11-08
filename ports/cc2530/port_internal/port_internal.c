
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



//_____ F U N C T I O N   D E F I N I T I O N S________________________________
//##### MAC init functions ####################################################
void port_init_coordinator();

//##### MAC request functions #################################################
void port_scan_request(uint8_t scan_type, uint8_t scan_duration)
{
  macMlmeScanReq_t scan_req;

  /* Fill in information for scan request structure */
  scan_req.scanChannels = (uint32_t) 1 << PORT_MAC_CHANNEL;
  scan_req.scanType = scan_type;
  scan_req.scanDuration = scan_duration;
  scan_req.maxResults = PORT_MAC_MAX_RESULTS;
  scan_req.permitJoining = PORT_EBR_PERMITJOINING;   
  scan_req.linkQuality = PORT_EBR_LINKQUALITY;
  scan_req.percentFilter = PORT_EBR_PERCENTFILTER; 
  scan_req.result.pPanDescriptor = scan_pan_results;
  osal_memset(&scan_req.sec, 0, sizeof(macSec_t));

  /* Call scan request */
  MAC_MlmeScanReq(&scan_req);
}