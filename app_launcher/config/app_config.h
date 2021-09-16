#if defined(APP_CONFIG_H) || !defined(SDK_CONFIG_H)
#error Do not include app_config.h directly, instead include sdk_common.h
#endif

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

//################################################################################
//	Device/Version information
//################################################################################
#define DEVICE_ID				DEV_ROCKET_CON
#define PERIPH_POWER_PROFILE	PERIPH_POWER_PROFILE_LOW

#define APP_MAJOR				0			/* Major revision. Major/minor combination must be unique on firmware releases, because these are the only fields returned by the get firmware version command*/
#define APP_MINOR				15			/* Minor revision. */
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

#define APP_MAX_PAYLOAD_SIZE	128

//################################################################################
//	NRF Defines
//################################################################################
//TODO these should either be merged in to sdk_config.h, or into a separate app specific include

#define APP_FIFO_ENABLED 1
#define APP_UART_ENABLED 1
#define APP_UART_DRIVER_INSTANCE 0

#define NRF_GFX_ENABLED 1


#define ST7565_ENABLED			1

#define ST7565_SCK_PIN			1
#define ST7565_MOSI_PIN			0
#define ST7565_SS_PIN			8
#define ST7565_A0_PIN			6
#define ST7565_RESET_PIN		7

#define ST7565_SPI_INSTANCE		2
#define SPI2_ENABLED			1
#define SPI2_USE_EASY_DMA		1

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
//	Central
//################################################################################

#define TXRX_BLE_USE_CENTRAL			1

// <e> NRF_BLE_SCAN_ENABLED - nrf_ble_scan - Scanning Module
//==========================================================
#ifndef NRF_BLE_SCAN_ENABLED
#define NRF_BLE_SCAN_ENABLED 1
#endif
// <o> NRF_BLE_SCAN_BUFFER - Data length for an advertising set.
#ifndef NRF_BLE_SCAN_BUFFER
#define NRF_BLE_SCAN_BUFFER 31
#endif

// <o> NRF_BLE_SCAN_NAME_MAX_LEN - Maximum size for the name to search in the advertisement report.
#ifndef NRF_BLE_SCAN_NAME_MAX_LEN
#define NRF_BLE_SCAN_NAME_MAX_LEN 32
#endif

// <o> NRF_BLE_SCAN_SHORT_NAME_MAX_LEN - Maximum size of the short name to search for in the advertisement report.
#ifndef NRF_BLE_SCAN_SHORT_NAME_MAX_LEN
#define NRF_BLE_SCAN_SHORT_NAME_MAX_LEN 32
#endif

// <o> NRF_BLE_SCAN_SCAN_INTERVAL - Scanning interval. Determines the scan interval in units of 0.625 millisecond.
#ifndef NRF_BLE_SCAN_SCAN_INTERVAL
#define NRF_BLE_SCAN_SCAN_INTERVAL 160
#endif

// <o> NRF_BLE_SCAN_SCAN_DURATION - Duration of a scanning session in units of 10 ms. Range: 0x0001 - 0xFFFF (10 ms to 10.9225 ms). If set to 0x0000, the scanning continues until it is explicitly disabled.
#ifndef NRF_BLE_SCAN_SCAN_DURATION
#define NRF_BLE_SCAN_SCAN_DURATION 0
#endif

// <o> NRF_BLE_SCAN_SCAN_WINDOW - Scanning window. Determines the scanning window in units of 0.625 millisecond.
#ifndef NRF_BLE_SCAN_SCAN_WINDOW
#define NRF_BLE_SCAN_SCAN_WINDOW 80
#endif

// <o> NRF_BLE_SCAN_MIN_CONNECTION_INTERVAL - Determines minimum connection interval in milliseconds.
#ifndef NRF_BLE_SCAN_MIN_CONNECTION_INTERVAL
#define NRF_BLE_SCAN_MIN_CONNECTION_INTERVAL 7.5
#endif

