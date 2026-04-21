/**
 ******************************************************************************
 * @file    lis3dh.c
 * @author  MEMS Software Solutions Team
 * @brief   LIS3DH driver file
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "lis3dh.h"

/** @addtogroup BSP BSP
 * @{
 */

/** @addtogroup Component Component
 * @{
 */

/** @defgroup LIS3DH LIS3DH
 * @{
 */

/** @defgroup LIS3DH_Exported_Variables LIS3DH Exported Variables
 * @{
 */

LIS3DH_CommonDrv_t LIS3DH_COMMON_Driver =
{
  LIS3DH_Init,
  LIS3DH_DeInit,
  LIS3DH_ReadID,
  LIS3DH_GetCapabilities,
};

LIS3DH_Drv_t LIS3DH_Driver =
{
  LIS3DH_Enable,
  LIS3DH_Disable,
  LIS3DH_GetSensitivity,
  LIS3DH_GetOutputDataRate,
  LIS3DH_SetOutputDataRate,
  LIS3DH_GetFullScale,
  LIS3DH_SetFullScale,
  LIS3DH_GetAxes,
  LIS3DH_GetAxesRaw,
};

/**
 * @}
 */

/** @defgroup LIS3DH_Private_Function_Prototypes LIS3DH Private Function Prototypes
 * @{
 */

static int32_t ReadAccRegWrap(void *Handle, uint8_t Reg, uint8_t *pData, uint16_t Length);
static int32_t WriteAccRegWrap(void *Handle, uint8_t Reg, const uint8_t *pData, uint16_t Length);
static int32_t LIS3DH_GetSensitivityHR(LIS3DH_Object_t *pObj, float *Sensitivity);
static int32_t LIS3DH_GetSensitivityNM(LIS3DH_Object_t *pObj, float *Sensitivity);
static int32_t LIS3DH_GetSensitivityLP(LIS3DH_Object_t *pObj, float *Sensitivity);
static int32_t LIS3DH_SetOutputDataRate_When_Enabled(LIS3DH_Object_t *pObj, float Odr);
static int32_t LIS3DH_SetOutputDataRate_When_Disabled(LIS3DH_Object_t *pObj, float Odr);

/**
 * @}
 */

/** @defgroup LIS3DH_Exported_Functions LIS3DH Exported Functions
 * @{
 */

/**
 * @brief  Register Component Bus IO operations
 * @param  pObj the device pObj
 * @retval 0 in case of success, an error code otherwise
 */
int32_t LIS3DH_RegisterBusIO(LIS3DH_Object_t *pObj, LIS3DH_IO_t *pIO)
{
  int32_t ret = LIS3DH_OK;

  if (pObj == NULL)
  {
    ret = LIS3DH_ERROR;
  }
  else
  {
    pObj->IO.Init      = pIO->Init;
    pObj->IO.DeInit    = pIO->DeInit;
    pObj->IO.BusType   = pIO->BusType;
    pObj->IO.Address   = pIO->Address;
    pObj->IO.WriteReg  = pIO->WriteReg;
    pObj->IO.ReadReg   = pIO->ReadReg;
    pObj->IO.GetTick   = pIO->GetTick;

    pObj->Ctx.read_reg  = ReadAccRegWrap;
    pObj->Ctx.write_reg = WriteAccRegWrap;
    pObj->Ctx.handle    = pObj;

    if (pObj->IO.Init == NULL)
    {
      ret = LIS3DH_ERROR;
    }
    else if (pObj->IO.Init() != LIS3DH_OK)
    {
      ret = LIS3DH_ERROR;
    }
    else
    {
      if (pObj->IO.BusType == LIS3DH_SPI_3WIRES_BUS) /* SPI 3-Wires */
      {
        /* Enable the SPI 3-Wires support only the first time */
        if (pObj->is_initialized == 0U)
        {
          /* Enable SPI 3-Wires on the component */
          lis3dh_sim_t data = LIS3DH_SPI_3_WIRE;

          if (lis3dh_spi_mode_set(&(pObj->Ctx), data) != LIS3DH_OK)
          {
            return LIS3DH_ERROR;
          }
        }
      }
    }
  }

  return ret;
}

/**
 * @brief  Initialize the LIS3DH sensor
 * @param  pObj the device pObj
 * @retval 0 in case of success, an error code otherwise
 */
