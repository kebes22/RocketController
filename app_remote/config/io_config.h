#ifndef IO_CONFIG_H
#define IO_CONFIG_H


#ifdef DEV_BOARD
#define BLE_MODULE	BLE_MODULE_NORDIC_PCA10040
#else
#define BLE_MODULE	BLE_MODULE_RAYTAC_MDBT42Q
#endif

#include "module_pinouts.h"
//#include "nrf_saadc.h"

// Hardware version is set in the preprocessor defines.
#define HW_VERSION BOARD_VERSION

#if BLE_MODULE == BLE_MODULE_RAYTAC_MDBT42Q
// Inputs
#define BUTTON1_IN_PIN			4
//#define BUTTON2_IN_PIN			22		// Center nav button
#define BUTTON2_IN_PIN			19		// SW1

#define I2C_SCL_PIN				28		// I2C clock pin
#define I2C_SDA_PIN				27		// I2C data pin.
// FIXME dummy pins
//#define I2C_SCL_PIN				6		// I2C clock pin
//#define I2C_SDA_PIN				7		// I2C data pin.

#define CHG_PG_PIN				3
#define CHG_STAT1_PIN			26
#define CHG_STAT2_PIN			25


// Outputs
#define PWR_ON_PIN				5
#define EN_5V_PIN				16
#define LED_DAT_PIN				18


// Analog
#define VBATT_AN_IN				NRF_SAADC_INPUT_AIN0

#elif BLE_MODULE == BLE_MODULE_NORDIC_PCA10040

#endif

#endif // IO_CONFIG_H