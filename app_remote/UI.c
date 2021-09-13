#include "sdk_common.h"
#include "UI.h"
#include "hl_adc.h"

#include "motor_driver.h"
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

static int16_t repeatDelay	= 0;


//################################################################################
//	LED strip initialization
//################################################################################
#define NUM_LEDS				2
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
//	Animation
//################################################################################
//#define USE_ANIMATION

#ifdef USE_ANIMATION
#define ANIMATION_QUEUE_SIZE 8
#define ANIMATION_OVERRIDE_QUEUE_SIZE 4

/**@brief Enumeration of animation types
 * @note Each item must have a corresponding entry in the animationThreadList[]
 */
typedef enum
{
	UIA_STATIC,
	UIA_BLINK,
	UIA_SPIN,
	UIA_BREATHE,
}	UI_AnimationType_t;

/**@brief Structure containing animation parameters
 * @details This structure should contain a set of parameters in the union for each animation type
 *			Each function implementing an animation should use the parameters in its corresponding entry
 *			to control the behavior of the animation
 */
typedef struct
{
	UI_AnimationType_t		type;
	union
	{
		struct
		{
			RGB_Color_t		color;
			uint16_t		fadeTime;
			uint16_t		minTime;
		}	staticParams;

		struct
		{
			RGB_Color_t		offColor;
			RGB_Color_t		onColor;
			uint16_t		offTime;
			uint16_t		onTime;
			uint16_t		fadeTime;
			uint16_t		minBlinks;
		}	blinkParams;

		struct
		{
			RGB_Color_t		color;
			uint16_t		speed;
			uint8_t			minCount;
		}	spinParams;

		struct
		{
			RGB_Color_t		offColor;
			RGB_Color_t		onColor;
			uint16_t		fadeTime;
			uint16_t		restTime;
			uint16_t		minTime;
		}	breatheParams;
	};
}	UI_Animation_t;

// function and pointer typedefs
typedef void (AnimationThread_t)( UI_Animation_t const * );
typedef void (*AnimationThreadPtr_t)( UI_Animation_t const * );

// Function prototypes for animation threads
static AnimationThread_t AnimationThreadStatic;
static AnimationThread_t AnimationThreadBlink;
static AnimationThread_t AnimationThreadSpin;
static AnimationThread_t AnimationThreadBreathe;

/**@brief List containing function pointers to all
 * @details This structure should contain a set of parameters in the union for each animation type
 *			Each function implementing an animation should use the parameters in its corresponding entry
 *			to control the behavior of the animation
 */
static const AnimationThreadPtr_t animationThreadList[] = {
	AnimationThreadStatic,
	AnimationThreadBlink,
	AnimationThreadSpin,
	AnimationThreadBreathe,
};


typedef struct
{
	UI_State_t			state;
	bool				override;
}	UI_AnimationQueueItem_t;


static QueueHandle_t animationQueue;
static QueueHandle_t animationOverrideQueue;

static UI_State_t baseAnimationState;

static RGB_Color_t ledsMain[4];

#define DEFAULT_SPIN_SPEED	150
#define DEFAULT_MIN_SPINS	1
#define DEFAULT_FADE_TIME	400
#define DEFAULT_MIN_TIME	500

//static const UI_Animation_t	uiIndicator_Boot			= { .type=UIA_BLINK, .blinkParams = { .offColor.u32=RGB_BLACK, .onColor.u32=RGB_RED, .offTime=200, .onTime=200, .fadeTime=20, .minBlinks=5 } };
//static const UI_Animation_t	uiIndicator_Boot			= { .type=UIA_BLINK, .blinkParams = { .offColor.u32=RGB_BLACK, .onColor.u32=RGB_YELLOW, .offTime=200, .onTime=200, .fadeTime=20, .minBlinks=3 } };

