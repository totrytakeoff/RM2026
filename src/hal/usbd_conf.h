/**
  ******************************************************************************
  * @file    usbd_conf.h
  * @author  MCD Application Team
  * @brief   Header file for the usbd_conf.c file
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USBD_CONF_H
#define __USBD_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* USER CODE BEGIN INCLUDE */

/* USER CODE END INCLUDE */

/** @addtogroup STM32_USB_DEVICE_LIBRARY
  * @{
  */
  
/** @defgroup USBD_CONF USBD_CONF
  * @brief Configuration file for USB Device Library
  * @{
  */

/** @defgroup USBD_CONF_Exported_Defines USBD_CONF_Exported_Defines
  * @brief Defines for configuration of the USB device.
  * @{
  */

/*---------- ----------- */
#define USBD_MAX_NUM_INTERFACES     1U
/*---------- ----------- */
#define USBD_MAX_NUM_CONFIGURATION     1U
/*---------- ----------- */
#define USBD_MAX_STR_DESC_SIZ     512U
/*---------- ----------- */
#define USBD_DEBUG_LEVEL     0U
/*---------- ----------- */
#define USBD_SELF_POWERED     1U
/*---------- ----------- */
#define USBD_SUPPORT_USER_STRING_DESC     1U

/****************************************/
/* #define for FS and HS identification */
#define DEVICE_FS 0

/**
  * @}
  */

/** @defgroup USBD_CONF_Exported_Macros USBD_CONF_Exported_Macros
  * @brief Aliases.
  * @{
  */

/* Memory management macros */

/** Alias for memory allocation. */
#define USBD_malloc         (void *)USBD_static_malloc

/** Alias for memory release. */
#define USBD_free           USBD_static_free

/** Alias for delay. */
#define USBD_Delay          HAL_Delay

/* For footprint reasons and since only one allocation is handled in the CDC class
   driver, the malloc/free is changed globally to static allocation method.
 */
void *USBD_static_malloc(uint32_t size);
void USBD_static_free(void *p);

/* Memory management functions */
#define USBD_memset(ptr, value, num) memset((ptr), (value), (num))
#define USBD_memcpy(dest, src, num) memcpy((dest), (src), (num))

/* DEBUG macros */

#if (USBD_DEBUG_LEVEL > 0)
#define  USBD_UsrLog(...)   printf(__VA_ARGS__);\
                            printf("\n");
#else
#define USBD_UsrLog(...)
#endif

#if (USBD_DEBUG_LEVEL > 1)

#define  USBD_ErrLog(...)   printf("ERROR: ") ;\
                            printf(__VA_ARGS__);\
                            printf("\n");
#else
#define USBD_ErrLog(...)
#endif

#if (USBD_DEBUG_LEVEL > 2)
#define  USBD_DbgLog(...)   printf("DEBUG : ") ;\
                            printf(__VA_ARGS__);\
                            printf("\n");
#else
#define USBD_DbgLog(...)
#endif

/**
  * @}
  */

/** @defgroup USBD_CONF_Exported_Types USBD_CONF_Exported_Types
  * @brief Types.
  * @{
  */

/**
  * @}
  */

/** @defgroup USBD_CONF_Exported_Variables USBD_CONF_Exported_Variables
  * @brief Public variables.
  * @{
  */

/**
  * @}
  */

/** @defgroup USBD_CONF_Exported_FunctionsPrototype USBD_CONF_Exported_FunctionsPrototype
  * @brief Declaration of public functions for USB device.
  * @{
  */

/* Exported functions -------------------------------------------------------*/

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __USBD_CONF_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
