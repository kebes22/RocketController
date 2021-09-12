#include "sdk_common.h"
#include "UI.h"

#define NRF_LOG_MODULE_NAME UI
#ifdef UI_LOG_LEVEL
#define	NRF_LOG_LEVEL UI_LOG_LEVEL
#else
#define	NRF_LOG_LEVEL 1
#endif
#include "log.h"


#include "ble_gap.h"
//#include "advertising.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

//#include "LED.h"

#include "io_config.h"
#include "buttons.h"
#include "nrf_gpio.h"

#include "WS2812_I2S.h"
#include "RGB.h"
#include "RGB_Fader.h"

#include "hardware.h"

#include "ppm_capture.h"

#include "suicide_pin.h"

#include "MenuSystem.h"
#include "nrf_power.h"

#include "Utils.h"

#include "mcp73833_lipo.h"


#define UI_THREAD_INTERVAL		10
//#define UI_THREAD_INTERVAL		20

//#define UI_COLOR_UPDATE_INTERVAL	1000


#define UI_THREAD_INTERVAL		10
#define COLOR_FADE_TIME			500



//sys_io_t sys.io;
system_t sys = { 0 };

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
BUTTON_DEF(buttonJ1B1, BUTTON_DEBOUNCE_COUNT);
BUTTON_DEF(buttonJ2B1, BUTTON_DEBOUNCE_COUNT);

static int16_t repeatDelay	= 0;

//################################################################################
//	LED stuff
//################################################################################
#define NUM_LEDS				5
#define LED_UPDATE_INTERVAL		10

WS2812_I2S_STRIP_DEF( leds, NUM_LEDS, true );

//RGB_Color_t		userLED;

//static RGB_Color_t const * color_list[] = {
//	&RGB_COLOR.RED,
//	&RGB_COLOR.GREEN,
//	&RGB_COLOR.BLUE,

////	&RGB_COLOR.CYAN,
//	&RGB_COLOR.MAGENTA,
//	&RGB_COLOR.YELLOW,
//};

//#define NUM_COLORS	ARRAY_SIZE(color_list)
//int8_t color_index = 0;

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

void FaderInit( void )
{
	uint8_t i = 0;
	for ( i=0; i<NUM_LEDS; i++ ) {
		leds.p_leds[i] = RGB_Fader_SetTarget( &rgbFaders[i], (RGB_Color_t){ 0, 0, 0 }, 0 );
	}
}

//################################################################################
//	LED strip initialization
//################################################################################
static TimerHandle_t m_ledTimer;

static void led_timer_handler( TimerHandle_t xTimer )
{
	FaderProcess( LED_UPDATE_INTERVAL );
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
	// TODO fix the dummy pins
	WS2812_I2S_Init_t init = {
		.pinNo = LED_DAT_PIN,
		.dummyPin1	= LED_DUMMY1_PIN,
		.dummyPin2	= LED_DUMMY2_PIN,
	};
	WS2812_Init( &leds, &init );
	FaderInit();
	led_timer_init();
}

static void LedsKill( void )
{
	xTimerStop(m_ledTimer, 2);
	WS2812_Clear(&leds);
    WS2812_Update(&leds);

}

//################################################################################
//	Battery Button stuff
//################################################################################
#define BUTTON_UP_NOMINAL_RATIO		0.75f
#define BUTTON_DN_NOMINAL_RATIO		0.60f
#define BUTTON_SEL_NOMINAL_RATIO	0.05f
#define BUTTON_NOMINAL_BAND			0.05f

#define BUTTON_MAX_RATIO			BUTTON_UP_NOMINAL_RATIO


#define BATT_FILTER_DIV		50.0f
static void batt_button_process( float vBatt )
{
	static float vBattSum = 0.0f;
	if ( vBatt > 3.3f ) {
		vBattSum += (vBatt - sys.battery.voltage);
		sys.battery.voltage = vBattSum / BATT_FILTER_DIV;
	}
}

//################################################################################
//	ADC Handling
//################################################################################
static SemaphoreHandle_t adc_semaphore;

#define ADC_PER_V_AN5V		3414	// (1/2)/(0.6*4)*16385 at gain of 1/4