//static const UI_Animation_t	uiIndicator_Boot			= { .type=UIA_STATIC, .staticParams = { .color.u32=RGB_WHITE, .fadeTime=DEFAULT_FADE_TIME, .minTime=5000 } };
static const UI_Animation_t uiIndicator_Boot			= { .type=UIA_STATIC, .staticParams = { .color.u32=RGB_YELLOW, .fadeTime=DEFAULT_FADE_TIME, .minTime=1000 } };

static const UI_Animation_t uiIndicator_Provisioning	= { .type=UIA_SPIN, .spinParams = { .color.u32=RGB_PURPLE, .speed=DEFAULT_SPIN_SPEED, .minCount=DEFAULT_MIN_SPINS } };
//static const UI_Animation_t uiIndicator_Provisioning	= { .type=UIA_BREATHE, .breatheParams = { .offColor.u32=RGB_CYAN_DIM, .onColor.u32=RGB_CYAN, .fadeTime=4000, .restTime=0, .minTime=DEFAULT_MIN_TIME } };
static const UI_Animation_t uiIndicator_ConnectWifi		= { .type=UIA_SPIN, .spinParams = { .color.u32=RGB_YELLOW, .speed=DEFAULT_SPIN_SPEED, .minCount=DEFAULT_MIN_SPINS } };
static const UI_Animation_t uiIndicator_ConnectCloud	= { .type=UIA_SPIN, .spinParams = { .color.u32=RGB_GREEN, .speed=DEFAULT_SPIN_SPEED, .minCount=DEFAULT_MIN_SPINS } };
static const UI_Animation_t uiIndicator_Connected		= { .type=UIA_STATIC, .staticParams = { .color.u32=RGB_YELLOW, .fadeTime=DEFAULT_FADE_TIME, .minTime=0 } };

//static const UI_Animation_t uiIndicator_Good			= { .type=UIA_STATIC, .staticParams.color.u32=RGB_CYAN, .staticParams.fadeTime=DEFAULT_FADE_TIME, .staticParams.minTime=DEFAULT_MIN_TIME };
//static const UI_Animation_t uiIndicator_Good			= { .type=UIA_BREATHE, .breatheParams = { .offColor.u32=RGB_CYAN_DIM, .onColor.u32=RGB_CYAN, .fadeTime=2000, .restTime=500, .minTime=DEFAULT_MIN_TIME } };
static const UI_Animation_t uiIndicator_Good			= { .type=UIA_BREATHE, .breatheParams = { .offColor.u32=RGB_CYAN_DIM, .onColor.u32=RGB_CYAN, .fadeTime=4000, .restTime=0, .minTime=DEFAULT_MIN_TIME } };
static const UI_Animation_t uiIndicator_Busy			= { .type=UIA_SPIN, .spinParams = { .color.u32=RGB_CYAN, .speed=DEFAULT_SPIN_SPEED, .minCount=7 } };

static const UI_Animation_t uiIndicator_NoWifi			= { .type=UIA_STATIC, .staticParams = { .color.u32=RGB_RED, .fadeTime=DEFAULT_FADE_TIME, .minTime=DEFAULT_MIN_TIME } };
static const UI_Animation_t uiIndicator_NoCloud			= { .type=UIA_STATIC, .staticParams = { .color.u32=RGB_YELLOW, .fadeTime=DEFAULT_FADE_TIME, .minTime=DEFAULT_MIN_TIME } };
static const UI_Animation_t uiIndicator_NoBlind			= { .type=UIA_SPIN, .spinParams = { .color.u32=RGB_RED, .speed=DEFAULT_SPIN_SPEED, .minCount=DEFAULT_MIN_SPINS } };

static const UI_Animation_t uiIndicator_Button			= { .type=UIA_SPIN, .spinParams = { .color.u32=RGB_WHITE, .speed=100, .minCount=1 } };
//static const UI_Animation_t uiIndicator_Button			= { .type=UIA_SPIN, .spinParams = { .color.u32=RGB_WHITE, .speed=200, .minCount=1 } };
//static const UI_Animation_t uiIndicator_Button			= { .type=UIA_BLINK, .blinkParams = { .offColor.u32=RGB_BLACK, .onColor.u32=RGB_WHITE, .offTime=50, .onTime=200, .fadeTime=50, .minBlinks=1 } };

