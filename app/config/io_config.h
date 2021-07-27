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

#if BLE_MODULE == BLE_MODULE_RAYTAC_MDBT42Q

// Inputs
#define BUTTON1_IN_PIN			P34
#define BUTTON2_IN_PIN			26
//#define BUTTON1_J1_PIN			P33
//#define BUTTON1_J2_PIN			P04
//#define BUTTON_J1B1_PIN			19
//#define BUTTON_J2B1_PIN			27

// Outputs
#define PWR_ON_PIN				P32
#define LED_PWM_PIN				P02

#define LCD_SCK_PIN				P13
#define LCD_MOSI_PIN			P14
#define LCD_CS_PIN				P21
#define LCD_A0_PIN				P19
#define LCD_RESET_PIN			P20

// Analog
#define VBATT_AN_IN				NRF_SAADC_INPUT_AIN7
//#define VBATT_JOY1X_IN			NRF_SAADC_INPUT_AIN3
//#define VBATT_JOY1Y_IN			NRF_SAADC_INPUT_AIN2
//#define VBATT_JOY2X_IN			NRF_SAADC_INPUT_AIN5
//#define VBATT_JOY2Y_IN			NRF_SAADC_INPUT_AIN4

//#define VBATT_JOY1_TRIM_IN		NRF_SAADC_INPUT_AIN1
//#define VBATT_JOY2_TRIM_IN		NRF_SAADC_INPUT_AIN6

#define AN_PRESSURE_IN			NRF_SAADC_INPUT_AIN3

#define RELAY_1_OUT_PIN			4
#define RELAY_2_OUT_PIN			9
#define RELAY_3_OUT_PIN			17
#define RELAY_4_OUT_PIN			11


#define CHG_PG_PIN				P41
#define CHG_STAT1_PIN			P40
#define CHG_STAT2_PIN			P38

#define TP1_PIN					P35
#define TP1_INIT()	nrf_gpio_cfg_output(TP1_PIN)
#define TP1_ON()	nrf_gpio_pin_write(TP1_PIN,1)
#define TP1_OFF()	nrf_gpio_pin_write(TP1_PIN,0)


#elif BLE_MODULE == BLE_MODULE_NORDIC_PCA10040


#endif

#endif // IO_CONFIG_H