//static float vBatt = 0;
#if 0
//#define AN_COUNTS_PER_V		3048		// Measured
#define AN_COUNTS_PER_V		3038		// Measured (Fluke 289)
//#define AN_COUNTS_PER_V		762		// Measured
//#define AN_COUNTS_PER_V		191		// Measured
#else
// Remapping based on external 5V supply
#define AN_COUNTS_PER_V		3491		// Measured (Fluke 289)
//#define AN_COUNTS_PER_V		3414	// (1/3)/(0.6*6)*16385 at gain of 1/6
#endif

static void _check_battery_voltage( void )
{
#ifndef DEV_BOARD
	if ( sys.battery.voltage < BATTERY_VOLTAGE_MINIMUM ) {
		vTaskDelay( 500 );
		_do_suicide();
	}
#endif
}

static void _adc_done_cb( void )
{
	// Process battery voltage
	sys.battery.voltage_raw = (float)sys.io.raw.vBatt / AN_COUNTS_PER_V;
	batt_button_process( sys.battery.voltage_raw );

	analog_buf_t * ptr = (analog_buf_t *)&sys.analog;
	for ( int i=0; i<(sizeof(analog_inputs_t) / sizeof(analog_buf_t)) ; i++ ) {
		ptr->voltage = (float)ptr->raw / ADC_PER_V_AN5V;
        ptr++;
    }

	sys.psi.current_raw = sys.analog.an1.raw;

//	_check_battery_voltage();

//	hl_adc_sleep();
	xSemaphoreGive( adc_semaphore );
}

static void _adc_init( void )
{
	nrfx_saadc_config_t saadc_config = {
		.resolution 		= NRF_SAADC_RESOLUTION_14BIT,
//		.resolution 		= NRF_SAADC_RESOLUTION_12BIT,
		.oversample 		= NRF_SAADC_OVERSAMPLE_32X,
//		.oversample 		= NRF_SAADC_OVERSAMPLE_16X,
		.interrupt_priority	= APP_IRQ_PRIORITY_MID,
		.low_power_mode		= false
	};

	nrf_saadc_channel_config_t adc_channel_config_voltage = HL_ADC_SIMPLE_CONFIG_SE( VBATT_AN_IN, NRF_SAADC_REFERENCE_INTERNAL, NRF_SAADC_GAIN1_6 );
	hl_adc_channel_register( &adc_channel_config_voltage, &sys.io.raw.vBatt, NULL );

	nrf_saadc_channel_config_t adc_channel_config = HL_ADC_SIMPLE_CONFIG_SE( EXT_AN_IN_1, NRF_SAADC_REFERENCE_INTERNAL, NRF_SAADC_GAIN1_4 );
	hl_adc_channel_register( &adc_channel_config, &sys.analog.an1.raw, NULL );
	adc_channel_config.pin_p = EXT_AN_IN_2;
	hl_adc_channel_register( &adc_channel_config, &sys.analog.an2.raw, NULL );
	adc_channel_config.pin_p = EXT_AN_IN_3;
	hl_adc_channel_register( &adc_channel_config, &sys.analog.an3.raw, NULL );
	adc_channel_config.pin_p = EXT_AN_IN_4;
	hl_adc_channel_register( &adc_channel_config, &sys.analog.an4.raw, NULL );

	hl_adc_init( &saadc_config, _adc_done_cb );

	adc_semaphore = xSemaphoreCreateBinary();
}

static void _adc_sample_and_wait( void )
{
	// TODO fixme. Sometimes crashes in this semaphor take
	xSemaphoreTake( adc_semaphore, 0 );		// Take semaphore in case already pending
	hl_adc_start_burst();
//	xSemaphoreTake( adc_semaphore, 2 );
	xSemaphoreTake( adc_semaphore, 10 );
}

//################################################################################
//	Button stuff
//################################################################################
#define JOY_NAV_TH_HI				(600)
#define JOY_NAV_TH_LO				(-600)

//static float	vBattNominal = 4.2f;
//static float	vBattNominal = 3.9f;

