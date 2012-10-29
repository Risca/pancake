#include "pancake.h"



static PANCSTATUS cc2530_init_func(void *dev_data);
static PANCSTATUS cc2530_write_func(void *dev_data, uint8_t *data, uint16_t length);
static PANCSTATUS cc2530_destroy_func(void *dev_data);



struct pancake_port_cfg cc2530_cfg = {
  .init_func = cc2530_init_func,
  .write_func = cc2530_write_func,
  .destroy_func = cc2530_destroy_func,
};



static PANCSTATUS cc2530_init_func(void *dev_data)
{

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