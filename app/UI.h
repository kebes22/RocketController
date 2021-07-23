#ifndef UI_H__
#define UI_H__


//#include <stdint.h>
//#include <stdbool.h>
#include "hl_adc.h"
#include "mcp73833_lipo.h"
#include "analog_stick.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BUTTON_SUICIDE_TIME			1500
#define BUTTON_SHORTPRESS_TIME		1000

#define BATTERY_VOLTAGE_MINIMUM	3.3

#define BUTTON_STARTUP_TIME_MS	500

#define BUTTON_SHORT_PRESS		1000
#define BUTTON_SYNC_DELAY		200
#define BUTTON_DOUBLE_DELAY		300
#define BUTTON_REPEAT_DELAY		200
#define BUTTON_MOVE_INC			400
#define BUTTON_SETTINGS_TIME	3000		// Time to wait before processing a settings save
#define BUTTON_TIMEOUT			500			// Milliseconds for the button polling to remain active without input.
#define BUTTON_DEBOUNCE_COUNT	30			// Milliseconds for the debounce to take place.
#define BUTTON_DUB_TAP			400			// Milliseconds between double button press to register as a double press, must be less than BUTTON_TIMEOUT.
#if BUTTON_DUB_TAP >= BUTTON_TIMEOUT
#error "BUTTON_DUB_TAP_COUNT must be less than BUTTON_TIMEOUT"
#endif

#define BL_BRIGHT_MIN			0
#define BL_BRIGHT_MAX			100
#define BL_BRIGHT_INC			5
#define BL_BRIGHT_UNIT			"%"

#define BL_DELAY_MIN			5
#define BL_DELAY_MAX			30
#define BL_DELAY_INC			1
#define BL_DELAY_UNIT			"s"

#define LCD_CONTRAST_MIN		0
#define LCD_CONTRAST_MAX		100
#define LCD_CONTRAST_INC		5
#define LCD_CONTRAST_UNIT		"%"



//################################################################################
//	System sub-structures
//################################################################################

typedef struct
{
	nrf_saadc_value_t	vBatt;
	nrf_saadc_value_t	joy1x;
	nrf_saadc_value_t	joy1y;
	nrf_saadc_value_t	joy2x;
	nrf_saadc_value_t	joy2y;
	nrf_saadc_value_t	joy1trim;
	nrf_saadc_value_t	joy2trim;
} sys_an_raw_t;

typedef struct
{
	analog_in_t			joy1x;
	analog_in_t			joy1y;
	analog_in_t			joy2x;
	analog_in_t			joy2y;
} sys_sticks_t;

typedef struct
{
	sys_an_raw_t		raw;
	sys_sticks_t		sticks;
} sys_io_t;

typedef struct
{
	float				voltage_raw;
	float				voltage;
} battery_status_t;

typedef enum
{
	STICK_MODE_SPLIT_LEFT	= 0,
	STICK_MODE_SINGLE_RIGHT,
	STICK_MODE_TANK,
	STICK_MODE_SPLIT_RIGHT,
	STICK_MODE_SINGLE_LEFT,

	STICK_MODE_COUNT,
} stick_mode_t;

typedef struct
{
	uint8_t				contrast;
	uint8_t				bl_bright;
	uint8_t				bl_delay;

	struct {
		int8_t			enable;
		int8_t			delay;
		int16_t			voltage;
	} auto_discharge;

	struct {
		stick_mode_t	mode;
		bool			enable;
	} stick;
} sys_settings_t;

typedef struct
{
	// TODO fill this structure
	bool				connected;
} sys_status_t;

//################################################################################
//	System main structure
//################################################################################
typedef struct
{
	sys_settings_t		settings;
    sys_io_t			io;
    battery_status_t	battery;
    mcp73833_t			charger;
} system_t;

extern system_t sys;

//################################################################################
//	UI API functions
//################################################################################

// User interface initialization. Run before starting scheduler.
void UI_Init(void);

void UI_on_connect( bool connected );

#ifdef __cplusplus
}
#endif

#endif // UI_H__
