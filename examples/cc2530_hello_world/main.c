
/* Pancake */
#include <pancake.h>
#include <netinet/ip6.h>

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

/* This function invokes osal_start_system */
void msaOSTask(void *task_parameter);

#ifdef IAR_ARMCM3_LM
#include "osal_task.h"
#define OSAL_START_SYSTEM() st(                                         \
  static uint8 param_to_pass;                                           \
  void * task_handle;                                                   \
  uint32 osal_task_priority;                                            \
                                                                        \
  /* create OSAL task */                                                \
  osal_task_priority = OSAL_TASK_PRIORITY_ONE;                          \
  osal_task_create(msaOSTask, "osal_task", OSAL_MIN_STACK_SIZE,         \
                   &param_to_pass, osal_task_priority, &task_handle);   \
  osal_task_start_scheduler();                                          \
)
#else
#define OSAL_START_SYSTEM() st(                                         \
  osal_start_system();                                                  \
)
#endif /* IAR_ARMCM3_LM */



extern struct pancake_port_cfg cc2530_cfg;
struct pancake_options_cfg my_options = {
	.compression = PANC_COMPRESSION_NONE,
	.security = PANC_SECURITY_NONE,
};
PANCHANDLE my_pancake_handle;


/*
static void my_read_callback(struct ip6_hdr *hdr, uint8_t *payload, uint16_t size)
{
	if (hdr == NULL) {
		printf("main.c: Got message: %s\n", payload);
		return;
	}
	printf("main.c: Looping incoming packet to output again\n");

	pancake_send(my_pancake_handle, hdr, payload, size);

}
*/

int main(int argc, int **argv)
{
	/* Initialize hardware */
	HAL_BOARD_INIT();
	
	PANCSTATUS ret = pancake_init(&my_pancake_handle, &my_options, &cc2530_cfg, NULL, my_read_callback);
	if (ret != PANCSTATUS_OK) {
		goto err_out;
	}
	
	/* Start OSAL */
	OSAL_START_SYSTEM();
	
	
	
#if 0
	ret = pancake_write_test(my_pancake_handle);
	if (ret != PANCSTATUS_OK) {
		printf("pancake_write_test failed!\n");
	}

	pancake_destroy(my_pancake_handle);
#endif
	
	
	
	return 0;
err_out:
  	return -1;
}

void msaOSTask(void *task_parameter)
{
  osal_start_system();
}