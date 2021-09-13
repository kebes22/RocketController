#ifndef HUB_UI_H__
#define HUB_UI_H__

#include "RGB.h"

//#include <stdint.h>
//#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BUTTON_SUICIDE_TIME		1500
#define BUTTON_DFU_COUNT		5

#define UI_UPDATE_INTERVAL_MS	500
#define UI_IDLE_OFF_DELAY_MS	20000

#define BATTERY_VOLTAGE_MINIMUM	3.3

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

// User interface initialization. Run before starting scheduler.
void UI_Init(void);

void	ui_set_color( RGB_Color_t color );
void	ui_set_motors( int16_t m1, int16_t m2 );
void	ui_update_connection( bool state );

void	ui_launch_dfu( void );

#ifdef __cplusplus
}
#endif

#endif // HUB_UI_H__
