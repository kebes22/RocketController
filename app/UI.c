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
#include "hardware.h"

#include "ppm_capture.h"

#include "suicide_pin.h"

#include "MenuSystem.h"
#include "nrf_power.h"

#include "Utils.h"

#include "mcp73833_lipo.h"


#define UI_THREAD_INTERVAL		10
//#define UI_THREAD_INTERVAL		20

#define UI_COLOR_UPDATE_INTERVAL	1000




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
RGB_Color_t		userLED;

static RGB_Color_t const * color_list[] = {
	&RGB_COLOR.RED,
	&RGB_COLOR.GREEN,
	&RGB_COLOR.BLUE,

//	&RGB_COLOR.CYAN,
	&RGB_COLOR.MAGENTA,
	&RGB_COLOR.YELLOW,
};

#define NUM_COLORS	ARRAY_SIZE(color_list)
int8_t color_index = 0;

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


//static float vBatt = 0;
#if 0
//#define AN_COUNTS_PER_V		3048		// Measured
#define AN_COUNTS_PER_V		3038		// Measured (Fluke 289)
//#define AN_COUNTS_PER_V		762		// Measured
//#define AN_COUNTS_PER_V		191		// Measured
#else
// Remapping based on external 5V supply
#define AN_COUNTS_PER_V		3491		// Measured (Fluke 289)
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

#if 0
	nrf_saadc_channel_config_t adc_channel_config_joystick = HL_ADC_SIMPLE_CONFIG_SE( VBATT_JOY1X_IN, NRF_SAADC_REFERENCE_VDD4, NRF_SAADC_GAIN1_4 );
	adc_channel_config_joystick.pin_p = VBATT_JOY1X_IN;
	hl_adc_channel_register( &adc_channel_config_joystick, &sys.io.raw.joy1x, NULL );
	adc_channel_config_joystick.pin_p = VBATT_JOY1Y_IN;
	hl_adc_channel_register( &adc_channel_config_joystick, &sys.io.raw.joy1y, NULL );
	adc_channel_config_joystick.pin_p = VBATT_JOY2X_IN;
	hl_adc_channel_register( &adc_channel_config_joystick, &sys.io.raw.joy2x, NULL );
	adc_channel_config_joystick.pin_p = VBATT_JOY2Y_IN;
	hl_adc_channel_register( &adc_channel_config_joystick, &sys.io.raw.joy2y, NULL );

	adc_channel_config_joystick.pin_p = VBATT_JOY1_TRIM_IN;
	hl_adc_channel_register( &adc_channel_config_joystick, &sys.io.raw.joy1trim, NULL );
	adc_channel_config_joystick.pin_p = VBATT_JOY2_TRIM_IN;
	hl_adc_channel_register( &adc_channel_config_joystick, &sys.io.raw.joy2trim, NULL );
#else
	nrf_saadc_channel_config_t adc_channel_config_psi = HL_ADC_SIMPLE_CONFIG_SE( AN_PRESSURE_IN, NRF_SAADC_REFERENCE_INTERNAL, NRF_SAADC_GAIN1_3 );
	hl_adc_channel_register( &adc_channel_config_psi, &sys.psi.current_raw, NULL );
	
#endif

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

	menuKeyBits.up		= sys.io.sticks.joy2y.value > JOY_NAV_TH_HI;
	menuKeyBits.down	= sys.io.sticks.joy2y.value < JOY_NAV_TH_LO;
	menuKeyBits.right	= sys.io.sticks.joy2x.value > JOY_NAV_TH_HI;
	menuKeyBits.left	= sys.io.sticks.joy2x.value < JOY_NAV_TH_LO;

	menuKeyBits.minus	= (	vRatio > (BUTTON_UP_NOMINAL_RATIO - BUTTON_NOMINAL_BAND)
							&& vRatio < (BUTTON_UP_NOMINAL_RATIO + BUTTON_NOMINAL_BAND) );
	menuKeyBits.plus	= (	vRatio > (BUTTON_DN_NOMINAL_RATIO - BUTTON_NOMINAL_BAND)
							&& vRatio < (BUTTON_DN_NOMINAL_RATIO + BUTTON_NOMINAL_BAND) );
//	menuKeyBits.left	= 0;
//	menuKeyBits.right	= 0;

	menuKeyBits.select	= (	vRatio > (BUTTON_SEL_NOMINAL_RATIO - BUTTON_NOMINAL_BAND)
							&& vRatio < (BUTTON_SEL_NOMINAL_RATIO + BUTTON_NOMINAL_BAND) )
							|| buttonJ2B1.on || button2.on;

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

#define FILL_DELAY			40
#define DUMP_DELAY			30
#define FIRE_TIME			10

void psi_update( void )
{
	sys.psi.current_mv = (float)sys.psi.current_raw / ADC_PER_V_PSI * ADC_COMP_PSI;
	sys.psi.current = (int8_t)((sys.psi.current_mv - 0.5f) * 100.0f / 4.0f);

	if ( sys.psi.current < 0 ) sys.psi.current = 0;
	if ( sys.psi.target < 0 ) sys.psi.target = 0;
	if ( sys.psi.target > 50 ) sys.psi.target = 50;
}

