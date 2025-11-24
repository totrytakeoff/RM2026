/**
  ******************************************************************************
  * @file           : usbd_desc.h
  * @version        : v1.0_Cube
  * @brief          : Header for usbd_desc.c file.
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USBD_DESC__H__
#define __USBD_DESC__H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usbd_def.h"

/* USER CODE BEGIN INCLUDE */

/* USER CODE END INCLUDE */

/** @addtogroup USBD_DESC USBD_DESC
  * @brief USB descriptor definitions.
  * @{
  */

/** @defgroup USBD_DESC_Exported_Constants USBD_DESC_Exported_Constants
  * @brief Constants.
  * @{
  */

/* Descriptor types */
#define USB_DESC_TYPE_DEVICE                      0x01U
#define USB_DESC_TYPE_CONFIGURATION               0x02U
#define USB_DESC_TYPE_STRING                      0x03U
#define USB_DESC_TYPE_INTERFACE                   0x04U
#define USB_DESC_TYPE_ENDPOINT                    0x05U
#define USB_DESC_TYPE_DEVICE_QUALIFIER            0x06U
#define USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION   0x07U
#define USB_DESC_TYPE_BOS                         0x0FU

/* BOS descriptor types */
#define USB_DEVICE_CAPABITY_TYPE                 0x10U

/* USB Specification Release Number */
#define USB_BCD_DEVICE                           0x0200U

/* USB LPM */
#define USBD_IDX_LPM_IDX                         0x00U
#define USBD_IDX_LPM_DESC                        0x00U

/* USB LPM */
#if (USBD_LPM_ENABLED == 1)
#define USBD_LPM_ATTRIBUTES        (USB_BMATTRIBUTE_DSE0 | USB_BMATTRIBUTE_ISOU | \
                                   USB_BMATTRIBUTE_ISOP | USB_BMATTRIBUTE_ISOI)
#else
#define USBD_LPM_ATTRIBUTES        0x00U
#endif /* (USBD_LPM_ENABLED == 1) */

/* USB String Index */
#define USBD_IDX_MFC_STR                      0x01U
#define USBD_IDX_PRODUCT_STR                  0x02U
#define USBD_IDX_SERIAL_STR                   0x03U
#define USBD_IDX_CONFIG_STR                   0x04U
#define USBD_IDX_INTERFACE_STR                0x05U

/* USB Device Descriptor */
#define USBD_DEVICE_ITF_MAX_NUM               1U
#define USBD_MAX_STR_DESC_SIZ                 0x100U

/* USB Device Descriptor */
#define USBD_DEVICE_QUALIFIER                 0x00U
#define USBD_MAX_POWER                        0x32U

/* USB Device Descriptor */
#define USBD_VID                              1155U
#define USBD_LANGID_STRING                    0x0409U
#define USBD_MANUFACTURER_STRING              "STMicroelectronics"
#define USBD_PID_FS                           22336U
#define USBD_PRODUCT_STRING_FS                "STM32 Virtual ComPort"
#define USBD_CONFIGURATION_STRING_FS          "CDC Config"
#define USBD_INTERFACE_STRING_FS              "CDC Interface"

/* USER CODE BEGIN EXPORTED_CONSTANTS */

/* USER CODE END EXPORTED_CONSTANTS */

/**
  * @}
  */

/** @defgroup USBD_DESC_Exported_Macros USBD_DESC_Exported_Macros
  * @brief Macros.
  * @{
  */

/* USER CODE BEGIN EXPORTED_MACRO */

/* USER CODE END EXPORTED_MACRO */

/**
  * @}
  */

/** @defgroup USBD_DESC_Exported_Types USBD_DESC_Exported_Types
  * @brief Types.
  * @{
  */

/* USER CODE BEGIN EXPORTED_TYPES */

/* USER CODE END EXPORTED_TYPES */

/**
  * @}
  */

/** @defgroup USBD_DESC_Exported_Variables USBD_DESC_Exported_Variables
  * @brief Public variables.
  * @{
  */

/** Descriptor for the Usb device. */
extern USBD_DescriptorsTypeDef FS_Desc;

/* USER CODE BEGIN EXPORTED_VARIABLES */

/* USER CODE END EXPORTED_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_DESC_Exported_FunctionsPrototype USBD_DESC_Exported_FunctionsPrototype
  * @brief Public functions declaration.
  * @{
  */

/* USER CODE BEGIN EXPORTED_FUNCTIONS */

/* USER CODE END EXPORTED_FUNCTIONS */

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

#endif /* __USBD_DESC__H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
