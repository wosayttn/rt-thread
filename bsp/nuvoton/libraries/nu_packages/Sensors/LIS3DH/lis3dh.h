/**
 ******************************************************************************
 * @file    lis3dh.h
 * @author  MEMS Software Solutions Team
 * @brief   LIS3DH header driver file
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef LIS3DH_H
#define LIS3DH_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "lis3dh_reg.h"
#include <string.h>

/** @addtogroup BSP BSP
 * @{
 */

/** @addtogroup Component Component
 * @{
 */

/** @addtogroup LIS3DH LIS3DH
 * @{
 */

/** @defgroup LIS3DH_Exported_Types LIS3DH Exported Types
 * @{
 */

typedef int32_t (*LIS3DH_Init_Func)(void);
typedef int32_t (*LIS3DH_DeInit_Func)(void);
typedef int32_t (*LIS3DH_GetTick_Func)(void);
typedef int32_t (*LIS3DH_WriteReg_Func)(uint16_t, uint16_t, const uint8_t *, uint16_t);
typedef int32_t (*LIS3DH_ReadReg_Func)(uint16_t, uint16_t, uint8_t *, uint16_t);

typedef struct
{
  LIS3DH_Init_Func           Init;
  LIS3DH_DeInit_Func         DeInit;
  uint32_t                     BusType; /*0 means I2C, 1 means SPI-3-Wires */
  uint8_t                      Address;
  LIS3DH_WriteReg_Func       WriteReg;
  LIS3DH_ReadReg_Func        ReadReg;
  LIS3DH_GetTick_Func        GetTick;
} LIS3DH_IO_t;


typedef struct
{
  int16_t x;
  int16_t y;
  int16_t z;
} LIS3DH_AxesRaw_t;

typedef struct
{
  int32_t x;
  int32_t y;
  int32_t z;
} LIS3DH_Axes_t;

typedef struct
{
  LIS3DH_IO_t         IO;
  stmdev_ctx_t          Ctx;
  uint8_t               is_initialized;
  uint8_t               acc_is_enabled;
  lis3dh_odr_t        acc_odr;
} LIS3DH_Object_t;

typedef struct
{
  uint8_t   Acc;
  uint8_t   Gyro;
  uint8_t   Magneto;
  uint8_t   LowPower;
  uint32_t  GyroMaxFS;
  uint32_t  AccMaxFS;
  uint32_t  MagMaxFS;
  float     GyroMaxOdr;
  float     AccMaxOdr;
  float     MagMaxOdr;
} LIS3DH_Capabilities_t;

typedef struct
{
  int32_t (*Init)(LIS3DH_Object_t *);
  int32_t (*DeInit)(LIS3DH_Object_t *);
  int32_t (*ReadID)(LIS3DH_Object_t *, uint8_t *);
  int32_t (*GetCapabilities)(LIS3DH_Object_t *, LIS3DH_Capabilities_t *);
} LIS3DH_CommonDrv_t;

typedef struct
{
  int32_t (*Enable)(LIS3DH_Object_t *);
  int32_t (*Disable)(LIS3DH_Object_t *);
  int32_t (*GetSensitivity)(LIS3DH_Object_t *, float *);
  int32_t (*GetOutputDataRate)(LIS3DH_Object_t *, float *);
  int32_t (*SetOutputDataRate)(LIS3DH_Object_t *, float);
  int32_t (*GetFullScale)(LIS3DH_Object_t *, int32_t *);
  int32_t (*SetFullScale)(LIS3DH_Object_t *, int32_t);
  int32_t (*GetAxes)(LIS3DH_Object_t *, LIS3DH_Axes_t *);
  int32_t (*GetAxesRaw)(LIS3DH_Object_t *, LIS3DH_AxesRaw_t *);
} LIS3DH_Drv_t;

typedef union{
  int16_t i16bit[3];
  uint8_t u8bit[6];
} lis3dh_axis3bit16_t;

typedef union{
  int16_t i16bit;
  uint8_t u8bit[2];
} lis3dh_axis1bit16_t;

typedef union{
  int32_t i32bit[3];
  uint8_t u8bit[12];
} lis3dh_axis3bit32_t;

typedef union{
  int32_t i32bit;
  uint8_t u8bit[4];
} lis3dh_axis1bit32_t;

/**
 * @}
 */

/** @defgroup LIS3DH_Exported_Constants LIS3DH Exported Constants
 * @{
 */

#define LIS3DH_OK                     0
#define LIS3DH_ERROR                 -1

#define LIS3DH_I2C_BUS               0U
#define LIS3DH_SPI_4WIRES_BUS        1U
#define LIS3DH_SPI_3WIRES_BUS        2U

