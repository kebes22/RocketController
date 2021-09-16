#include "sdk_common.h"
#include "UI.h"
#include "hl_adc.h"

#include "Hardware.h"

#define NRF_LOG_MODULE_NAME UI
#ifdef UI_LOG_LEVEL
#define	NRF_LOG_LEVEL UI_LOG_LEVEL
#else
#define	NRF_LOG_LEVEL 1
#endif
#include "log.h"

#include "ble_gap.h"
#include "advertising.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

//#include "LED.h"

#include "io_config.h"
#include "buttons.h"
#include "nrf_gpio.h"
#include "suicide_pin.h"
#include "dfu_api.h"

#include "WS2812_I2S.h"
#include "RGB.h"
#include "RGB_Fader.h"

#include "mcp73833_lipo.h"

#include "comms.h"


#define UI_THREAD_INTERVAL		10
#define COLOR_FADE_TIME			500

//################################################################################
//	Function prototypes
//################################################################################
static void _do_suicide( void );


//################################################################################
//	Local member variables
//################################################################################
static TaskHandle_t m_ui_thread;
static TimerHandle_t updateTimer;

BUTTON_DEF(button, BUTTON_DEBOUNCE_COUNT);
BUTTON_DEF(button2, BUTTON_DEBOUNCE_COUNT);

static int16_t repeatDelay	= 0;


//################################################################################
//	LED strip initialization
//################################################################################
#define NUM_LEDS				1
#define LED_UPDATE_INTERVAL		10


static TimerHandle_t m_ledTimer;


WS2812_I2S_STRIP_DEF( leds, NUM_LEDS, true );


static void led_timer_handler( TimerHandle_t xTimer )
{
	UNUSED_PARAMETER(xTimer);
	WS2812_Update( &leds );
}

static void led_timer_init( void )
{
	m_ledTimer = xTimerCreate("LED", LED_UPDATE_INTERVAL, pdTRUE, NULL, led_timer_handler);
	if (pdPASS != xTimerStart(m_ledTimer, 2)) {
		APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
	}
}

static void LedsInit( void )
{
	WS2812_I2S_Init_t init = {
		.pinNo = LED_DAT_PIN,
	};
	WS2812_Init( &leds, &init );
	led_timer_init();
}

//################################################################################
//	LED strip animation
//################################################################################



//################################################################################
//	Animation Process
//################################################################################


//################################################################################
//	RGB/Fader Handling
//################################################################################
RGB_Fader_t rgbFaders[NUM_LEDS];

static void FaderProcess( uint8_t steps )
{
	RGB_Color_t tempColor;
	uint8_t i = 0;
	for ( i=0; i<NUM_LEDS; i++ ) {									// Process individual LEDs
		tempColor = RGB_Fader_Process( &rgbFaders[i], steps );		// Fade Colors
		leds.p_leds[i] = tempColor;									// Write color
	}
}


void RGB_Init( void )
{
	uint8_t i = 0;
	for ( i=0; i<NUM_LEDS; i++ ) {
		leds.p_leds[i] = RGB_Fader_SetTarget( &rgbFaders[i], (RGB_Color_t){ 0, 0, 0 }, 0 );
	}
}


//################################################################################
//	Button processing code
//################################################################################


//################################################################################
//	Charger
//################################################################################
#if 0
/*	Charger state table from datasheet
	----------------------------------------
	Charge Cycle State	STAT1	STAT2	PG
	----------------------------------------
	Shutdown			Hi-Z	Hi-Z	Hi-Z
	Standby				Hi-Z	Hi-Z	L
	Charge in Progress	L		Hi-Z	L
	Charge Complete		Hi-Z	L		L
	Temperature Fault	Hi-Z	Hi-Z	L
	Timer Fault			Hi-Z	Hi-Z	L
	System Test Mode	L		L		L
	----------------------------------------
*/

typedef enum
{
	CHG_SHUTDOWN,
//	CHG_STANDBY,
	CHG_CHARGING,
	CHG_CHARGED,
	CHG_FAULT,
} charger_state_t;


typedef struct
{
	bool				pgState;
	bool				stat1;
	bool				stat2;

    charger_state_t		chargeState;
	RGB_Color_t			ledColor;
	bool				isBusy;

} charger_t;

charger_t charger;