static const UI_Animation_t uiIndicator_MfgTest			= { .type=UIA_SPIN, .spinParams = { .color.u32=RGB_YELLOW, .speed=100, .minCount=1 } };
static const UI_Animation_t uiIndicator_MfgTestRun		= { .type=UIA_BLINK, .blinkParams = { .offColor.u32=RGB_PURPLE, .onColor.u32=RGB_YELLOW, .offTime=200, .onTime=200, .fadeTime=20, .minBlinks=2 } };
static const UI_Animation_t uiIndicator_MfgTestPass		= { .type=UIA_BLINK, .blinkParams = { .offColor.u32=RGB_BLACK, .onColor.u32=RGB_GREEN, .offTime=200, .onTime=200, .fadeTime=20, .minBlinks=8 } };
static const UI_Animation_t uiIndicator_MfgTestFail		= { .type=UIA_BLINK, .blinkParams = { .offColor.u32=RGB_BLACK, .onColor.u32=RGB_RED, .offTime=200, .onTime=200, .fadeTime=20, .minBlinks=8 } };
static const UI_Animation_t uiIndicator_RTFD			= { .type=UIA_SPIN, .spinParams = { .color.u32=RGB_RED, .speed=100, .minCount=1 } };
static const UI_Animation_t uiIndicator_Identify		= { .type=UIA_BLINK, .blinkParams = { .offColor.u32=RGB_BLACK, .onColor.u32=RGB_BLUE, .offTime=300, .onTime=300, .fadeTime=20, .minBlinks=10 } };
static const UI_Animation_t uiIndicator_BLE_Connected	= { .type=UIA_SPIN, .spinParams = { .color.u32=RGB_BLUE, .speed=DEFAULT_SPIN_SPEED, .minCount=1 } };
static const UI_Animation_t uiIndicator_BLE_Pairing		= { .type=UIA_BLINK, .blinkParams = { .offColor.u32=RGB_BLACK, .onColor.u32=RGB_BLUE, .offTime=480, .onTime=480, .fadeTime=20, .minBlinks=4 } };

static const UI_Animation_t uiIndicator_NoCert		= { .type=UIA_BLINK, .blinkParams = { .offColor.u32=RGB_RED, .onColor.u32=RGB_YELLOW, .offTime=350, .onTime=350, .fadeTime=20, .minBlinks=4 } };
static const UI_Animation_t uiIndicator_NoMfgKey	= { .type=UIA_BLINK, .blinkParams = { .offColor.u32=RGB_RED, .onColor.u32=RGB_BLUE, .offTime=350, .onTime=350, .fadeTime=20, .minBlinks=4 } };


static const UI_Animation_t * const animations[] = {
	NULL,

	&uiIndicator_Boot,

	&uiIndicator_Provisioning,
	&uiIndicator_ConnectWifi,
	&uiIndicator_ConnectCloud,
	&uiIndicator_Connected,

	&uiIndicator_Busy,
	&uiIndicator_Good,

	&uiIndicator_NoWifi,
	&uiIndicator_NoCloud,
	&uiIndicator_NoBlind,

	&uiIndicator_Button,
	&uiIndicator_MfgTest,
	&uiIndicator_MfgTestRun,
	&uiIndicator_MfgTestPass,
	&uiIndicator_MfgTestFail,
	&uiIndicator_RTFD,
	&uiIndicator_Identify,
	&uiIndicator_BLE_Connected,
	&uiIndicator_BLE_Pairing,

	&uiIndicator_NoCert,
	&uiIndicator_NoMfgKey,

};

static UI_State_t animationOverrideState = UI_STATE_NONE;
static UI_State_t animationCurrentState = UI_STATE_NONE;
static UI_AnimationQueueItem_t dummyAnimation;
static bool pairingState = false;