int32_t LIS3DH_Init(LIS3DH_Object_t *pObj)
{
  /* Enable BDU */
  if (lis3dh_block_data_update_set(&(pObj->Ctx), PROPERTY_ENABLE) != LIS3DH_OK)
  {
    return LIS3DH_ERROR;
  }

  /* FIFO mode selection */
  if (lis3dh_fifo_mode_set(&(pObj->Ctx), LIS3DH_BYPASS_MODE) != LIS3DH_OK)
  {
    return LIS3DH_ERROR;
  }

  /* Select default output data rate. */
  pObj->acc_odr = LIS3DH_ODR_100Hz;

  /* Output data rate selection - power down. */
  if (lis3dh_data_rate_set(&(pObj->Ctx), LIS3DH_POWER_DOWN) != LIS3DH_OK)
  {
    return LIS3DH_ERROR;
  }

  /* Full scale selection. */
  if (lis3dh_full_scale_set(&(pObj->Ctx), LIS3DH_2g) != LIS3DH_OK)
  {
    return LIS3DH_ERROR;
  }

  pObj->is_initialized = 1;

  return LIS3DH_OK;
}

/**
 * @brief  Deinitialize the LIS3DH accelerometer sensor
 * @param  pObj the device pObj
 * @retval 0 in case of success, an error code otherwise
 */
int32_t LIS3DH_DeInit(LIS3DH_Object_t *pObj)
{
  /* Disable the component */
  if (LIS3DH_Disable(pObj) != LIS3DH_OK)
  {
    return LIS3DH_ERROR;
  }

  /* Reset output data rate. */
  pObj->acc_odr = LIS3DH_POWER_DOWN;
  pObj->is_initialized = 0;

  return LIS3DH_OK;
}

/**
 * @brief  Read component ID
 * @param  pObj the device pObj
 * @param  Id the WHO_AM_I value
 * @retval 0 in case of success, an error code otherwise
 */
int32_t LIS3DH_ReadID(LIS3DH_Object_t *pObj, uint8_t *Id)
{
  if (lis3dh_device_id_get(&(pObj->Ctx), Id) != LIS3DH_OK)
  {
    return LIS3DH_ERROR;
  }

  return LIS3DH_OK;
}

/**
 * @brief  Get LIS3DH accelerometer sensor capabilities
 * @param  pObj Component object pointer
 * @param  Capabilities pointer to LIS3DH accelerometer sensor capabilities
 * @retval Component status
 */
int32_t LIS3DH_GetCapabilities(LIS3DH_Object_t *pObj, LIS3DH_Capabilities_t *Capabilities)
{
  /* Prevent unused argument(s) compilation warning */
  (void)(pObj);

  Capabilities->Acc          = 1;
  Capabilities->Gyro         = 0;
  Capabilities->Magneto      = 0;
  Capabilities->LowPower     = 0;
  Capabilities->GyroMaxFS    = 0;
  Capabilities->AccMaxFS     = 16;
  Capabilities->MagMaxFS     = 0;
  Capabilities->GyroMaxOdr   = 0.0f;
  Capabilities->AccMaxOdr    = 400.0f;
  Capabilities->MagMaxOdr    = 0.0f;
  return LIS3DH_OK;
}

/**
 * @brief  Enable the LIS3DH accelerometer sensor
 * @param  pObj the device pObj
 * @retval 0 in case of success, an error code otherwise
 */
int32_t LIS3DH_Enable(LIS3DH_Object_t *pObj)
{
  /* Check if the component is already enabled */
  if (pObj->acc_is_enabled == 1U)
  {
    return LIS3DH_OK;
  }

  /* Output data rate selection. */
  if (lis3dh_data_rate_set(&(pObj->Ctx), pObj->acc_odr) != LIS3DH_OK)
  {
    return LIS3DH_ERROR;
  }

  pObj->acc_is_enabled = 1;

  return LIS3DH_OK;
}

/**
 * @brief  Disable the LIS3DH accelerometer sensor
 * @param  pObj the device pObj
 * @retval 0 in case of success, an error code otherwise
 */
