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
#define BUTTON_IN_PIN			P15

#define I2C_SCL_PIN				P28		// I2C clock pin
#define I2C_SDA_PIN				P27		// I2C data pin.

#define ACCEL_INT1_PIN			P25
#define ACCEL_INT2_PIN			P26


#define CHG_PG_PIN				P16
#define CHG_STAT1_PIN			P18
#define CHG_STAT2_PIN			P17


// Outputs
#define PWR_ON_PIN				P14
#define EN_5V_PIN				P04
#define LED_DAT_PIN				P07


// Motors
#define MOTOR_1_SLEEP_PIN		P32
#define MOTOR_1_EN_P1_PIN		P33
#define MOTOR_1_PH_P2_PIN		P34

#define MOTOR_2_SLEEP_PIN		P29
#define MOTOR_2_EN_P1_PIN		P30
#define MOTOR_2_PH_P2_PIN		P31

#define MOTOR_3_SLEEP_PIN		P21
#define MOTOR_3_EN_P1_PIN		P20
#define MOTOR_3_PH_P2_PIN		P19

// Analog
#define VBATT_AN_IN				NRF_SAADC_INPUT_AIN7

#elif BLE_MODULE == BLE_MODULE_NORDIC_PCA10040

// Inputs
#define BUTTON_IN_PIN			13

#define I2C_SCL_PIN				27		// I2C clock pin
#define I2C_SDA_PIN				26		// I2C data pin.

#define ACCEL_INT1_PIN			0xFFFFFFFF
#define ACCEL_INT2_PIN			0xFFFFFFFF


#define CHG_PG_PIN				14
#define CHG_STAT1_PIN			15
#define CHG_STAT2_PIN			16


// Outputs
//#define PWR_ON_PIN				18
//#define EN_5V_PIN				9
//#define LED_DAT_PIN				17

#define PWR_ON_PIN				23
#define EN_5V_PIN				9
#define LED_DAT_PIN				22



// Motors
#define MOTOR_1_SLEEP_PIN		25
#define MOTOR_1_EN_P1_PIN		19
#define MOTOR_1_PH_P2_PIN		20

#define MOTOR_2_SLEEP_PIN		24
#define MOTOR_2_EN_P1_PIN		17
#define MOTOR_2_PH_P2_PIN		18

#define MOTOR_3_SLEEP_PIN		0xFF
#define MOTOR_3_EN_P1_PIN		0xFF
#define MOTOR_3_PH_P2_PIN		0xFF

// Analog
#define VBATT_AN_IN				NRF_SAADC_INPUT_AIN0

#endif

#endif // IO_CONFIG_H