static void UI_SetState_Generic( UI_State_t state, bool override )
{
	static UI_State_t lastState = -1;
//	static uint8_t maxDepth = 0;
//	uint8_t depth;
//	if ( !animationQueue ) {
//		NRF_LOG_ERROR( "!!! Animation queue not initialized !!!" );
//		return;
//	}
	UI_AnimationQueueItem_t qItem = { .state=state, .override=override };

	if ( state != lastState || override ) {									// Make sure it's not a duplicate
		//	If full, discard oldest items, until space is available
		bool full;
		// Loop will make sure any pending tasks (with higher priority) get processed as well, and don't sneak in
		while ( (full = (uxQueueSpacesAvailable( animationQueue ) < 1)) ) {
			NRF_LOG_ERROR( "!!! Animation state discarded !!!" );
			xQueueReceive( animationQueue, &dummyAnimation, 0 );
		}

		BaseType_t result;
		if ( override ) {
			result = xQueueSendToFront( animationQueue, &qItem, 0 );
		} else {
			result = xQueueSend( animationQueue, &qItem, 0 );
		}

		if ( pdFAIL == result ) {
			NRF_LOG_ERROR( "!!! Animation queue overflow !!!" );
		}
//		depth = uxQueueMessagesWaiting( animationQueue );
//		if ( depth > maxDepth ) {
//			maxDepth = depth;
//		}
	}
	NRF_LOG_DEBUG( "UI state set to: %d", state);
	lastState = state;
}

static bool UI_ProcessOverrideQueue( void )
{
	bool override = false;
	if ( uxQueueMessagesWaiting(animationOverrideQueue) ) {
		BaseType_t result = xQueuePeek( animationQueue, &dummyAnimation, 0 );
		if ( !result || !dummyAnimation.override ) { // Nothing pending, or it isn't an override
			if ( pdPASS == xQueueReceive( animationOverrideQueue, &dummyAnimation, 0 ) ) {
				override = true;
				UI_SetState_Generic( dummyAnimation.state, true );
			}
		}
	}
	return override;
}


//static void UI_SetOverride( UI_State_t state )
//{
//	animationOverrideState = state;						// Flag indicating active override state
//	UI_SetState_Generic( state, true );
//}

static void UI_SetOverride( UI_State_t state )
{
	static UI_State_t lastState = -1;
	animationOverrideState = state;
	if ( state != lastState ) { // Make sure it's not a duplicate
		bool full;
		// Loop will make sure any pending tasks (with higher priority) get processed as well, and don't sneak in
		while ( (full = (uxQueueSpacesAvailable( animationOverrideQueue ) < 1)) ) {
			NRF_LOG_ERROR( "!!! Override state discarded !!!" );
			xQueueReceive( animationOverrideQueue, &dummyAnimation, 0 );
		}

		UI_AnimationQueueItem_t qItem = { .state=state, .override=true };
		xQueueSend( animationOverrideQueue, &qItem, 0 );
	}
	UI_ProcessOverrideQueue(); // Check if it can be moved to main queue
}

#endif // USE_ANIMATION

//################################################################################
//	Button processing code
//################################################################################


//################################################################################
//	Charger
//################################################################################
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
		leds.p_leds[1] = RGB_COLOR.RED;
		vTaskDelay( 500 );
		_do_suicide();
	}
#endif
}

static void _adc_done_cb( void )
{
	vBatt = (float)vBattRaw / AN_COUNTS_PER_V;

	_check_battery_voltage();

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
	int16_t			motorChannels[2];

	RGB_Color_t		outputColor;

	uint16_t		rxCount;
	uint32_t		offCount;
}	ui_status_t;

ui_status_t ui_status = {0};