int32_t LIS3DH_Disable(LIS3DH_Object_t *pObj)
{
  /* Check if the component is already disabled */
  if (pObj->acc_is_enabled == 0U)
  {
    return LIS3DH_OK;
  }

  /* Get current output data rate. */
  if (lis3dh_data_rate_get(&(pObj->Ctx), &pObj->acc_odr) != LIS3DH_OK)
  {
    return LIS3DH_ERROR;
  }

  /* Output data rate selection - power down. */
  if (lis3dh_data_rate_set(&(pObj->Ctx), LIS3DH_POWER_DOWN) != LIS3DH_OK)
  {
    return LIS3DH_ERROR;
  }

  pObj->acc_is_enabled = 0;

  return LIS3DH_OK;
}

/**
 * @brief  Get the LIS3DH accelerometer sensor sensitivity
 * @param  pObj the device pObj
 * @param  Sensitivity pointer
 * @retval 0 in case of success, an error code otherwise
 */
int32_t LIS3DH_GetSensitivity(LIS3DH_Object_t *pObj, float *Sensitivity)
{
  int32_t ret = LIS3DH_OK;
  lis3dh_op_md_t op_mode;

  /* Read operative mode from sensor. */
  if (lis3dh_operating_mode_get(&(pObj->Ctx), &op_mode) != LIS3DH_OK)
  {
    return LIS3DH_ERROR;
  }

  /* Store the Sensitivity based on actual full scale. */
  switch (op_mode)
  {
    case LIS3DH_HR_12bit:
      if (LIS3DH_GetSensitivityHR(pObj, Sensitivity) != LIS3DH_OK)
      {
        return LIS3DH_ERROR;
      }
      break;

    case LIS3DH_NM_10bit:
      if (LIS3DH_GetSensitivityNM(pObj, Sensitivity) != LIS3DH_OK)
      {
        return LIS3DH_ERROR;
      }
      break;

    case LIS3DH_LP_8bit:
      if (LIS3DH_GetSensitivityLP(pObj, Sensitivity) != LIS3DH_OK)
      {
        return LIS3DH_ERROR;
      }
      break;

    default:
      ret = LIS3DH_ERROR;
      break;
  }

  return ret;
}

/**
 * @brief  Get the LIS3DH accelerometer sensor output data rate
 * @param  pObj the device pObj
 * @param  Odr pointer where the output data rate is written
 * @retval 0 in case of success, an error code otherwise
 */
int32_t LIS3DH_GetOutputDataRate(LIS3DH_Object_t *pObj, float *Odr)
{
  int32_t ret = LIS3DH_OK;
  lis3dh_op_md_t op_mode;
  lis3dh_odr_t odr_low_level;

  /* Read operative mode from sensor. */
  if (lis3dh_operating_mode_get(&(pObj->Ctx), &op_mode) != LIS3DH_OK)
  {
    return LIS3DH_ERROR;
  }

  /* Get current output data rate. */
  if (lis3dh_data_rate_get(&(pObj->Ctx), &odr_low_level) != LIS3DH_OK)
  {
    return LIS3DH_ERROR;
  }

  if (op_mode == LIS3DH_LP_8bit) /* LP mode */
  {
    switch (odr_low_level)
    {
      case LIS3DH_POWER_DOWN:
        *Odr = 0.0f;
        break;

      case LIS3DH_ODR_1Hz:
        *Odr = 1.0f;
        break;

      case LIS3DH_ODR_10Hz:
        *Odr = 10.0f;
        break;

      case LIS3DH_ODR_25Hz:
        *Odr = 25.0f;
        break;

      case LIS3DH_ODR_50Hz:
        *Odr = 50.0f;
        break;

      case LIS3DH_ODR_100Hz:
        *Odr = 100.0f;
        break;

      case LIS3DH_ODR_200Hz:
        *Odr = 200.0f;
        break;

      case LIS3DH_ODR_400Hz:
        *Odr = 400.0f;
        break;

      case LIS3DH_ODR_1kHz620_LP:
        *Odr = 1620.0f;
        break;

      case LIS3DH_ODR_5kHz376_LP_1kHz344_NM_HP:
        *Odr = 5376.0f;
        break;

      default:
        ret = LIS3DH_ERROR;
        break;
    }
  }
  else /* HR and NM modes */
  {
    switch (odr_low_level)
    {
      case LIS3DH_POWER_DOWN:
        *Odr = 0.0f;
        break;

      case LIS3DH_ODR_1Hz:
        *Odr = 1.0f;
        break;

      case LIS3DH_ODR_10Hz:
        *Odr = 10.0f;
        break;

      case LIS3DH_ODR_25Hz:
        *Odr = 25.0f;
        break;

      case LIS3DH_ODR_50Hz:
        *Odr = 50.0f;
        break;

      case LIS3DH_ODR_100Hz:
        *Odr = 100.0f;
        break;

      case LIS3DH_ODR_200Hz:
        *Odr = 200.0f;
        break;

      case LIS3DH_ODR_400Hz:
        *Odr = 400.0f;
        break;

      case LIS3DH_ODR_1kHz620_LP:
        *Odr = 1620.0f;
        break;

      case LIS3DH_ODR_5kHz376_LP_1kHz344_NM_HP:
        *Odr = 1344.0f;
        break;

      default:
        ret = LIS3DH_ERROR;
        break;
    }
  }

  return ret;
}