void charger_update( void )
{
	charger.pgState = nrf_gpio_pin_read( CHG_PG_PIN );
	charger.stat1 = nrf_gpio_pin_read( CHG_STAT1_PIN );
	charger.stat2 = nrf_gpio_pin_read( CHG_STAT2_PIN );

	charger_state_t newState = CHG_SHUTDOWN;
	if ( charger.pgState == 0 ) {
		if ( charger.stat1 == 0 )
			newState = CHG_CHARGING;
		else if ( charger.stat2 == 0 )
			newState = CHG_CHARGED;
		else
			newState = CHG_FAULT;
	} else {
		newState = CHG_SHUTDOWN;
	}

	RGB_Color_t const * color = &RGB_COLOR.BLACK;
	switch ( newState ) {
		case CHG_SHUTDOWN:
//			color = NULL;
			break;

		case CHG_CHARGING:
			color = &RGB_COLOR.YELLOW;
			break;

		case CHG_CHARGED:
			color = &RGB_COLOR.GREEN;
			break;

		case CHG_FAULT:
			color = &RGB_COLOR.RED;
			break;
	}

// Check if charger was just disconnected
	bool doShutdown = false;
	if ( newState == CHG_SHUTDOWN && charger.isBusy )
		doShutdown = true;

	charger.isBusy = ( newState == CHG_CHARGING || newState == CHG_CHARGED );
	charger.chargeState = newState;

	if ( doShutdown )
		_do_suicide();

//	if ( color ) {
		charger.ledColor = *color;
//	}
}

void charger_init( void )
{
	nrf_gpio_cfg_input( CHG_PG_PIN, NRF_GPIO_PIN_PULLUP );
	nrf_gpio_cfg_input( CHG_STAT1_PIN, NRF_GPIO_PIN_PULLUP );
	nrf_gpio_cfg_input( CHG_STAT2_PIN, NRF_GPIO_PIN_PULLUP );
}
#endif

//################################################################################
//	ADC Handling
//################################################################################
static SemaphoreHandle_t adc_semaphore;
static nrf_saadc_value_t vBattRaw;

static float vBatt = 0;

//#define AN_COUNTS_PER_V		3416
//#define AN_COUNTS_PER_V		3425
#define AN_COUNTS_PER_V		3400

static void _check_battery_voltage( void )
{
#ifndef DEV_BOARD
	if ( vBatt < BATTERY_VOLTAGE_MINIMUM ) {
		leds.p_leds[0] = RGB_COLOR.RED;
		vTaskDelay( 500 );
		_do_suicide();
	}
#endif
}

static void _adc_done_cb( void )
{
	vBatt = (float)vBattRaw / AN_COUNTS_PER_V;

	//_check_battery_voltage();

	hl_adc_sleep();
	xSemaphoreGive( adc_semaphore );
}

static void _adc_init( void )
{
	nrfx_saadc_config_t saadc_config = {
		.resolution 		= NRF_SAADC_RESOLUTION_14BIT,
		.oversample 		= NRF_SAADC_OVERSAMPLE_32X,
		.interrupt_priority	= APP_IRQ_PRIORITY_MID,
		.low_power_mode		= false
	};

	nrf_saadc_channel_config_t adc_channel_config_voltage = HL_ADC_SIMPLE_CONFIG_SE( VBATT_AN_IN, NRF_SAADC_REFERENCE_INTERNAL, NRF_SAADC_GAIN1_6 );
	hl_adc_channel_register( &adc_channel_config_voltage, &vBattRaw, NULL );

	hl_adc_init( &saadc_config, _adc_done_cb );

	adc_semaphore = xSemaphoreCreateBinary();
}

static void _adc_sample_and_wait( void )
{
	xSemaphoreTake( adc_semaphore, 0 );				// Take semaphore in case already pending
	hl_adc_start_burst();
	xSemaphoreTake( adc_semaphore, 2 );	// TODO use a timeout
}


//################################################################################
//	Utilities
//################################################################################

static RGB_Color_t disconnectedColor = { 16, 16, 16 };

typedef struct
{
	bool			allowRun;
	bool			connected;	//TODO add connected logic

	RGB_Color_t		radioColor;

	RGB_Color_t		outputColor;

	uint16_t		rxCount;
	uint32_t		offCount;

	mcp73833_t	charger;

}	ui_status_t;

ui_status_t ui_status = {0};