void	StateControl( void )
{
	ui_status.allowRun = false;
	RGB_Color_t newColor = { 0, 0, 0 };
// Process states
	// Charging has highest priority, and blocks all other states
	if ( charger.chargeState != CHG_SHUTDOWN ) {
        newColor = charger.ledColor;
        ui_status.offCount = 0;
	} else if ( !ui_status.connected ) {
		newColor = disconnectedColor;
	} else {
		ui_status.allowRun = true;
        newColor = ui_status.radioColor;
	}

// Process motor outputs
	if ( !ui_status.allowRun ) {
		ui_status.motorChannels[0] = 0;
		ui_status.motorChannels[1] = 0;
	}
	motor_run( ui_status.motorChannels[0], ui_status.motorChannels[1] );

// Set LEDs
	if ( 0 != memcmp( &newColor, &ui_status.outputColor, sizeof(newColor) ) ) {
		ui_status.outputColor = newColor;
		RGB_Fader_SetTarget( &rgbFaders[0], ui_status.outputColor, COLOR_FADE_TIME );
		RGB_Fader_SetTarget( &rgbFaders[1], ui_status.outputColor, COLOR_FADE_TIME );
	}
}




static void _update_timer_handler( TimerHandle_t xTimer )
{
	NRF_LOG_DEBUG( LOG_COLOR_MAGENTA "Batt: Raw: %d, " NRF_LOG_FLOAT_MARKER "V (RX: %d)" , vBattRaw, NRF_LOG_FLOAT(vBatt), ui_status.rxCount );
	if ( !ui_status.rxCount ) {
		// TODO error
		ui_status.motorChannels[0] = 0;
		ui_status.motorChannels[1] = 0;
		ui_status.offCount += UI_UPDATE_INTERVAL_MS;
		if ( ui_status.offCount > UI_IDLE_OFF_DELAY_MS )
			_do_suicide();
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
	ui_status.motorChannels[0] = m1;
	ui_status.motorChannels[1] = m2;
    ui_status.rxCount++;
}

void	ui_update_connection( bool state )
{
	ui_status.connected = state;
}

void	ui_launch_dfu( void )
{
	leds.p_leds[0] = RGB_COLOR.RED;
	leds.p_leds[1] = RGB_COLOR.GREEN;
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
	if ( charger.isBusy )
		return;

	leds.p_leds[0] = RGB_COLOR.BLACK;
	leds.p_leds[1] = RGB_COLOR.BLACK;
	vTaskDelay(50);
	nrf_gpio_pin_write( EN_5V_PIN, 0 );	// Turn off leds
	suicide();
}


static void _startup_voltage_check( void )
{
	_adc_sample_and_wait();	// Take a sample to trigger check
}


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

	vTaskDelay( 50 );
	Devices_Init();  // Initializes flash memory.

	_startup_voltage_check();
	_startup_button_check();

	advertising_start(BLE_ADV_MODE_FAST);

	while ( 1 )
	{
#if 1
		vTaskDelayUntil( &xLastWakeTime, xFrequency );
#else
        vTaskDelay( xFrequency );
#endif
		_adc_sample_and_wait();
		ButtonProcess(&button, !nrf_gpio_pin_read(BUTTON_IN_PIN), UI_THREAD_INTERVAL);
		FaderProcess( UI_THREAD_INTERVAL );

		charger_update();
		StateControl();

		if ( button.onTime > BUTTON_SUICIDE_TIME ) {
			_do_suicide();
		}

		ButtonClearEvents( &button );
	}
}




void	UI_Init( void )
{
	nrf_gpio_cfg_input( BUTTON_IN_PIN, NRF_GPIO_PIN_PULLUP );

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

//	leds.p_leds[0] = disconnectedColor;
//	leds.p_leds[1] = disconnectedColor;
//	leds.p_leds[0] = RGB_COLOR.MAGENTA;
//	leds.p_leds[1] = RGB_COLOR.MAGENTA;
	RGB_Fader_SetTarget( &rgbFaders[0], RGB_COLOR.MAGENTA, 100 );
	RGB_Fader_SetTarget( &rgbFaders[1], RGB_COLOR.MAGENTA, 100 );
	
	charger_init();
	motor_init();
}