/**
 * @brief  Set the LIS3DH accelerometer sensor output data rate
 * @param  pObj the device pObj
 * @param  Odr the output data rate value to be set
 * @retval 0 in case of success, an error code otherwise
 */
int32_t LIS3DH_SetOutputDataRate(LIS3DH_Object_t *pObj, float Odr)
{
  /* Check if the component is enabled */
  if (pObj->acc_is_enabled == 1U)
  {
    return LIS3DH_SetOutputDataRate_When_Enabled(pObj, Odr);
  }
  else
  {
    return LIS3DH_SetOutputDataRate_When_Disabled(pObj, Odr);
  }
}

/**
 * @brief  Get the LIS3DH accelerometer sensor full scale
 * @param  pObj the device pObj
 * @param  FullScale pointer where the full scale is written
 * @retval 0 in case of success, an error code otherwise
 */
int32_t LIS3DH_GetFullScale(LIS3DH_Object_t *pObj, int32_t *FullScale)
{
  int32_t ret = LIS3DH_OK;
  lis3dh_fs_t fs_low_level;

  /* Read actual full scale selection from sensor. */
  if (lis3dh_full_scale_get(&(pObj->Ctx), &fs_low_level) != LIS3DH_OK)
  {
    return LIS3DH_ERROR;
  }

  switch (fs_low_level)
  {
    case LIS3DH_2g:
      *FullScale =  2;
      break;

    case LIS3DH_4g:
      *FullScale =  4;
      break;

    case LIS3DH_8g:
      *FullScale =  8;
      break;

    case LIS3DH_16g:
      *FullScale = 16;
      break;

    default:
      ret = LIS3DH_ERROR;
      break;
  }

  return ret;
}

/**
 * @brief  Set the LIS3DH accelerometer sensor full scale
 * @param  pObj the device pObj
 * @param  FullScale the functional full scale to be set
 * @retval 0 in case of success, an error code otherwise
 */
int32_t LIS3DH_SetFullScale(LIS3DH_Object_t *pObj, int32_t FullScale)
{
  lis3dh_fs_t new_fs;

  new_fs = (FullScale <= 2) ? LIS3DH_2g
           : (FullScale <= 4) ? LIS3DH_4g
           : (FullScale <= 8) ? LIS3DH_8g
           :                    LIS3DH_16g;

  if (lis3dh_full_scale_set(&(pObj->Ctx), new_fs) != LIS3DH_OK)
  {
    return LIS3DH_ERROR;
  }

  return LIS3DH_OK;
}

/**
 * @brief  Get the LIS3DH accelerometer sensor raw axes
 * @param  pObj the device pObj
 * @param  Value pointer where the raw values of the axes are written
 * @retval 0 in case of success, an error code otherwise
 */
