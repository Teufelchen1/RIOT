#include "stsafea_service.h"
#include "periph/i2c.h"
#include "checksum/crc16_ccitt.h"
#include "ztimer.h"

#ifndef STSAFEA_I2C_NUM
#define STSAFEA_I2C_NUM 0
#endif

#define STSAFEA_DEVICE_ADDRESS                    0x0020

int32_t io_init(void)
{
  return STSAFEA_BUS_OK;
}
int32_t bus_init(void)
{
  return STSAFEA_BUS_OK;
}

int32_t bus_deinit(void)
{
  return STSAFEA_BUS_OK;
}

int32_t bus_send(uint16_t DevAddr, uint8_t *pData, uint16_t Length)
{
  int ret = -1;

  i2c_acquire(I2C_DEV(STSAFEA_I2C_NUM));
  ret = i2c_write_bytes(I2C_DEV(STSAFEA_I2C_NUM), DevAddr>> 1, pData, Length, 0);
  i2c_release(I2C_DEV(STSAFEA_I2C_NUM));

  if (ret != 0) {
    return STSAFEA_BUS_ERR;
  }
  return STSAFEA_BUS_OK;
}

int32_t bus_recv(uint16_t DevAddr, uint8_t *pData, uint16_t Length)
{
  int ret = -1;
  i2c_acquire(I2C_DEV(STSAFEA_I2C_NUM));
  ret = i2c_read_bytes(I2C_DEV(0), DevAddr>> 1, pData, Length, 0);
  i2c_release(I2C_DEV(STSAFEA_I2C_NUM));

  if (ret != 0) {
    /* This sleep is needed since the SDK does a quick "polling" read-loop.
     * When pushing big amounts of data (> ~100 Byte) to the A110, the 
     * computation time needed by the device might be longer than the 
     * polling attemps. Sleeping a few milliseconds after a failed read
     * seems to work around this issue permanently.
     */
    ztimer_sleep(ZTIMER_MSEC, 2);
    return STSAFEA_BUS_ERR;
  }

  return STSAFEA_BUS_OK;
}

void time_delay(uint32_t msDelay)
{
  ztimer_sleep(ZTIMER_MSEC, msDelay);
}

int32_t crc_init(void)
{
  return STSAFEA_BUS_OK;
}

uint32_t crc_comp(uint8_t *pData1, uint16_t Length1, uint8_t *pData2, uint16_t Length2)
{
  (void)Length1;

  uint16_t crc16 = 0;
  if ((pData1 != NULL) && (pData2 != NULL))
  {
    crc16 = crc16_ccitt_mcrf4xx_calc(&pData1[0], 1);
    crc16 = crc16_ccitt_mcrf4xx_update(crc16, pData2, Length2);

    crc16 = (uint16_t)SWAP2BYTES(crc16);
    crc16 ^= 0xFFFFU;
  }

  return (uint32_t)crc16;
}

/**
  * @brief  StSafeA_HW_Probe
  *         Configures the middleware to use RIOTs bus functions
  * @param  Context struct provided by the middleware
  * @retval STSAFEA_BUS_OK
  */
int8_t StSafeA_HW_Probe(void *context)
{
  STSAFEA_HW_t *hw_context = (STSAFEA_HW_t *)context;

  hw_context->IOInit     = io_init;
  hw_context->BusInit    = bus_init;
  hw_context->BusDeInit  = bus_deinit;
  hw_context->BusSend    = bus_send;
  hw_context->BusRecv    = bus_recv;
  hw_context->CrcInit    = crc_init;
  hw_context->CrcCompute = crc_comp;
  hw_context->TimeDelay  = time_delay;
  hw_context->DevAddr    = STSAFEA_DEVICE_ADDRESS;

  return STSAFEA_BUS_OK;
}
