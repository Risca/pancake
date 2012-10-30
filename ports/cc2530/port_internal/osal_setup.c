
/* Port */
#include "port_internal.h"
#include "port_callbacks.h"

/* OSAL */
#include "OSAL.h"
#include "OSAL_Tasks.h"

/* MAC */
#include "mac_api.h"

/* HAL */
#include "hal_drivers.h"
#include "hal_types.h"



/* Needed by OSAL - Must be implemented
 *
 * The order in this table must be identical to the task initialization calls below in osalInitTask. */
const pTaskEventHandlerFn tasksArr[] =
{
  macEventLoop,
  port_process_event,
  Hal_ProcessEvent
};
const uint8 tasksCnt = sizeof( tasksArr ) / sizeof( tasksArr[0] );
uint16 *tasksEvents;

/* Called by OSAL - Must be implemented */
void osalInitTasks( void )
{
  uint8 taskID = 0;

  tasksEvents = (uint16 *)osal_mem_alloc( sizeof( uint16 ) * tasksCnt);
  osal_memset( tasksEvents, 0, (sizeof( uint16 ) * tasksCnt));

  macTaskInit( taskID++ );
  init_process_event_task( taskID++ );
  Hal_Init( taskID );
}

void init_port_process_event( uint8_t task_id )
{
	uint8 i;
	
	/* Initialize the task id */
	port_process_event_task_id = taskId;
	
	/* initialize MAC features */
	MAC_InitDevice();
	MAC_InitCoord();
	
	/* Initialize MAC beacon */
	MAC_InitBeaconDevice();
	MAC_InitBeaconCoord();
	
	/* Reset the MAC */
	MAC_MlmeResetReq(TRUE);

#if 0
	/* Initialize the data packet */
	for (i=MSA_HEADER_LENGTH; i<MSA_PACKET_LENGTH; i++)
	{
	msa_Data1[i] = i-MSA_HEADER_LENGTH;
	}
	
	/* Initialize the echo packet */
	for (i=0; i<MSA_ECHO_LENGTH; i++)
	{
	msa_Data2[i] = 0xEE;
	}
	
	msa_BeaconOrder = MSA_MAC_BEACON_ORDER;
	msa_SuperFrameOrder = MSA_MAC_SUPERFRAME_ORDER;
#endif
}