int32_t LIS3DH_GetAxesRaw(LIS3DH_Object_t *pObj, LIS3DH_AxesRaw_t *Value)
{
  int16_t divisor = 1;
  lis3dh_axis3bit16_t data_raw;
  int32_t ret = LIS3DH_OK;
  lis3dh_op_md_t op_mode;

  /* Read operative mode from sensor. */
  if (lis3dh_operating_mode_get(&(pObj->Ctx), &op_mode) != LIS3DH_OK)
  {
    return LIS3DH_ERROR;
  }

  /* Store the sensitivity based on actual full scale. */
  switch (op_mode)
  {
    case LIS3DH_HR_12bit:
      divisor = 16;
      break;

    case LIS3DH_NM_10bit:
      divisor = 64;
      break;

    case LIS3DH_LP_8bit:
      divisor = 256;
      break;

    default:
      ret = LIS3DH_ERROR;
      break;
  }

  if (ret == LIS3DH_ERROR)
  {
    return ret;
  }

  /* Read raw data values. */
  if (lis3dh_acceleration_raw_get(&(pObj->Ctx), data_raw.i16bit) != LIS3DH_OK)
  {
    return LIS3DH_ERROR;
  }

  /* Format the data. */
  Value->x = (data_raw.i16bit[0] / divisor);
  Value->y = (data_raw.i16bit[1] / divisor);
  Value->z = (data_raw.i16bit[2] / divisor);

  return ret;
}

/**
 * @brief  Get the LIS3DH accelerometer sensor axes
 * @param  pObj the device pObj
 * @param  Acceleration pointer where the values of the axes are written
 * @retval 0 in case of success, an error code otherwise
 */
int32_t LIS3DH_GetAxes(LIS3DH_Object_t *pObj, LIS3DH_Axes_t *Acceleration)
{
  LIS3DH_AxesRaw_t data_raw;
  float sensitivity = 0.0f;

  /* Read raw data values. */
  if (LIS3DH_GetAxesRaw(pObj, &data_raw) != LIS3DH_OK)
  {
    return LIS3DH_ERROR;
  }

  /* Get LIS3DH actual sensitivity. */
  if (LIS3DH_GetSensitivity(pObj, &sensitivity) != LIS3DH_OK)
  {
    return LIS3DH_ERROR;
  }

  /* Calculate the data. */
  Acceleration->x = (int32_t)((float)((float)data_raw.x * sensitivity));
  Acceleration->y = (int32_t)((float)((float)data_raw.y * sensitivity));
  Acceleration->z = (int32_t)((float)((float)data_raw.z * sensitivity));

  return LIS3DH_OK;
}

/**
 * @brief  Get the LIS3DH register value for accelerometer sensor
 * @param  pObj the device pObj
 * @param  Reg address to be read
 * @param  Data pointer where the value is written
 * @retval 0 in case of success, an error code otherwise
 */
int32_t LIS3DH_Read_Reg(LIS3DH_Object_t *pObj, uint8_t Reg, uint8_t *Data)
{
  if (lis3dh_read_reg(&(pObj->Ctx), Reg, Data, 1) != LIS3DH_OK)
  {
    return LIS3DH_ERROR;
  }

  return LIS3DH_OK;
}

/**
 * @brief  Set the LIS3DH register value for accelerometer sensor
 * @param  pObj the device pObj
 * @param  Reg address to be written
 * @param  Data value to be written
 * @retval 0 in case of success, an error code otherwise
 */
int32_t LIS3DH_Write_Reg(LIS3DH_Object_t *pObj, uint8_t Reg, uint8_t Data)
{
  if (lis3dh_write_reg(&(pObj->Ctx), Reg, &Data, 1) != LIS3DH_OK)
  {
    return LIS3DH_ERROR;
  }

  return LIS3DH_OK;
}

/**
 * @brief  Get the LIS3DH ACC data ready bit value
 * @param  pObj the device pObj
 * @param  Status the status of data ready bit
 * @retval 0 in case of success, an error code otherwise
 */
int32_t LIS3DH_Get_DRDY_Status(LIS3DH_Object_t *pObj, uint8_t *Status)
{
  if (lis3dh_xl_data_ready_get(&(pObj->Ctx), Status) != LIS3DH_OK)
  {
    return LIS3DH_ERROR;
  }

  return LIS3DH_OK;
}

/**
 * @brief  Get the LIS3DH ACC initialization status
 * @param  pObj the device pObj
 * @param  Status 1 if initialized, 0 otherwise
 * @retval 0 in case of success, an error code otherwise
 */
int32_t LIS3DH_Get_Init_Status(LIS3DH_Object_t *pObj, uint8_t *Status)
{
  if (pObj == NULL)
  {
    return LIS3DH_ERROR;
  }

  *Status = pObj->is_initialized;

  return LIS3DH_OK;
}

/**
 * @}
 */

/** @defgroup LIS3DH_Private_Functions LIS3DH Private Functions
 * @{
 */

