#ifndef IO_CONFIG_H
#define IO_CONFIG_H

//#include "sdk_common.h"

#ifdef DEV_BOARD
#define BLE_MODULE	BLE_MODULE_NORDIC_PCA10040
#else
#define BLE_MODULE	BLE_MODULE_RAYTAC_MDBT42Q
#endif

#include "module_pinouts.h"

// Hardware version is set in the preprocessor defines.
#define HW_VERSION BOARD_VERSION

#if HW_VERSION == 01

//----------------------------------------
// LCD Module Pins
//----------------------------------------
// Inputs
#define BUTTON1_IN_PIN			P34

// Outputs
#define PWR_ON_PIN				P32
#define LED_PWM_PIN				P02

#define LCD_SCK_PIN				P13
#define LCD_MOSI_PIN			P14
#define LCD_CS_PIN				P21
#define LCD_A0_PIN				P19
#define LCD_RESET_PIN			P20

#define CHG_PG_PIN				P41
#define CHG_STAT1_PIN			P40
#define CHG_STAT2_PIN			P38

// Analog
#define VBATT_AN_IN				NRF_SAADC_INPUT_AIN7

//----------------------------------------
// External Pins
//----------------------------------------
#define BUTTON2_IN_PIN			26

// Analog
//#define VBATT_12_AN_IN			NRF_SAADC_INPUT_AIN7

#define AN_PRESSURE_IN			NRF_SAADC_INPUT_AIN3

#define RELAY_1_OUT_PIN			4
#define RELAY_2_OUT_PIN			9
#define RELAY_3_OUT_PIN			17
#define RELAY_4_OUT_PIN			11

#define TP1_PIN					P35
#define TP1_INIT()	nrf_gpio_cfg_output(TP1_PIN)
#define TP1_ON()	nrf_gpio_pin_write(TP1_PIN,1)
#define TP1_OFF()	nrf_gpio_pin_write(TP1_PIN,0)

#elif HW_VERSION == 10

//----------------------------------------
// LCD Module Pins
//----------------------------------------
// Inputs
#define BUTTON1_IN_PIN			20

// Outputs
#define PWR_ON_PIN				18
#define LED_PWM_PIN				25

#define LCD_SCK_PIN				1
#define LCD_MOSI_PIN			0
#define LCD_CS_PIN				8
#define LCD_A0_PIN				6
#define LCD_RESET_PIN			7

#define CHG_PG_PIN				23
#define CHG_STAT1_PIN			24
#define CHG_STAT2_PIN			22

// Analog
#define VBATT_AN_IN				NRF_SAADC_INPUT_AIN7

//----------------------------------------
// External Pins
//----------------------------------------
//#define BUTTON2_IN_PIN			2
#define BUTTON2_IN_PIN			2

// Analog
#define VBATT_12_AN_IN			NRF_SAADC_INPUT_AIN4
#define EXT_AN_IN_1				NRF_SAADC_INPUT_AIN3
#define EXT_AN_IN_2				NRF_SAADC_INPUT_AIN2
#define EXT_AN_IN_3				NRF_SAADC_INPUT_AIN1
#define EXT_AN_IN_4				NRF_SAADC_INPUT_AIN0

#define AN_PRESSURE_IN			EXT_AN_IN_1


#define RELAY_1_OUT_PIN			19
#define RELAY_2_OUT_PIN			17
#define RELAY_3_OUT_PIN			29
#define RELAY_4_OUT_PIN			27

#define RELAY_1_IN_PIN			10
#define RELAY_2_IN_PIN			11
#define RELAY_3_IN_PIN			30
#define RELAY_4_IN_PIN			26

#define LED_DAT_PIN				9
//#define LED_DUMMY1_PIN			29	// Relay pin
//#define LED_DUMMY2_PIN			27	//
#define LED_DUMMY1_PIN			14	// SDC pins
#define LED_DUMMY2_PIN			15	// SDC pins
//#define LED_DUMMY1_PIN			14
//#define LED_DUMMY2_PIN			15
//#define LED_DUMMY1_PIN			0xFF
//#define LED_DUMMY2_PIN			0xFF

#define BEEPER_PIN				21
#define RELAY_5_OUT_PIN			BEEPER_PIN
#define RELAY_5_IN_PIN			0xFF

#define TP1_PIN					21
#define TP1_INIT()	nrf_gpio_cfg_output(TP1_PIN)
//#define TP1_ON()	nrf_gpio_pin_write(TP1_PIN,0)
//#define TP1_OFF()	nrf_gpio_pin_write(TP1_PIN,0)
//#define TP1_INIT()
#define TP1_ON()
#define TP1_OFF()

#elif BLE_MODULE == BLE_MODULE_NORDIC_PCA10040


#endif

#endif // IO_CONFIG_H