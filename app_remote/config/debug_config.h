#ifndef DEBUG_CONFIG_H
#define DEBUG_CONFIG_H

#if DEBUG

//################################################################################
//	Debugging options
//################################################################################
#define NEVER_SLEEP


//################################################################################
//	NRF LOG Debugging levels per module
//################################################################################
/* Log Levels are:
	0 = No logging
	1 = Errors only
	2 = Warnings and above
	3 = Info and above
	4 = Debug and above
	5 = Trace level (must include log.h from lib)
*/

#define NRF_LOG_ENABLED						1
#define NRF_LOG_STACK_SIZE					256

// NOTE this is the maximal allowed log level, _not_ the default
#define NRF_LOG_DEFAULT_LEVEL				4


#define NRF_SDH_BLE_LOG_ENABLED				0

#define	ADVERT_LOG_LEVEL					3

#define UI_LOG_LEVEL						3
#define LIS3DH_LOG_LEVEL					3
#define TXRX_COMMS_LOG_LEVEL				4




#define SPI_FLASH_LOG_LEVEL					3
#define FDS_API_LOG_LEVEL					3
#define FLASH_VAR_LOG_LEVEL					3
#define TRIG_LOG_LEVEL						3
#define SCHED_LOG_LEVEL						3
#define SCHED_CB_LOG_LEVEL					3
#define DATE_LOG_LEVEL						3
#define RP_SYS_LOG_LEVEL					3
#define SENSORS_LOG_LEVEL					3
#define BME280_LOG_LEVEL					3
#define ALS_LOG_LEVEL						3
#define SHT30_LOG_LEVEL						3
#define LSM_LOG_LEVEL						3
#define HW_LOG_LEVEL						3
#define SELF_TEST_LOG_LEVEL					3

#define NRFX_TWI_CONFIG_LOG_ENABLED			0
#define TWI_CONFIG_LOG_ENABLED				0

#define PIC_BOOT_LOG_LEVEL					3
#define NP_COMMS_LOG_LEVEL					3
#define MOT_COMMS_LOG_LEVEL					3


#define SCHEDULE_DEBUG						2 // Special logging level for schedule debugging messages (0-4)


#else // DEBUG

#define NRF_LOG_ENABLED						0

#endif // DEBUG

#endif // DEBUG_CONFIG_H