static Menu_KeyBits_t menuKeyBits = { 0 };


static Menu_KeyBits_t MenuKeys_Collect( void )
{
	float vRatio = sys.battery.voltage_raw / sys.battery.voltage;

	//Menu_KeyBits_t menuKeyBits = { 0 };

	//menuKeyBits.up		= sys.io.sticks.joy2y.value > JOY_NAV_TH_HI;
	//menuKeyBits.down	= sys.io.sticks.joy2y.value < JOY_NAV_TH_LO;
	//menuKeyBits.right	= sys.io.sticks.joy2x.value > JOY_NAV_TH_HI;
	//menuKeyBits.left	= sys.io.sticks.joy2x.value < JOY_NAV_TH_LO;

	menuKeyBits.minus	= (	vRatio > (BUTTON_UP_NOMINAL_RATIO - BUTTON_NOMINAL_BAND)
							&& vRatio < (BUTTON_UP_NOMINAL_RATIO + BUTTON_NOMINAL_BAND) );
	menuKeyBits.plus	= (	vRatio > (BUTTON_DN_NOMINAL_RATIO - BUTTON_NOMINAL_BAND)
							&& vRatio < (BUTTON_DN_NOMINAL_RATIO + BUTTON_NOMINAL_BAND) );
//	menuKeyBits.left	= 0;
//	menuKeyBits.right	= 0;

	menuKeyBits.select	= (	vRatio > (BUTTON_SEL_NOMINAL_RATIO - BUTTON_NOMINAL_BAND)
							&& vRatio < (BUTTON_SEL_NOMINAL_RATIO + BUTTON_NOMINAL_BAND) )
							|| sys.analog.an4.voltage > 1.5;//|| buttonJ2B1.on || button2.on;

	menuKeyBits.back	= button.on || buttonJ1B1.on;

	return menuKeyBits;
}

//################################################################################
//	Relay control
//################################################################################
#define PSI_HOLD_MARGIN		2
#define ADC_PER_V_PSI		4551	// (1/2)/(0.6*3)*16385 at gain of 1/3
//#define ADC_COMP_PSI		1.1f	// Coefficient of the correct answer (probably compensating for resistor divider load)
#define ADC_COMP_PSI		1.05f	// Coefficient of the correct answer (probably compensating for resistor divider load)

#define FILL_DELAY			40		// In 10ms
#define DUMP_DELAY			30		// In 10ms
#define FIRE_TIME			100		// In ms