void	StateControl( void )
{
	RGB_Color_t const * charger_color = NULL;
	switch ( ui_status.charger.chargeState ) {
		case CHG_SHUTDOWN:
//			charger_color = NULL;
			break;

		case CHG_CHARGING:
			charger_color = &RGB_COLOR.YELLOW;
			break;

		case CHG_CHARGED:
			charger_color = &RGB_COLOR.GREEN;
			break;

		case CHG_FAULT:
			charger_color = &RGB_COLOR.RED;
			break;
	}

	ui_status.allowRun = false;
	RGB_Color_t newColor = { 0, 0, 0 };
// Process states
	// Charging has highest priority, and blocks all other states

// TODO update these states for remote
	if ( charger_color ) {
        newColor = *charger_color;
        ui_status.offCount = 0;
	} else if ( !ui_status.connected ) {
		newColor = disconnectedColor;
	} else {
		ui_status.allowRun = true;
        newColor = ui_status.radioColor;
	}

// Process motor outputs
	if ( !ui_status.allowRun ) {
		//ui_status.motorChannels[0] = 0;
		//ui_status.motorChannels[1] = 0;
	}

// Set LEDs
	if ( 0 != memcmp( &newColor, &ui_status.outputColor, sizeof(newColor) ) ) {
		ui_status.outputColor = newColor;
		RGB_Fader_SetTarget( &rgbFaders[0], ui_status.outputColor, COLOR_FADE_TIME );
	}
}




static void _update_timer_handler( TimerHandle_t xTimer )
{
	NRF_LOG_DEBUG( LOG_COLOR_MAGENTA "Batt: Raw: %d, " NRF_LOG_FLOAT_MARKER "V (RX: %d)" , vBattRaw, NRF_LOG_FLOAT(vBatt), ui_status.rxCount );
	if ( !ui_status.rxCount ) {
		// TODO error
		//ui_status.motorChannels[0] = 0;
		//ui_status.motorChannels[1] = 0;
#ifndef NEVER_SLEEP
		ui_status.offCount += UI_UPDATE_INTERVAL_MS;
		if ( ui_status.offCount > UI_IDLE_OFF_DELAY_MS )
			_do_suicide();
#endif
	} else {
		ui_status.offCount = 0;
	}
	ui_status.rxCount = 0;
}

//################################################################################
//	External API
//################################################################################
void	ui_set_color( RGB_Color_t color )
{
	ui_status.radioColor = color;
}

void ui_set_motors( int16_t m1, int16_t m2 )
{
	//ui_status.motorChannels[0] = m1;
	//ui_status.motorChannels[1] = m2;
    ui_status.rxCount++;
}

void	ui_update_connection( bool state )
{
	ui_status.connected = state;
}

void	ui_launch_dfu( void )
{
	leds.p_leds[0] = RGB_COLOR.RED;
	vTaskDelay( 500 );
	dfu_launch();
}


//################################################################################
//	Utilities
//################################################################################

static void ReportMAC( void )
{
	ble_gap_addr_t mac_addr;
	sd_ble_gap_addr_get(&mac_addr);
	NRF_LOG_INFO(
		"MAC Address: %02x:%02x:%02x:%02x:%02x:%02x",
		mac_addr.addr[5], mac_addr.addr[4], mac_addr.addr[3],
		mac_addr.addr[2], mac_addr.addr[1], mac_addr.addr[0]
	);
}

static void _do_suicide( void )
{
	if ( ui_status.charger.isBusy )
		return;

	NRF_LOG_INFO( "Shutting Down" );
	leds.p_leds[0] = RGB_COLOR.BLACK;
	vTaskDelay(50);
	nrf_gpio_pin_write( EN_5V_PIN, 0 );	// Turn off leds
	suicide();
}

static void _startup_voltage_check( void )
{
	_adc_sample_and_wait();	// Take a sample to trigger check
}

//################################################################################
//	Startup check
//################################################################################

static bool	_startup_check( void )
{
	if ( nrf_gpio_pin_read(BUTTON1_IN_PIN) != 0 ) // If button was not source of wakeup, then always wake up
		return true;

	bool startup = false;

	// Button woke us up, so check it for long enough hold
	uint8_t startup_delay = 10;
	while ( 1 ) {
        vTaskDelay( UI_THREAD_INTERVAL );
		ButtonProcess(&button, !nrf_gpio_pin_read(BUTTON1_IN_PIN), UI_THREAD_INTERVAL);
		FaderProcess( UI_THREAD_INTERVAL );

		if ( !startup_delay ) {
			if ( !button.on ) {
				//return false;
				// TODO temporary to allow infinite on hold
				return startup? true: false;
			}
			if ( button.onTime > BUTTON_STARTUP_TIME_MS ) {
				startup = true;
			}
		}

		if ( startup_delay )
			startup_delay--;

		ButtonClearEvents( &button );
	}
}