/**
 * @brief  Get the LIS3DH accelerometer sensor sensitivity for HR mode
 * @param  pObj the device pObj
 * @param  Sensitivity pointer to sensitivity
 * @retval 0 in case of success, an error code otherwise
 */
static int32_t LIS3DH_GetSensitivityHR(LIS3DH_Object_t *pObj, float *Sensitivity)
{
  int32_t ret = LIS3DH_OK;
  lis3dh_fs_t fullscale;

  /* Read actual full scale selection from sensor. */
  if (lis3dh_full_scale_get(&(pObj->Ctx), &fullscale) != LIS3DH_OK)
  {
    return LIS3DH_ERROR;
  }

  /* Store the sensitivity based on actual full scale. */
  switch (fullscale)
  {
    case LIS3DH_2g:
      *Sensitivity = (float)LIS3DH_SENSITIVITY_FS_2G_HIGH_RESOLUTION_MODE;
      break;

    case LIS3DH_4g:
      *Sensitivity = (float)LIS3DH_SENSITIVITY_FS_4G_HIGH_RESOLUTION_MODE;
      break;

    case LIS3DH_8g:
      *Sensitivity = (float)LIS3DH_SENSITIVITY_FS_8G_HIGH_RESOLUTION_MODE;
      break;

    case LIS3DH_16g:
      *Sensitivity = (float)LIS3DH_SENSITIVITY_FS_16G_HIGH_RESOLUTION_MODE;
      break;

    default:
      ret = LIS3DH_ERROR;
      break;
  }

  return ret;
}

/**
 * @brief  Get the LIS3DH accelerometer sensor sensitivity for NM mode
 * @param  pObj the device pObj
 * @param  Sensitivity pointer to sensitivity
 * @retval 0 in case of success, an error code otherwise
 */
static int32_t LIS3DH_GetSensitivityNM(LIS3DH_Object_t *pObj, float *Sensitivity)
{
  int32_t ret = LIS3DH_OK;
  lis3dh_fs_t fullscale;

  /* Read actual full scale selection from sensor. */
  if (lis3dh_full_scale_get(&(pObj->Ctx), &fullscale) != LIS3DH_OK)
  {
    return LIS3DH_ERROR;
  }

  /* Store the sensitivity based on actual full scale. */
  switch (fullscale)
  {
    case LIS3DH_2g:
      *Sensitivity = (float)LIS3DH_SENSITIVITY_FS_2G_NORMAL_MODE;
      break;

    case LIS3DH_4g:
      *Sensitivity = (float)LIS3DH_SENSITIVITY_FS_4G_NORMAL_MODE;
      break;

    case LIS3DH_8g:
      *Sensitivity = (float)LIS3DH_SENSITIVITY_FS_8G_NORMAL_MODE;
      break;

    case LIS3DH_16g:
      *Sensitivity = (float)LIS3DH_SENSITIVITY_FS_16G_NORMAL_MODE;
      break;

    default:
      ret = LIS3DH_ERROR;
      break;
  }

  return ret;
}

/**
 * @brief  Get the LIS3DH accelerometer sensor sensitivity for LP mode
 * @param  pObj the device pObj
 * @param  Sensitivity pointer to sensitivity
 * @retval 0 in case of success, an error code otherwise
 */
static int32_t LIS3DH_GetSensitivityLP(LIS3DH_Object_t *pObj, float *Sensitivity)
{
  int32_t ret = LIS3DH_OK;
  lis3dh_fs_t fullscale;

  /* Read actual full scale selection from sensor. */
  if (lis3dh_full_scale_get(&(pObj->Ctx), &fullscale) != LIS3DH_OK)
  {
    return LIS3DH_ERROR;
  }

  /* Store the sensitivity based on actual full scale. */
  switch (fullscale)
  {
    case LIS3DH_2g:
      *Sensitivity = (float)LIS3DH_SENSITIVITY_FS_2G_LOW_POWER_MODE;
      break;

    case LIS3DH_4g:
      *Sensitivity = (float)LIS3DH_SENSITIVITY_FS_4G_LOW_POWER_MODE;
      break;

    case LIS3DH_8g:
      *Sensitivity = (float)LIS3DH_SENSITIVITY_FS_8G_LOW_POWER_MODE;
      break;

    case LIS3DH_16g:
      *Sensitivity = (float)LIS3DH_SENSITIVITY_FS_16G_LOW_POWER_MODE;
      break;

    default:
      ret = LIS3DH_ERROR;
      break;
  }

  return ret;
}