// <o> NRF_BLE_SCAN_MAX_CONNECTION_INTERVAL - Determines maximum connection interval in milliseconds.
#ifndef NRF_BLE_SCAN_MAX_CONNECTION_INTERVAL
//#define NRF_BLE_SCAN_MAX_CONNECTION_INTERVAL 30
#define NRF_BLE_SCAN_MAX_CONNECTION_INTERVAL 15
#endif

// <o> NRF_BLE_SCAN_SLAVE_LATENCY - Determines the slave latency in counts of connection events.
#ifndef NRF_BLE_SCAN_SLAVE_LATENCY
#define NRF_BLE_SCAN_SLAVE_LATENCY 0
#endif

// <o> NRF_BLE_SCAN_SUPERVISION_TIMEOUT - Determines the supervision time-out in units of 10 millisecond.
#ifndef NRF_BLE_SCAN_SUPERVISION_TIMEOUT
#define NRF_BLE_SCAN_SUPERVISION_TIMEOUT 4000
#endif

// <o> NRF_BLE_SCAN_SCAN_PHY  - PHY to scan on.

// <0=> BLE_GAP_PHY_AUTO
// <1=> BLE_GAP_PHY_1MBPS
// <2=> BLE_GAP_PHY_2MBPS
// <4=> BLE_GAP_PHY_CODED
// <255=> BLE_GAP_PHY_NOT_SET

#ifndef NRF_BLE_SCAN_SCAN_PHY
#define NRF_BLE_SCAN_SCAN_PHY 1
#endif

// <e> NRF_BLE_SCAN_FILTER_ENABLE - Enabling filters for the Scanning Module.
//==========================================================
#ifndef NRF_BLE_SCAN_FILTER_ENABLE
#define NRF_BLE_SCAN_FILTER_ENABLE 1
#endif
// <o> NRF_BLE_SCAN_UUID_CNT - Number of filters for UUIDs.
#ifndef NRF_BLE_SCAN_UUID_CNT
#define NRF_BLE_SCAN_UUID_CNT 1
#endif

// <o> NRF_BLE_SCAN_NAME_CNT - Number of name filters.
#ifndef NRF_BLE_SCAN_NAME_CNT
#define NRF_BLE_SCAN_NAME_CNT 0
#endif

// <o> NRF_BLE_SCAN_SHORT_NAME_CNT - Number of short name filters.
#ifndef NRF_BLE_SCAN_SHORT_NAME_CNT
#define NRF_BLE_SCAN_SHORT_NAME_CNT 0
#endif

// <o> NRF_BLE_SCAN_ADDRESS_CNT - Number of address filters.
#ifndef NRF_BLE_SCAN_ADDRESS_CNT
#define NRF_BLE_SCAN_ADDRESS_CNT 0
#endif

// <o> NRF_BLE_SCAN_APPEARANCE_CNT - Number of appearance filters.
#ifndef NRF_BLE_SCAN_APPEARANCE_CNT
#define NRF_BLE_SCAN_APPEARANCE_CNT 0
#endif

// </e>

// </e>


// <o> NRF_BLE_SCAN_OBSERVER_PRIO
// <i> Priority for dispatching the BLE events to the Scanning Module.

#ifndef NRF_BLE_SCAN_OBSERVER_PRIO
#define NRF_BLE_SCAN_OBSERVER_PRIO 1
#endif



#ifndef BLE_DB_DISCOVERY_ENABLED
#define BLE_DB_DISCOVERY_ENABLED 1
#endif

// <o> NRF_SDH_BLE_CENTRAL_LINK_COUNT - Maximum number of central links.
#ifndef NRF_SDH_BLE_CENTRAL_LINK_COUNT
#define NRF_SDH_BLE_CENTRAL_LINK_COUNT 1
#endif


//################################################################################
//	Debug
//################################################################################

#include "debug_config.h"

//################################################################################
//	Other options
//################################################################################

#define NRFX_PPI_ENABLED 1
#define PPI_ENABLED 1


#include "lib_common.h"
#include "io_config.h"

#endif // APP_CONFIG_H