#define RELAY_INIT(_num)						\
	RocketRelay_Config_t config ## _num = {		\
		.outPin = RELAY_ ## _num ## _OUT_PIN,	\
		.inPin = RELAY_ ## _num ## _IN_PIN		\
	};											\
    RocketRelay_Init(&sys.relays.relay ## _num, &config ## _num)

#define RELAY_UPDATE(_num)	do {	\
		RocketRelay_Process(&sys.relays.relay ## _num, UI_THREAD_INTERVAL);	\
		uint8_t status = (sys.relays.relay ## _num).status;					\
		RGB_Fader_SetTarget( &rgbFaders[(_num-1)], status==RR_STATUS_READY? RGB_COLOR.GREEN_DIM: (status==RR_STATUS_ON? RGB_COLOR.BLUE: RGB_COLOR.RED_DIM), 50 );	\
	} while (0)

#define RELAY_SET(_num,_state)	RocketRelay_Set(&sys.relays.relay ## _num, _state)
#define BEEP(_time)				RocketRelay_On(&sys.relays.relay5, _time)
#define FIRE(_time)				RocketRelay_On(&sys.relays.relay3, _time)
#define IS_FIRING()				(sys.relays.relay3.isOn)

void psi_update( void )
{
	sys.psi.current_mv = sys.analog.an1.voltage * ADC_COMP_PSI;
	sys.psi.current = (int8_t)((sys.psi.current_mv - 0.5f) * 100.0f / 4.0f);

	if ( sys.psi.current < 0 ) sys.psi.current = 0;
	if ( sys.psi.target < 0 ) sys.psi.target = 0;
	if ( sys.psi.target > sys.settings.air.max_pressure ) sys.psi.target = sys.settings.air.max_pressure;
}


void Relays_Process( bool fire, bool forcce )
{
	static int delay = 0;
    static bool old_fire = 0, old_busy = false;

	psi_update();

	bool busy = (sys.psi.fill || sys.psi.dump);
	if ( !busy && old_busy ) {
		BEEP(50);
    }

	// Fire solenoid on rising edge, if not busy
	if ( !busy && !old_fire && fire ) {
		sys.psi.fire = fire;
		FIRE(sys.settings.air.fire_pulse);
		BEEP(50);
	}

	if ( sys.psi.fire ) {
		if ( !IS_FIRING() )
			sys.psi.fire = false;
	} else {
		// Target is exact while filling/dumping, then relaxes for holding
		int8_t margin = (busy || forcce)? 0: PSI_HOLD_MARGIN;
		if ( sys.psi.current < sys.psi.target - margin && !sys.psi.dump ) {				// Only fill if not dumping
			sys.psi.fill = true;
			delay = FILL_DELAY;
		} else if ( sys.psi.current > sys.psi.target + margin && !sys.psi.fill ) {		// Only dump if not filling
			sys.psi.dump = true;
			delay = DUMP_DELAY;
		} else 	if ( delay ) {
			delay--;
		} else {
			sys.psi.fill = false;
			sys.psi.dump = false;
		}
	}

	old_fire = fire;
    old_busy = busy;

	RELAY_SET( 1, sys.psi.fill );
	RELAY_SET( 2, sys.psi.dump );
	RELAY_SET( 4, 0 );
}

void Relays_Init( void )
{
	RELAY_INIT(1);
	RELAY_INIT(2);
	RELAY_INIT(3);
	RELAY_INIT(4);
	RELAY_INIT(5);	// Beeper

	//RocketRelay_Config_t config1 = {
	//	uint8_t		outPin;
	//	uint8_t		inPin;
	//}
	//RocketRelay_Init(sys.relays.relay1, )
	//nrf_gpio_pin_write( RELAY_1_OUT_PIN, 0 );
	//nrf_gpio_cfg_output( RELAY_1_OUT_PIN );
	//nrf_gpio_pin_write( RELAY_2_OUT_PIN, 0 );
	//nrf_gpio_cfg_output( RELAY_2_OUT_PIN );
	//nrf_gpio_pin_write( RELAY_3_OUT_PIN, 0 );
	//nrf_gpio_cfg_output( RELAY_3_OUT_PIN );
	//nrf_gpio_pin_write( RELAY_4_OUT_PIN, 0 );
	//nrf_gpio_cfg_output( RELAY_4_OUT_PIN );
}

void Relays_Off( void )
{
	Relays_Init();
}

void Relays_Update( void )
{
	RELAY_UPDATE(1);
	RELAY_UPDATE(2);
	RELAY_UPDATE(3);
	RELAY_UPDATE(4);
	RELAY_UPDATE(5);
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
//	if ( charger.chargeState != CHG_SHUTDOWN )
//		return;

	if ( sys.status.isOn )					// Only beep on shutdown if we successfully powered on
		nrf_gpio_pin_set( BEEPER_PIN );
    // Turn off all lights
	nrf_gpio_pin_clear( LED_PWM_PIN );
    LedsKill();

	vTaskDelay(50);
	nrf_gpio_pin_clear( BEEPER_PIN );

	suicide();
}

static void _beeper_on( void )
{
#ifdef BEEPER_PIN
	nrf_gpio_pin_write( BEEPER_PIN, 1 );
#endif
}

static void _beeper_off( void )
{
#ifdef BEEPER_PIN
	nrf_gpio_pin_write( BEEPER_PIN, 0 );
#endif
}

void UI_on_connect( bool connected )
{
    nrf_gpio_pin_write( LED_PWM_PIN, connected );
}

//################################################################################
//	System settings
//################################################################################
void	run_backlight( void )
{
	static uint8_t count = 0;
	nrf_gpio_pin_write( LED_PWM_PIN, ( count < sys.settings.display.bl_bright ) );
	if ( ++count >= 100 )
		count = 0;
}

void	system_monitor( void )
{
	run_backlight();

	static sys_settings_t oldSettings = { 0 };

	if ( sys.settings.display.bl_bright != oldSettings.display.bl_bright ) {
	}
	if ( sys.settings.display.bl_delay != oldSettings.display.bl_delay ) {
	}
	if ( sys.settings.display.contrast != oldSettings.display.contrast ) {
	}

    oldSettings = sys.settings;
}

void 	init_system( void )
{
	memset( &sys, 0, sizeof(sys) );

    sys.status.isOn = false;

	sys.settings.display.bl_bright = 100;
	sys.settings.display.bl_delay = 10;
	sys.settings.display.contrast = 50;

	sys.settings.power.activity_delay = 5;
	sys.settings.power.auto_discharge.enable = true;
	sys.settings.power.auto_discharge.delay = 3;
	sys.settings.power.auto_discharge.voltage = 380;

	sys.settings.stick.enable = false;
	sys.settings.stick.mode = STICK_MODE_SPLIT_LEFT;

    sys.settings.air.fire_pulse = FIRE_TIME;
	sys.settings.air.max_pressure = 50;
}

//################################################################################
//	Startup check
//################################################################################

bool	startup_check( void )
{
	if ( nrf_gpio_pin_read(BUTTON1_IN_PIN) != 0 ) // If button was not source of wakeup, then always wake up
		return true;

	//// Soft reset is valid for power on
	//uint32_t reset_reason = nrf_power_resetreas_get();
	//if ( reset_reason & NRF_POWER_RESETREAS_SREQ_MASK )
	//	return true;

	// Button woke us up, so check it for long enough hold
	uint8_t startup_delay = 10;
	while ( 1 ) {
        vTaskDelay( UI_THREAD_INTERVAL );
		ButtonProcess(&button, !nrf_gpio_pin_read(BUTTON1_IN_PIN), UI_THREAD_INTERVAL);

		if ( !startup_delay ) {
			if ( !button.on ) {
				return false;
			}
			if ( button.onTime > BUTTON_STARTUP_TIME_MS ) {
				return true;
			}
		}

		if ( startup_delay )
			startup_delay--;

		ButtonClearEvents( &button );
	}
}

//################################################################################
//	RTOS Thread
//################################################################################

static int32_t update_delay = 0;


static void ui_thread( void * arg )
{
	UNUSED_PARAMETER(arg);
	NRF_LOG_INFO("Thread Started: %s", pcTaskGetName(NULL));

	const TickType_t xFrequency = UI_THREAD_INTERVAL;
	TickType_t xLastWakeTime = xTaskGetTickCount();

	ReportMAC();

	if ( !startup_check() )
		_do_suicide();
	else
        sys.status.isOn = true;

	xTimerStart( updateTimer, 0 );

	_beeper_on();
	vTaskDelay( 50 );
	_beeper_off();


	Devices_Init();  // Initializes flash memory.

//	advertising_start(BLE_ADV_MODE_FAST);

	update_delay = 0;

	while ( 1 )
	{
#if 1
		vTaskDelayUntil( &xLastWakeTime, xFrequency );
		// Check if still behind after one makeup tick. If so, clear out backlogged ticks to keep from bogging down UI
		TickType_t time = xTaskGetTickCount();
		if ( xLastWakeTime+UI_THREAD_INTERVAL < time ) {
			xLastWakeTime = time;
		}
#else
        vTaskDelay( xFrequency );
#endif
		_adc_sample_and_wait();
		charger_update( &sys.charger, sys.battery.voltage );
//		charger_update( &sys.charger, ((float)MapLimit16( sys.io.raw.joy1y, 0, 16383, 3400, 4200))/1000.0f );

//#if 0
//		analog_in_update( &sys.io.sticks.joy1x, sys.io.raw.joy1x );
//		analog_in_update( &sys.io.sticks.joy1y, sys.io.raw.joy1y );
//		analog_in_update( &sys.io.sticks.joy2x, sys.io.raw.joy2x );
//		analog_in_update( &sys.io.sticks.joy2y, sys.io.raw.joy2y );
//#else
//#define STICK_MID 8192
//		analog_in_update( &sys.io.sticks.joy1x, STICK_MID );
//		analog_in_update( &sys.io.sticks.joy1y, STICK_MID );
//		analog_in_update( &sys.io.sticks.joy2x, STICK_MID );
//		analog_in_update( &sys.io.sticks.joy2y, STICK_MID );
//#endif

		ButtonProcess(&button, !nrf_gpio_pin_read(BUTTON1_IN_PIN), UI_THREAD_INTERVAL);
		ButtonProcess(&button2, !nrf_gpio_pin_read(BUTTON2_IN_PIN), UI_THREAD_INTERVAL);
#if 0
		ButtonProcess(&buttonJ1B1, !nrf_gpio_pin_read(BUTTON_J1B1_PIN), UI_THREAD_INTERVAL);
		ButtonProcess(&buttonJ2B1, !nrf_gpio_pin_read(BUTTON_J2B1_PIN), UI_THREAD_INTERVAL);
#else
		ButtonProcess(&buttonJ1B1, 0, UI_THREAD_INTERVAL);
		ButtonProcess(&buttonJ2B1, 0, UI_THREAD_INTERVAL);
#endif

		psi_update();
        Relays_Update();

		// TODO do stuff
//		send_channels();

		if ( button.onTime > BUTTON_SUICIDE_TIME ) {
			_do_suicide();
		}


#if 0
		if ( button.offEvent && button.offEvent < BUTTON_SHORTPRESS_TIME ) {
			if ( ++color_index >= NUM_COLORS ) color_index = 0;
            update_delay = 0;
		}

		// Process LED update code
		update_delay -= UI_THREAD_INTERVAL;
		if ( update_delay <= 0 ) {
			userLED = *color_list[color_index];
			//HACK because only 1 packet is allowed per connection interval on central side
			// So retry soon if this one failed
			if ( NRF_SUCCESS == send_color( (uint8_t*)&userLED ) )
				update_delay = UI_COLOR_UPDATE_INTERVAL;
		}
#endif

		MenuUpdate( MenuKeys_Collect() );

		system_monitor();

		ButtonClearEvents( &button );
		ButtonClearEvents( &button2 );
	}
}

//################################################################################
//	Mixer/channel handlers
//################################################################################

static void ppm_handler(ppm_capture_evt_t * p_evt)
{
	switch ( p_evt->type ) {
		case PPM_EVT_RX_DATA:
//			send_channels( p_evt->params.rx_data.channels, p_evt->params.rx_data.count );
			send_channels( p_evt->params.rx_data.channels, 2 );
			break;

		case PPM_EVT_TIMEOUT:

			break;
	}

//	static TickType_t lastTime = 0;
//	TickType_t newTime = xTaskGetTickCount();
//	TickType_t diff = newTime - lastTime;
//	NRF_LOG_DEBUG( "Interval: %d", diff );
//    lastTime = newTime;
}


static void _tx_timer_handler( TimerHandle_t xTimer )
{
	uint16_t channels[2];
    int16_t x;
    int16_t y;
	if ( sys.settings.stick.enable ) {
		switch ( sys.settings.stick.mode ) {
			case STICK_MODE_SPLIT_LEFT:
				x = Map16( sys.io.raw.joy2x, 0, 16383, -250, 250 );
				y = Map16( sys.io.raw.joy1y, 0, 16383, -500, 500 );
				channels[0] = Limit( -x - y, -500, 500 ) + 1500;
				channels[1] = Limit( -x + y, -500, 500 ) + 1500;
				break;

			case STICK_MODE_SPLIT_RIGHT:
				x = Map16( sys.io.raw.joy1x, 0, 16383, -250, 250 );
				y = Map16( sys.io.raw.joy2y, 0, 16383, -500, 500 );
				channels[0] = Limit( -x - y, -500, 500 ) + 1500;
				channels[1] = Limit( -x + y, -500, 500 ) + 1500;
				break;

			case STICK_MODE_SINGLE_LEFT:
				x = Map16( sys.io.raw.joy1x, 0, 16383, -250, 250 );
				y = Map16( sys.io.raw.joy1y, 0, 16383, -500, 500 );
				channels[0] = Limit( -x - y, -500, 500 ) + 1500;
				channels[1] = Limit( -x + y, -500, 500 ) + 1500;
				break;

			case STICK_MODE_SINGLE_RIGHT:
				x = Map16( sys.io.raw.joy2x, 0, 16383, -250, 250 );
				y = Map16( sys.io.raw.joy2y, 0, 16383, -500, 500 );
				channels[0] = Limit( -x - y, -500, 500 ) + 1500;
				channels[1] = Limit( -x + y, -500, 500 ) + 1500;
				break;

			case STICK_MODE_TANK:
				x = Map16( sys.io.raw.joy1y, 0, 16383, -500, 500 );
				y = Map16( sys.io.raw.joy2y, 0, 16383, -500, 500 );
				channels[0] = Limit( -x, -500, 500 ) + 1500;
				channels[1] = Limit( y, -500, 500 ) + 1500;
				break;
		}
	} else {
		channels[0] = 1500;
		channels[1] = 1500;
	}

	send_channels( &channels, 2 );
}

//################################################################################
//	Battery debug logging
//################################################################################
#define BATT_DEBUG_INTERVAL	60000
static TimerHandle_t batt_timer;

static void _batt_timer_handler( TimerHandle_t xTimer )
{
	NRF_LOG_INFO( "Batt, " NRF_LOG_FLOAT_MARKER ", %d", NRF_LOG_FLOAT(sys.battery.voltage), sys.charger.percent );
}

static void battery_debug_init( void )
{
	batt_timer = xTimerCreate( "BATT-TMR", BATT_DEBUG_INTERVAL, pdTRUE, NULL, _batt_timer_handler );
	if ( batt_timer == NULL ) {
		APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
	}
	xTimerStart( batt_timer, 0 );
}


//################################################################################
//	Init
//################################################################################

void	UI_Init( void )
{
	init_system();

	TP1_INIT();
	nrf_gpio_cfg_input( BUTTON1_IN_PIN, NRF_GPIO_PIN_PULLUP );
	//nrf_gpio_cfg_input( BUTTON2_IN_PIN, NRF_GPIO_PIN_PULLUP );

	Relays_Init();

#ifdef BEEPER_PIN
	nrf_gpio_cfg_output( BEEPER_PIN );
	nrf_gpio_pin_write( BEEPER_PIN, 0 );
#endif

#ifdef LED_PWM_PIN
	nrf_gpio_cfg_output( LED_PWM_PIN );
	nrf_gpio_pin_write( LED_PWM_PIN, 0 );
#endif

    LedsInit();
	//RGB_Fader_SetTarget( &rgbFaders[0], RGB_COLOR.MAGENTA, 1000 );
	//RGB_Fader_SetTarget( &rgbFaders[1], RGB_COLOR.MAGENTA, 1000 );
	//RGB_Fader_SetTarget( &rgbFaders[2], RGB_COLOR.MAGENTA, 1000 );
	//RGB_Fader_SetTarget( &rgbFaders[3], RGB_COLOR.MAGENTA, 1000 );

	_adc_init();

	// Create UI thread
	if ( pdPASS != xTaskCreate(ui_thread, "UI", 256, NULL, 2, &m_ui_thread) ) {
		APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
	}

	updateTimer = xTimerCreate( "TXTMR", 20, pdTRUE, NULL, _tx_timer_handler );
	if ( updateTimer == NULL ) {
		APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
	}
//	xTimerStart( updateTimer, 0 );


#ifdef PPM_IN_PIN
	ppm_capture_config_t config = {
		.pin		= PPM_IN_PIN,
		.handler	= ppm_handler,
		.timeout	= 0
	};
	ppm_capture_init( &config );
#endif

	mcp73833_config_t chargerConfig = {
		.stat1_pin	= CHG_STAT1_PIN,
		.stat2_pin	= CHG_STAT2_PIN,
		.pg_pin		= CHG_PG_PIN
	};
	charger_init( &sys.charger, &chargerConfig );

	battery_debug_init();
}