/**
 * @brief  Set the LIS3DH accelerometer sensor output data rate when enabled
 * @param  pObj the device pObj
 * @param  Odr the functional output data rate to be set
 * @retval 0 in case of success, an error code otherwise
 */
static int32_t LIS3DH_SetOutputDataRate_When_Enabled(LIS3DH_Object_t *pObj, float Odr)
{
  lis3dh_odr_t new_odr;

  new_odr = (Odr <=    1.0f) ? LIS3DH_ODR_1Hz
            : (Odr <=   10.0f) ? LIS3DH_ODR_10Hz
            : (Odr <=   25.0f) ? LIS3DH_ODR_25Hz
            : (Odr <=   50.0f) ? LIS3DH_ODR_50Hz
            : (Odr <=  100.0f) ? LIS3DH_ODR_100Hz
            : (Odr <=  200.0f) ? LIS3DH_ODR_200Hz
            :                    LIS3DH_ODR_400Hz;

  /* Output data rate selection. */
  if (lis3dh_data_rate_set(&(pObj->Ctx), new_odr) != LIS3DH_OK)
  {
    return LIS3DH_ERROR;
  }

  return LIS3DH_OK;
}

/**
 * @brief  Set the LIS3DH accelerometer sensor output data rate when disabled
 * @param  pObj the device pObj
 * @param  Odr the functional output data rate to be set
 * @retval 0 in case of success, an error code otherwise
 */
static int32_t LIS3DH_SetOutputDataRate_When_Disabled(LIS3DH_Object_t *pObj, float Odr)
{
  pObj->acc_odr = (Odr <=    1.0f) ? LIS3DH_ODR_1Hz
                  : (Odr <=   10.0f) ? LIS3DH_ODR_10Hz
                  : (Odr <=   25.0f) ? LIS3DH_ODR_25Hz
                  : (Odr <=   50.0f) ? LIS3DH_ODR_50Hz
                  : (Odr <=  100.0f) ? LIS3DH_ODR_100Hz
                  : (Odr <=  200.0f) ? LIS3DH_ODR_200Hz
                  :                    LIS3DH_ODR_400Hz;

  return LIS3DH_OK;
}

/**
 * @brief  Wrap Read register component function to Bus IO function
 * @param  Handle the device handler
 * @param  Reg the register address
 * @param  pData the stored data pointer
 * @param  Length the length
 * @retval 0 in case of success, an error code otherwise
 */
static int32_t ReadAccRegWrap(void *Handle, uint8_t Reg, uint8_t *pData, uint16_t Length)
{
  LIS3DH_Object_t *pObj = (LIS3DH_Object_t *)Handle;

  if (pObj->IO.BusType == LIS3DH_I2C_BUS) /* I2C */
  {
    /* Enable Multi-byte read */
    return pObj->IO.ReadReg(pObj->IO.Address, (Reg | 0x80U), pData, Length);
  }
  else   /* SPI 3-Wires */
  {
    /* Enable Multi-byte read */
    return pObj->IO.ReadReg(pObj->IO.Address, (Reg | 0x40U), pData, Length);
  }
}

/**
 * @brief  Wrap Write register component function to Bus IO function
 * @param  Handle the device handler
 * @param  Reg the register address
 * @param  pData the stored data pointer
 * @param  Length the length
 * @retval 0 in case of success, an error code otherwise
 */
static int32_t WriteAccRegWrap(void *Handle, uint8_t Reg, const uint8_t *pData, uint16_t Length)
{
  LIS3DH_Object_t *pObj = (LIS3DH_Object_t *)Handle;

  if (pObj->IO.BusType == LIS3DH_I2C_BUS) /* I2C */
  {
    /* Enable Multi-byte write */
    return pObj->IO.WriteReg(pObj->IO.Address, (Reg | 0x80U), pData, Length);
  }
  else   /* SPI 3-Wires */
  {
    /* Enable Multi-byte write */
    return pObj->IO.WriteReg(pObj->IO.Address, (Reg | 0x40U), pData, Length);
  }
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