// TODO add back button launch of DFU
#if 0
static void _startup_button_check( void )
{
	uint8_t dfuCount = 0;
	while ( 1 ) {
		vTaskDelay( UI_THREAD_INTERVAL );
		ButtonProcess(&button, !nrf_gpio_pin_read(BUTTON_IN_PIN), UI_THREAD_INTERVAL);
		FaderProcess( UI_THREAD_INTERVAL );

		// Count sequential short press events
		if ( button.offEvent && button.offEvent < BUTTON_SHORT_PRESS ) {
			dfuCount++;
		}

		if ( dfuCount >= BUTTON_DFU_COUNT ) {
			ui_launch_dfu();
		}

		ButtonClearEvents( &button );
		// Bail out if button is not continually pressed
		if ( button.offTime > BUTTON_SHORT_PRESS )
			break;
	}
}
#endif

//################################################################################
//	RTOS Threads / Init
//################################################################################


static void ui_thread( void * arg )
{
	UNUSED_PARAMETER(arg);
	NRF_LOG_INFO("Thread Started: %s", pcTaskGetName(NULL));

	const TickType_t xFrequency = UI_THREAD_INTERVAL;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	ReportMAC();

	// Process button startup check immediately
	bool startup = _startup_check();

	// Wait a little, then check voltage
	vTaskDelay( 50 );
	_startup_voltage_check();
	if ( !startup )
		_do_suicide();

	Devices_Init();  // Initializes flash memory.

	advertising_start(BLE_ADV_MODE_FAST);

	while ( 1 )
	{
#if 1
		vTaskDelayUntil( &xLastWakeTime, xFrequency );
#else
        vTaskDelay( xFrequency );
#endif
		_adc_sample_and_wait();
		ButtonProcess(&button, !nrf_gpio_pin_read(BUTTON1_IN_PIN), UI_THREAD_INTERVAL);
		ButtonProcess(&button2, !nrf_gpio_pin_read(BUTTON2_IN_PIN), UI_THREAD_INTERVAL);
		FaderProcess( UI_THREAD_INTERVAL );

		charger_update( &ui_status.charger, vBatt );
		StateControl();

#if 0
		if ( button.onEvent ) {
			comms_send_button( 1 );
		}
#else
		if ( button2.onEvent ) {
			comms_send_button( 1 );
		}
#endif

		if ( button.onTime > BUTTON_SUICIDE_TIME ) {
			_do_suicide();
		}
		//if ( button2.onTime > BUTTON_SUICIDE_TIME ) {
		//	_do_suicide();
		//}

		ButtonClearEvents( &button );
		ButtonClearEvents( &button2 );
	}
}




void	UI_Init( void )
{
	nrf_gpio_cfg_input( BUTTON1_IN_PIN, NRF_GPIO_PIN_PULLUP );
	nrf_gpio_cfg_input( BUTTON2_IN_PIN, NRF_GPIO_PIN_PULLUP );

	nrf_gpio_cfg_output( EN_5V_PIN );
	nrf_gpio_pin_write( EN_5V_PIN, 1 );

	// Create Hub UI thread
	if ( pdPASS != xTaskCreate(ui_thread, "UI", 256, NULL, 2, &m_ui_thread) ) {
		APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
	}

	updateTimer = xTimerCreate( "Update", UI_UPDATE_INTERVAL_MS, pdTRUE, NULL, _update_timer_handler );
	if ( updateTimer == NULL ) {
		APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
	}
    xTimerStart( updateTimer, 0 );

	LedsInit();
	_adc_init();

	RGB_Init();

	RGB_Fader_SetTarget( &rgbFaders[0], RGB_COLOR.MAGENTA, 100 );

	mcp73833_config_t chargerConfig = {
		.stat1_pin	= CHG_STAT1_PIN,
		.stat2_pin	= CHG_STAT2_PIN,
		.pg_pin		= CHG_PG_PIN
	};
	charger_init( &ui_status.charger, &chargerConfig );
}
