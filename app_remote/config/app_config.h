#if defined(APP_CONFIG_H) || !defined(SDK_CONFIG_H)
#error Do not include app_config.h directly, instead include sdk_common.h
#endif

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

//################################################################################
//	Device/Version information
//################################################################################
#define DEVICE_ID				DEV_ROCKET_REM
#define PERIPH_POWER_PROFILE	PERIPH_POWER_PROFILE_LOW

#define APP_MAJOR				0			/* Major revision. Major/minor combination must be unique on firmware releases, because these are the only fields returned by the get firmware version command*/
#define APP_MINOR				1			/* Minor revision. */
#define APP_PATCH				0			/* Patch version. Use this to change file name on server (i.e bad zip file), but indicating no firmware change (rarely used) */
#define DEV_LABEL				BETA		/* Options are: STABLE, RC, BETA, DEV_BRD */
#define DEV_LABEL_VER			1			/* STABLE versions should have 0 here */

#define HW_MAJOR				1			/* Hardware revision major */
#define HW_MINOR				0			/* Hardware revision minor */

#define COMMS_PROTOCOL_MAJOR	1			/* Major version changes will require different application support */
#define COMMS_PROTOCOL_MINOR	0			/* Minor versions do not affect app comms interface significantly */

//################################################################################
//	Comms info
//################################################################################

//#define APP_MAX_PAYLOAD_SIZE	128

//################################################################################
//	NRF Defines
//################################################################################
//TODO these should either be merged in to sdk_config.h, or into a separate app specific include

// <q> APP_FIFO_ENABLED  - app_fifo - Software FIFO implementation

#if 0
#ifndef APP_FIFO_ENABLED
#define APP_FIFO_ENABLED 1
#endif

// <e> APP_UART_ENABLED - app_uart - UART driver
//==========================================================
#ifndef APP_UART_ENABLED
#define APP_UART_ENABLED 1
#endif
// <o> APP_UART_DRIVER_INSTANCE  - UART instance used

// <0=> 0

#ifndef APP_UART_DRIVER_INSTANCE
#define APP_UART_DRIVER_INSTANCE 0
#endif
#endif

//################################################################################
//	NRF Defines for FLASH
//################################################################################


#define POWER_ENABLED 1
#define NRFX_POWER_ENABLED	1
#define NRFX_POWER_CONFIG_DEFAULT_DCDCEN 1
#define NRF_PWR_MGMT_ENABLED 1
#define NRF_PWR_MGMT_CONFIG_STANDBY_TIMEOUT_ENABLED 0
#define NRF_PWR_MGMT_CONFIG_STANDBY_TIMEOUT_S 5
#define NRF_PWR_MGMT_CONFIG_FPU_SUPPORT_ENABLED 1

#define NRFX_TWIM_CONFIG_LOG_ENABLED 1


//################################################################################
//	NRF Defines for internal oscillator
//################################################################################

#define USE_INTERNAL_OSCILLATOR

#ifdef  USE_INTERNAL_OSCILLATOR
#define NRF_SDH_CLOCK_LF_SRC			0
#define NRF_SDH_CLOCK_LF_RC_CTIV		16
#define NRF_SDH_CLOCK_LF_RC_TEMP_CTIV	2
#define NRF_SDH_CLOCK_LF_ACCURACY		1
#endif

//################################################################################
//	Debug
//################################################################################

#include "debug_config.h"

//################################################################################
//	Other options
//################################################################################

#ifndef BUTTON_DFU_LAUNCH
#define BUTTON_DFU_LAUNCH
#endif

#include "lib_common.h"
#include "io_config.h"

#endif // APP_CONFIG_H
