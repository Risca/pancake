
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
// For random numbers
#include "mac_radio_defs.h"

/* OSAL */
#include "OSAL.h"
#include "OSAL_Tasks.h"
#include "OnBoard.h"
#include "OSAL_PwrMgr.h"

#include "stdint.h"
#include <string.h>
#include "ioCC2530.h"



//_____ T Y P E   D E F I N I T I O N S________________________________
/* Structure that contains information about the device that associates with the coordinator */
typedef struct
{
  uint16 devShortAddr;
  uint8  isDirectMsg;
}DeviceInfo_t;

/* Array contains the information of the devices */
static DeviceInfo_t DeviceRecord[PORT_MAX_DEVICE_NUM];

//_____ V A R I A B L E   D E C L A R A T I O N S______________________________
/* Coordinator and Device information */
sAddrExt_t ExtAddr;
sAddrExt_t ExtAddr1 = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
sAddrExt_t ExtAddr2 = {0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xF0, 0x00, 0x00};

uint16 DevShortAddrList[] = {0x0001, 0x0002, 0x0003, 0x0004, 0x0005,
                             0x0006, 0x0007, 0x0008, 0x0009, 0x000A,
                             0x000B, 0x000C, 0x000D, 0x000E, 0x000F,
                             0x0010, 0x0011, 0x0012, 0x0013, 0x0014,
                             0x0015, 0x0016, 0x0017, 0x0018, 0x0019,
                             0x001A, 0x001B, 0x001C, 0x001D, 0x001E,
                             0x001F, 0x0020, 0x0021, 0x0022, 0x0023,
                             0x0024, 0x0025, 0x0026, 0x0027, 0x0028,
                             0x0029, 0x002A, 0x002B, 0x002C, 0x002D,
                             0x002E, 0x002F, 0x0030, 0x0031, 0x0032};

/* Current number of devices associated to the coordinator */
uint8 NumOfDevices = 0;

/* Coordinator and Device information */
uint16 PanId = PORT_PAN_ID;
uint16 CoordShortAddr = PORT_COORD_SHORT_ADDR;
uint16 DevShortAddr   = PORT_DEV_SHORT_ADDR;

bool MACTrue = TRUE;
bool MACFalse = FALSE;

uint8 BeaconPayload[] = {0x22, 0x33, 0x44};
uint8 BeaconPayloadLen = 3;

uint8 SuperFrameOrder = PORT_MAC_SUPERFRAME_ORDER;
uint8 BeaconOrder = PORT_MAC_BEACON_ORDER;

uint8 securityLevel = MAC_SEC_LEVEL_NONE;
uint8 keyIdMode     = MAC_KEY_ID_MODE_NONE;

/* Structure that used for association request */
macMlmeAssociateReq_t AssociateReq;

/* Structure that used for association response */
macMlmeAssociateRsp_t AssociateRsp;

// Passed to the application
struct pancake_event_con_update connection_update;

//_____ F U N C T I O N   D E F I N I T I O N S________________________________
//##### MAC init functions ####################################################
void port_init_coordinator(void)
{
	macMlmeStartReq_t   startReq;

	/* Setup MAC_EXTENDED_ADDRESS */
	sAddrExtCpy(ExtAddr, ExtAddr1);
	MAC_MlmeSetReq(MAC_EXTENDED_ADDRESS, &ExtAddr);
	
	/* Setup MAC_SHORT_ADDRESS */
	MAC_MlmeSetReq(MAC_SHORT_ADDRESS, &CoordShortAddr);
	
	/* Setup MAC_BEACON_PAYLOAD_LENGTH */
	MAC_MlmeSetReq(MAC_BEACON_PAYLOAD_LENGTH, &BeaconPayloadLen);
	
	/* Setup MAC_BEACON_PAYLOAD */
	MAC_MlmeSetReq(MAC_BEACON_PAYLOAD, &BeaconPayload);
	
	/* Enable RX */
	MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &MACTrue);
	
	/* Setup MAC_ASSOCIATION_PERMIT */
	MAC_MlmeSetReq(MAC_ASSOCIATION_PERMIT, &MACTrue);
	
	/* Fill in the information for the start request structure */
	startReq.startTime = 0;
	startReq.panId = PanId;
	startReq.logicalChannel = PORT_MAC_CHANNEL;
	startReq.beaconOrder = BeaconOrder;
	startReq.superframeOrder = SuperFrameOrder;
	startReq.panCoordinator = TRUE;
	startReq.batteryLifeExt = FALSE;
	startReq.coordRealignment = FALSE;
	startReq.realignSec.securityLevel = FALSE;
	startReq.beaconSec.securityLevel = FALSE;
	
	/* Call start request to start the device as a coordinator */
	MAC_MlmeStartReq(&startReq);
	
	is_coordinator = TRUE;
}

void port_init_device(void)
{
	uint16 AtoD = 0;

	/* On CC2430 and CC2530 use current MAC timer */
	AtoD = macMcuPrecisionCount();
	ExtAddr2[6] = HI_UINT16( AtoD );
	ExtAddr2[7] = LO_UINT16( AtoD );
	
	sAddrExtCpy(ExtAddr, ExtAddr2);
	MAC_MlmeSetReq(MAC_EXTENDED_ADDRESS, &ExtAddr);
	
	/* Setup MAC_BEACON_PAYLOAD_LENGTH */
	MAC_MlmeSetReq(MAC_BEACON_PAYLOAD_LENGTH, &BeaconPayloadLen);
	
	/* Setup MAC_BEACON_PAYLOAD */
	MAC_MlmeSetReq(MAC_BEACON_PAYLOAD, &BeaconPayload);
	
	/* Setup PAN ID */
	MAC_MlmeSetReq(MAC_PAN_ID, &PanId);
	
	/* This device is setup for Direct Message */
	MAC_MlmeSetReq(MAC_RX_ON_WHEN_IDLE, &MACTrue);
	
	/* Setup Coordinator short address */
	MAC_MlmeSetReq(MAC_COORD_SHORT_ADDRESS, &AssociateReq.coordAddress.addr.shortAddr);
	
	is_device = TRUE;
}