void Relays_Process( bool fire, bool forcce )
{
	static int delay = 0;
    static bool old_fire = 0;

	psi_update();

	bool busy = (sys.psi.fill || sys.psi.dump);
	
	// Fire solenoid on rising edge, if not busy
	if ( !busy && !old_fire && fire ) {
		sys.psi.fire = fire;
		delay = FIRE_TIME;
	}

	if ( sys.psi.fire && delay ) {
		if ( --delay == 0 )
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

	nrf_gpio_pin_write( RELAY_1_OUT_PIN, sys.psi.fire );
	nrf_gpio_pin_write( RELAY_2_OUT_PIN, sys.psi.fill );
	nrf_gpio_pin_write( RELAY_3_OUT_PIN, sys.psi.dump );
}


void Relays_Init( void )
{
	nrf_gpio_pin_write( RELAY_1_OUT_PIN, 0 );
	nrf_gpio_cfg_output( RELAY_1_OUT_PIN );
	nrf_gpio_pin_write( RELAY_2_OUT_PIN, 0 );
	nrf_gpio_cfg_output( RELAY_2_OUT_PIN );
	nrf_gpio_pin_write( RELAY_3_OUT_PIN, 0 );
	nrf_gpio_cfg_output( RELAY_3_OUT_PIN );
	nrf_gpio_pin_write( RELAY_4_OUT_PIN, 0 );
	nrf_gpio_cfg_output( RELAY_4_OUT_PIN );
}

void Relays_Off( void )
{
	Relays_Init();
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
	nrf_gpio_pin_clear( LED_PWM_PIN );
	vTaskDelay(50);
//	nrf_gpio_pin_write( EN_5V_PIN, 0 );	// Turn off leds
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
	nrf_gpio_pin_write( LED_PWM_PIN, ( count < sys.settings.bl_bright ) );
	if ( ++count >= 100 )
		count = 0;
}

void	system_monitor( void )
{
	run_backlight();

	static sys_settings_t oldSettings = { 0 };

	if ( sys.settings.bl_bright != oldSettings.bl_bright ) {
	}
	if ( sys.settings.bl_delay != oldSettings.bl_delay ) {
	}
	if ( sys.settings.contrast != oldSettings.contrast ) {
	}

    oldSettings = sys.settings;
}

void 	init_system( void )
{
	sys.settings.bl_bright = 100;
	sys.settings.bl_delay = 10;
	sys.settings.contrast = 50;

	sys.settings.auto_discharge.enable = true;
	sys.settings.auto_discharge.delay = 3;
	sys.settings.auto_discharge.voltage = 380;

	sys.settings.stick.enable = false;
	sys.settings.stick.mode = STICK_MODE_SPLIT_LEFT;
}

//################################################################################
//	Startup check
//################################################################################

bool	startup_check( void )
{
	return true; // NOTE because we don't have a suicide circuit

	// Soft reset is valid for power on
	uint32_t reset_reason = nrf_power_resetreas_get();
	if ( reset_reason & NRF_POWER_RESETREAS_SREQ_MASK )
		return true;

	static uint8_t startup_delay = 10;
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

		// TODO add logic for other wake up reasons like reset

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

#if 0
		analog_in_update( &sys.io.sticks.joy1x, sys.io.raw.joy1x );
		analog_in_update( &sys.io.sticks.joy1y, sys.io.raw.joy1y );
		analog_in_update( &sys.io.sticks.joy2x, sys.io.raw.joy2x );
		analog_in_update( &sys.io.sticks.joy2y, sys.io.raw.joy2y );
#else
#define STICK_MID 8192
		analog_in_update( &sys.io.sticks.joy1x, STICK_MID );
		analog_in_update( &sys.io.sticks.joy1y, STICK_MID );
		analog_in_update( &sys.io.sticks.joy2x, STICK_MID );
		analog_in_update( &sys.io.sticks.joy2y, STICK_MID );
#endif

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

		// TODO do stuff
//		send_channels();

		if ( button.offEvent && button.offEvent < BUTTON_SHORTPRESS_TIME ) {
			if ( ++color_index >= NUM_COLORS ) color_index = 0;
            update_delay = 0;
		}

		if ( button.onTime > BUTTON_SUICIDE_TIME ) {
			_do_suicide();
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
	nrf_gpio_cfg_input( BUTTON2_IN_PIN, NRF_GPIO_PIN_PULLUP );
#if 0
	nrf_gpio_cfg_input( BUTTON_J1B1_PIN, NRF_GPIO_PIN_PULLUP );
	nrf_gpio_cfg_input( BUTTON_J2B1_PIN, NRF_GPIO_PIN_PULLUP );
#endif

	Relays_Init();

#ifdef BEEPER_PIN
	nrf_gpio_cfg_output( BEEPER_PIN );
	nrf_gpio_pin_write( BEEPER_PIN, 0 );
#endif

#ifdef LED_PWM_PIN
	nrf_gpio_cfg_output( LED_PWM_PIN );
	nrf_gpio_pin_write( LED_PWM_PIN, 0 );
#endif

	_adc_init();

	analog_in_init( &sys.io.sticks.joy1x );
	analog_in_init( &sys.io.sticks.joy1y );
	analog_in_init( &sys.io.sticks.joy2x );
	analog_in_init( &sys.io.sticks.joy2y );

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