#define LIS3DH_SENSITIVITY_FS_2G_NORMAL_MODE               3.900f  /**< Sensitivity value for 2 g full scale and normal mode [mg/LSB] */
#define LIS3DH_SENSITIVITY_FS_2G_HIGH_RESOLUTION_MODE      0.980f  /**< Sensitivity value for 2 g full scale and high resolution mode [mg/LSB] */
#define LIS3DH_SENSITIVITY_FS_2G_LOW_POWER_MODE           15.630f  /**< Sensitivity value for 2 g full scale and low power mode [mg/LSB] */
#define LIS3DH_SENSITIVITY_FS_4G_NORMAL_MODE               7.820f  /**< Sensitivity value for 4 g full scale and normal mode [mg/LSB] */
#define LIS3DH_SENSITIVITY_FS_4G_HIGH_RESOLUTION_MODE      1.950f  /**< Sensitivity value for 4 g full scale and high resolution mode [mg/LSB] */
#define LIS3DH_SENSITIVITY_FS_4G_LOW_POWER_MODE           31.260f  /**< Sensitivity value for 4 g full scale and low power mode [mg/LSB] */
#define LIS3DH_SENSITIVITY_FS_8G_NORMAL_MODE              15.630f  /**< Sensitivity value for 8 g full scale and normal mode [mg/LSB] */
#define LIS3DH_SENSITIVITY_FS_8G_HIGH_RESOLUTION_MODE      3.900f  /**< Sensitivity value for 8 g full scale and high resolution mode [mg/LSB] */
#define LIS3DH_SENSITIVITY_FS_8G_LOW_POWER_MODE           62.520f  /**< Sensitivity value for 8 g full scale and low power mode [mg/LSB] */
#define LIS3DH_SENSITIVITY_FS_16G_NORMAL_MODE             46.900f  /**< Sensitivity value for 16 g full scale and normal mode [mg/LSB] */
#define LIS3DH_SENSITIVITY_FS_16G_HIGH_RESOLUTION_MODE    11.720f  /**< Sensitivity value for 16 g full scale and high resolution mode [mg/LSB] */
#define LIS3DH_SENSITIVITY_FS_16G_LOW_POWER_MODE         187.580f  /**< Sensitivity value for 16 g full scale and low power mode [mg/LSB] */

#define LIS3DH_MAG_SENSITIVITY_FS_50GAUSS  1.500f  /**< Sensitivity value for 50 gauss full scale [mgauss/LSB] */

/**
 * @}
 */

/** @defgroup LIS3DH_Exported_Functions LIS3DH Exported Functions
 * @{
 */

int32_t LIS3DH_RegisterBusIO(LIS3DH_Object_t *pObj, LIS3DH_IO_t *pIO);
int32_t LIS3DH_Init(LIS3DH_Object_t *pObj);
int32_t LIS3DH_DeInit(LIS3DH_Object_t *pObj);
int32_t LIS3DH_ReadID(LIS3DH_Object_t *pObj, uint8_t *Id);
int32_t LIS3DH_GetCapabilities(LIS3DH_Object_t *pObj, LIS3DH_Capabilities_t *Capabilities);

int32_t LIS3DH_Enable(LIS3DH_Object_t *pObj);
int32_t LIS3DH_Disable(LIS3DH_Object_t *pObj);
int32_t LIS3DH_GetSensitivity(LIS3DH_Object_t *pObj, float *sensitivity);
int32_t LIS3DH_GetOutputDataRate(LIS3DH_Object_t *pObj, float *odr);
int32_t LIS3DH_SetOutputDataRate(LIS3DH_Object_t *pObj, float odr);
int32_t LIS3DH_GetFullScale(LIS3DH_Object_t *pObj, int32_t *fullscale);
int32_t LIS3DH_SetFullScale(LIS3DH_Object_t *pObj, int32_t fullscale);
int32_t LIS3DH_GetAxes(LIS3DH_Object_t *pObj, LIS3DH_Axes_t *acceleration);
int32_t LIS3DH_GetAxesRaw(LIS3DH_Object_t *pObj, LIS3DH_AxesRaw_t *value);

int32_t LIS3DH_Read_Reg(LIS3DH_Object_t *pObj, uint8_t reg, uint8_t *data);
int32_t LIS3DH_Write_Reg(LIS3DH_Object_t *pObj, uint8_t reg, uint8_t data);

int32_t LIS3DH_Get_DRDY_Status(LIS3DH_Object_t *pObj, uint8_t *status);
int32_t LIS3DH_Get_Init_Status(LIS3DH_Object_t *pObj, uint8_t *status);

/**
 * @}
 */

/** @addtogroup LIS3DH_Exported_Variables LIS3DH Exported Variables
 * @{
 */

extern LIS3DH_CommonDrv_t LIS3DH_COMMON_Driver;
extern LIS3DH_Drv_t LIS3DH_Driver;

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif

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