//##### MAC send/request functions ############################################
void port_send_scan_request(uint8_t scan_type, uint8_t scan_duration)
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

void port_send_associate_request(void)
{
	MAC_MlmeAssociateReq(&AssociateReq);
}

void port_send_associate_response( macCbackEvent_t* pData )
{
	/* Assign the short address  for the Device, from pool */
	uint16 assocShortAddress = DevShortAddrList[NumOfDevices];
	
	/* Build the record for this device */
	DeviceRecord[NumOfDevices].devShortAddr = DevShortAddrList[NumOfDevices];
	DeviceRecord[NumOfDevices].isDirectMsg = pData->associateInd.capabilityInformation & MAC_CAPABLE_RX_ON_IDLE;
	NumOfDevices++;
	
	/* If the number of devices are more than MAX_DEVICE_NUM, turn off the association permit */
	if (NumOfDevices == PORT_MAX_DEVICE_NUM)
		MAC_MlmeSetReq(MAC_ASSOCIATION_PERMIT, &MACFalse);
	
	/* Fill in association respond message */
	sAddrExtCpy(AssociateRsp.deviceAddress, pData->associateInd.deviceAddress);
	AssociateRsp.assocShortAddress = assocShortAddress;
	AssociateRsp.status = MAC_SUCCESS;
	AssociateRsp.sec.securityLevel = MAC_SEC_LEVEL_NONE;
	
	/* Call Associate Response */
	MAC_MlmeAssociateRsp(&AssociateRsp);
	
	/* Update connection status */
	connection_update.status = PANC_CONNECTED;
	connection_update.is_device = FALSE;
	connection_update.is_coordinator = TRUE;
	
	PANCSTATUS ret = pancake_connection_update( NULL, &connection_update );
}

void port_send_data_request(uint8* data, uint8 dataLength, bool directMsg, uint16 dstShortAddr)
{
  macMcpsDataReq_t  *pData;
  static uint8      handle = 0;

  pData = MAC_McpsDataAlloc(dataLength, securityLevel, keyIdMode);
  if (NULL != pData)
  {
    pData->mac.srcAddrMode = SADDR_MODE_SHORT;
    pData->mac.dstAddr.addrMode = SADDR_MODE_SHORT;
    pData->mac.dstAddr.addr.shortAddr = dstShortAddr;
    pData->mac.dstPanId = PanId;
    pData->mac.msduHandle = handle++;
    pData->mac.txOptions = MAC_TXOPTION_ACK;

    /* Copy data */
    osal_memcpy(pData->msdu.p, data, dataLength);

    /* Send out data request */
    MAC_McpsDataReq(pData);
  }
  else {
	halAssertHandler();
  }

}



//##### MAC receive functions ############################################
void port_beacon_received( macCbackEvent_t* pData ) 
{
  
	//if( PORT_PAN_ID == beacon_pan_id ) {
		AssociateReq.logicalChannel = PORT_MAC_CHANNEL;
		AssociateReq.coordAddress.addrMode = SADDR_MODE_SHORT;
		AssociateReq.coordAddress.addr.shortAddr = pData->beaconNotifyInd.pPanDesc->coordAddress.addr.shortAddr;
		AssociateReq.coordPanId = pData->beaconNotifyInd.pPanDesc->coordPanId;
		
		// Direct messaging
		AssociateReq.capabilityInformation = MAC_CAPABLE_ALLOC_ADDR | MAC_CAPABLE_RX_ON_IDLE;
	
		AssociateReq.sec.securityLevel = MAC_SEC_LEVEL_NONE;
		
		/* Retrieve beacon order and superframe order from the beacon */
		BeaconOrder = MAC_SFS_BEACON_ORDER(pData->beaconNotifyInd.pPanDesc->superframeSpec);
		SuperFrameOrder = MAC_SFS_SUPERFRAME_ORDER(pData->beaconNotifyInd.pPanDesc->superframeSpec);
		
	// 	HalLedBlink(HAL_LED_2, 0, 40, 1000);
	//}
	//else
	//	HalLedSet( HAL_LED_2, HAL_LED_MODE_ON );
}

void port_associate_response_received(void)
{
  	// Device now online
	is_online = TRUE;
	
	connection_update.status = PANC_CONNECTED;
	connection_update.is_device = TRUE;
	connection_update.is_coordinator = FALSE;
	
	PANCSTATUS ret = pancake_connection_update( NULL, &connection_update );
}

void port_data_received( macCbackEvent_t* pData )
{
	HalLedBlink(HAL_LED_4, 0, 40, 1000);
	struct pancake_ieee_addr dst;
	struct pancake_ieee_addr src;
	
	dst.ieee_short = pData->dataInd.mac.dstAddr.addr.shortAddr;
	dst.addr_mode = PANCAKE_IEEE_ADDR_MODE_SHORT;
	src.ieee_short = pData->dataInd.mac.srcAddr.addr.shortAddr;
	src.addr_mode = PANCAKE_IEEE_ADDR_MODE_SHORT;

	PANCSTATUS ret = pancake_process_data( NULL, &src, &dst, pData->dataInd.msdu.p, pData->dataInd.msdu.len );
	if(PANCSTATUS_ERR == ret) {
		halAssertHandler();
	}
}