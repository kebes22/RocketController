#include "MenuSystem.h"
#include "UI.h"
#include "Utils.h"

#include "gfx.h"
#include "gfx_battery.h"
#include "gfx_resources/gfx_resources.h"

#include "menu_images.h"
#include "analog_stick.h"

#include "tetris.h"

#define STR_BUF_LEN		33					//	Size of temporary string buffer
static char strBuf[STR_BUF_LEN];			//	Buffer for temporary string assembly

//################################################################################
//	String pointers
//################################################################################
//----------------------------------------
//	Menu message strings
//----------------------------------------
const char * hdrBack			= "Back";
const char * hdrMain			= "Main Menu";
const char * hdrSingle			= "Single Lifters";
const char * hdrGroups			= "Group Lifters";
const char * hdrAdd				= "Add New Lifters";
const char * hdrSettings		= "Settings";
const char * hdrBrightness		= "Brightness";
const char * hdrContrast		= "Contrast";
const char * hdrBacklight		= "Backlight Delay";
const char * hdrSleep			= "Sleep Delay";
const char * hdrAbout			= "About";
const char * msgDisconnect		= "Disconnect";

const char * msgConnecting		= "Connecting";
const char * msgPleaseWait		= "Please Wait...";
const char * msgStop			= "STOP";
const char * msgLoad			= "LOAD";

const char * hdrSetHigh			= "Set/Clear High";
const char * hdrSetLow			= "Set/Clear Low";

const char * strPercent			= "%";
const char * strSeconds			= "s";

//----------------------------------------
//	Popup message strings
//----------------------------------------
const char * hdrOverload		= "Overload";
const char * hdrError			= "Error";
const char * hdrWarning			= "Warning";

const char * msgOverload		= "Check for\nobstructions or\noverweight items.";
const char * msgTempError		= "Temperature range\nexceeded, please\try again later.";
const char * msgSoftLimit		= "Do you want to\nmove past this\nsmart point?";
const char * msgNoReference		= "Reference not\nset, please run\ncalibration.";
const char * msgNoSmartPoint	= "Smart point not\nset, please set\nbefore using.";
const char * msgGenericError	= "Problem detected.\nIf this persists\ncontact support.";

const char * msgSetSmart		= "Set smart point to\ncurrent position?\nOr clear it?";

//----------------------------------------
//	Button set strings
//----------------------------------------
const char * btnDelim			= "/";
const char * btnOk				= "OK";
const char * btnCancel			= "Cancel";
const char * btnOkCancel		= "OK/Cancel";
const char * btnYesNo			= "Yes/No";
const char * btnNoYes			= "No/Yes";
const char * btnSetClearCancel	= "Set/Clear/Cancel";


//################################################################################
//	Menu prototypes
//################################################################################
void	ProcessMenu( void );
void	MainMenu( void );
//void		Menu_SingleLifters( void );
//void			Menu_DevList( void );
//void			Menu_DevActions( void );
//void			Menu_Connected( void );
//void		Menu_GroupLifters( void );
//void		Menu_AddLifters( void );
void		Menu_Settings( void );
void			Menu_About( void );
void			Menu_FirmwareUpdate( void );


void	Menu_Popup();

void	Menu_Debug( void );

void	Menu_IO_Display( void );


//################################################################################
//	User menus
//################################################################################
extern const uint8_t myLIFTER_Splash[];
extern const uint8_t Benson_Splash[];
extern const uint8_t Legobot_Splash[];

static const uint8_t * p_image = NULL;
static void MenuImage( void )
{
	EVENT_LOAD_MENU {
		GLCD_DrawBitmap( p_image, 0, 0, false );
	}
	EVENT_BACK_KEY { MenuPop(); }
	EVENT_SELECT_KEY { MenuPop(); }
}

void MenuLoadImage( const uint8_t * image )
{
	p_image = image;
	MenuPush( MenuImage );
}

char *	get_adc_Str( const char * prefix, nrf_saadc_value_t * p_adc )
{
	sprintf( strBuf, "%s: %d", prefix, *(int16_t*)p_adc );
	return strBuf;
}

void Menu_ADC( void )
{
	EVENT_UPDATE_INTERVAL( 33 ) {
		SetCurrentUpdate( DRAW_UPDATE_MINOR );
	}

	MENU_START( "Raw ADC Values" );
	MENU_ITEM_TEXT( get_adc_Str( "BAT", &sys.io.raw.vBatt ) );
	MENU_ITEM_TEXT( get_adc_Str( "IN1", &sys.analog.an1.raw ) );
	MENU_ITEM_TEXT( get_adc_Str( "IN2", &sys.analog.an2.raw ) );
	MENU_ITEM_TEXT( get_adc_Str( "IN3", &sys.analog.an3.raw ) );
	MENU_ITEM_TEXT( get_adc_Str( "IN4", &sys.analog.an4.raw ) );
	MENU_END();

   	EVENT_BACK_KEY { MenuPop(); }
}

void	TetrisMenu( void )
{
	SetCurrentUpdate( DRAW_UPDATE_MINOR ); // Always update screen

	bool init = false;
	EVENT_LOAD_MENU {
		init = true;
	}

	if ( 0 == tetris_run( init, 10, currentKeys ) ) {
		MenuPop();
	}

}

#define ADC_BATT_SENSE			sys.io.raw.joy1x
#define BATT_RAW_MIN			0			//	Fully discharged battery
#define BATT_RAW_MAX			16383			//	Fully charged battery
#define BATT_RAW_WARNING		3500			//	Warning level

void	BatteryGameMenu( void )
{
	gfx_battery_t batt;
	SetCurrentUpdate( DRAW_UPDATE_MAJOR );
	GLCD_ClearAll();
	int16_t bW = Limit( Map16R( sys.io.raw.joy2x, BATT_RAW_MIN, BATT_RAW_MAX, 12, 128), 12, 128 );
	int16_t bH = Limit( Map16R( sys.io.raw.joy2y, BATT_RAW_MIN, BATT_RAW_MAX, 6, 64), 6, 64 );
	int16_t x1 = menuFrame.outer.mid.x - bW/2;
	int16_t y1 = menuFrame.outer.mid.y - bH/2;
	gfx_battery_create( &batt, x1, y1, bW, bH, false );

	int8_t percent = Limit( Map16R( ADC_BATT_SENSE, BATT_RAW_MIN, BATT_RAW_MAX, 0, 100), 0, 100 );
	gfx_battery_draw( &batt, lcdCurrentUpdate, percent, true, false );

	EVENT_BACK_KEY { MenuPop(); }
}

void	BatteryMenu( void )
{
	static bool flash = false;
	EVENT_UPDATE_INTERVAL( 50 ) {
		SetCurrentUpdate( DRAW_UPDATE_MAJOR );
		flash = sys.charger.chargeState == CHG_CHARGING? !flash: false;
	}

	if ( lcdCurrentUpdate >= DRAW_UPDATE_MINOR ) {
		gfx_battery_t batt;
		SetCurrentUpdate( DRAW_UPDATE_MAJOR );
		GLCD_ClearAll();
		int16_t bW = Limit( Map16R( sys.io.raw.joy2x, BATT_RAW_MIN, BATT_RAW_MAX, 12, 128), 12, 128 );
		int16_t bH = Limit( Map16R( sys.io.raw.joy2y, BATT_RAW_MIN, BATT_RAW_MAX, 6, 64), 6, 64 );
		int16_t x1 = menuFrame.outer.mid.x - bW/2;
		int16_t y1 = menuFrame.outer.mid.y - bH/2;
		gfx_battery_create( &batt, x1, y1, bW, bH, false );

		int8_t percent = Limit( sys.charger.percent - (flash? 30: 0), 0, 100 );
		gfx_battery_draw( &batt, lcdCurrentUpdate, percent, (sys.charger.chargeState == CHG_CHARGING? false: true), false );
	}

	EVENT_BACK_KEY { MenuPop(); }
}

void	RootMenu( void )
{
//	GroupStartDisconnect();														//	Should always be disconnected in main menu

	MENU_START( PSTR(hdrMain) );
	MENU_ITEM( PSTR(hdrSettings), MenuPush( Menu_Settings ) );
	//MENU_ITEM( PSTR("Sticks"), MenuPush( Menu_IO_Display ) );
	//MENU_ITEM( PSTR("TETRIS"), MenuPush( TetrisMenu ) );
	//MENU_ITEM( PSTR("Battery Game"), MenuPush( BatteryGameMenu ) );
	//MENU_ITEM( PSTR("Battery"), MenuPush( BatteryMenu ) );
	MENU_ITEM( PSTR("Raw ADC"), MenuPush( Menu_ADC ) );
	MENU_ITEM( PSTR("Debug"), MenuPush( Menu_Debug ) );
	MENU_ITEM( PSTR("Test Image 1"), MenuLoadImage( myLIFTER_Splash ) );
	MENU_ITEM( PSTR("Test Image 2"), MenuLoadImage( Benson_Splash ) );
//	MENU_ITEM( PSTR("Test Image 3"), MenuLoadImage( Legobot_Splash ) );
	MENU_ITEM( PSTR("Test 3"), Menu_Null_Action() );
	MENU_ITEM( PSTR("Test 4"), Menu_Null_Action() );
	MENU_ITEM( PSTR("Test 5"), Menu_Null_Action() );
	MENU_END();

	EVENT_BACK_KEY {
		MenuPop();
	}
}

static void _draw_stick( int16_t x_center, int16_t y_center, bool up_down, bool left_right )
{
	if ( up_down )
//		GLCD_DrawBitmap( (const uint8_t*)ArrowUpDown, x_center-16, y_center-16, false );
		GLCD_Draw_tImage( &ArrowUpDown, x_center-16, y_center-16, DRAW_ON );
	if ( left_right )
//		GLCD_DrawBitmap( (const uint8_t*)ArrowLeftRight, x_center-16, y_center-16, false );
		GLCD_Draw_tImage( &ArrowLeftRight, x_center-16, y_center-16, DRAW_ON );
}


#define STATUS_BAR_HEIGHT	12

static void _draw_status_bar( void )
{
	static gfx_battery_t batt;
	static bool flashBatt = false;
	static bool flashBle = false;
	flashBatt = sys.charger.chargeState == CHG_CHARGING? !flashBatt: false;
	flashBle = !flashBle;

//	if ( lcdCurrentUpdate >= DRAW_UPDATE_MAJOR ) {
	if ( lcdCurrentUpdate >= DRAW_UPDATE_MINOR ) {
		// Create battery
		int16_t bW = 20;
		int16_t bH = STATUS_BAR_HEIGHT - 2;
		int16_t x1 = menuFrame.outer.box.p2.x - bW;
		int16_t y1 = menuFrame.outer.box.p1.y + 1;
		gfx_battery_create( &batt, x1, y1, bW, bH, true );
		// Fill header
		GLCD_DrawFill( menuFrame.outer.box.p1.x, menuFrame.outer.box.p1.y, menuFrame.outer.box.p2.x, menuFrame.outer.box.p1.y+STATUS_BAR_HEIGHT-1, DRAW_ON );
	}

	if ( lcdCurrentUpdate >= DRAW_UPDATE_MINOR ) {
		// Update battery
		int8_t percent = Limit( sys.charger.percent - (flashBatt? 30: 0), 0, 100 );
		gfx_battery_draw( &batt, lcdCurrentUpdate, percent, (sys.charger.chargeState == CHG_CHARGING? false: true), false );
		// Update BLE icon
		GLCD_Draw_tImage( (flashBle? &icon_ble2: &icon_ble1), menuFrame.outer.box.p1.x + 1, menuFrame.outer.box.p1.y, DRAW_OFF );
	}
}

#define STICK_X_OFFSET	30
void	ControlMenu( void )
{
	EVENT_UPDATE_INTERVAL( 50 ) {
		SetCurrentUpdate( DRAW_UPDATE_MAJOR );
	}

	sys.settings.stick.enable = true;

	int8_t inc = 0;
	if ( currentKeys.onEvent.plus ) inc++;
	if ( currentKeys.onEvent.minus ) inc--;

	if ( inc != 0 ) {
		sys.settings.stick.mode += inc;
		if ( (int8_t)sys.settings.stick.mode > STICK_MODE_COUNT-1 ) sys.settings.stick.mode = 0;
		if ( (int8_t)sys.settings.stick.mode < 0 ) sys.settings.stick.mode = STICK_MODE_COUNT-1;
		SetCurrentUpdate( DRAW_UPDATE_MAJOR );
	}

	if ( lcdCurrentUpdate >= DRAW_UPDATE_MAJOR ) {
		gFrame_Clear( &menuFrame.inner );

		_draw_status_bar();

		int16_t y = menuFrame.inner.box.p1.y + 30;
		switch ( sys.settings.stick.mode ) {
			case STICK_MODE_SPLIT_LEFT:
				_draw_stick( menuFrame.inner.mid.x - STICK_X_OFFSET, y, true, false );
				_draw_stick( menuFrame.inner.mid.x + STICK_X_OFFSET, y, false, true );
				break;

			case STICK_MODE_SPLIT_RIGHT:
				_draw_stick( menuFrame.inner.mid.x - STICK_X_OFFSET, y, false, true );
				_draw_stick( menuFrame.inner.mid.x + STICK_X_OFFSET, y, true, false );
				break;

			case STICK_MODE_SINGLE_LEFT:
				_draw_stick( menuFrame.inner.mid.x - STICK_X_OFFSET, y, true, true );
				break;

			case STICK_MODE_SINGLE_RIGHT:
				_draw_stick( menuFrame.inner.mid.x + STICK_X_OFFSET, y, true, true );
				break;

			case STICK_MODE_TANK:
				_draw_stick( menuFrame.inner.mid.x - STICK_X_OFFSET, y, true, false );
				_draw_stick( menuFrame.inner.mid.x + STICK_X_OFFSET, y, true, false );
				break;
		}
	}

	EVENT_SELECT_KEY {
		sys.settings.stick.enable = false;
		MenuPush(RootMenu);
	}

	EVENT_BACK_KEY {
		MenuPop();
	}

//	static bool goMenu = false;
//	if ( currentKeys.repeatEvent.select ) {
//		goMenu = true;
//		MenuPush(RootMenu);
//	}
//	if ( goMenu && currentKeys.offEvent.select ) {
//		goMenu = false;
//		MenuPush(RootMenu);
//	}
}

char *	get_psi_str( uint8_t current, uint8_t target )
{
	sprintf( strBuf, "PSI: %d / %d", current, target );
	return strBuf;
}

char *	get_psi_raw( void )
{
	sprintf( strBuf, "PSI Raw: %d", sys.psi.current_raw );
	return strBuf;
}

void	PSI_Menu( void )
{
	EVENT_LOAD_MENU {
		sys.psi.target = sys.psi.current;
		Relays_Off();
	}

	EVENT_UPDATE_INTERVAL( 10 ) {
		SetCurrentUpdate( DRAW_UPDATE_MINOR );
	}

	uint8_t newTarget = sys.psi.target;
	if ( currentKeys.onEvent.minus || currentKeys.repeatEvent.minus ) newTarget--;
	if ( currentKeys.onEvent.plus || currentKeys.repeatEvent.plus ) newTarget++;
	bool changed = newTarget != sys.psi.target;
    sys.psi.target = newTarget;
	Relays_Process( currentKeys.onState.select, changed );

#if 0
	MENU_START( "PSI CONTROLLER" );
	MENU_ITEM_TEXT( get_psi_str( sys.psi.current, sys.psi.target ) );
	//MENU_ITEM_TEXT( get_psi_raw() );
	MENU_END();
#else
	//MENU_START( "PSI CONTROLLER" );
	////MENU_ITEM_TEXT( "PSI:" );
	//MENU_END();
#endif

#if 0
	if ( lcdCurrentUpdate >= DRAW_UPDATE_MINOR ) {
		font_t * newFont = (font_t*)Arial_bold_14;
		font_t * oldFont = GLCD_SetFont( newFont );
		GLCD_Goto( (64), (32) - (newFont->height>>1) );
		sprintf( strBuf, "%d / %d PSI", sys.psi.current, sys.psi.target );
		GLCD_PutStringAligned( strBuf, ALIGN_CENTER );
		GLCD_SetFont( oldFont );
	}
#else
	if ( lcdCurrentUpdate >= DRAW_UPDATE_MINOR ) {
		gFrame_Clear( &menuFrame.outer );
		sprintf( strBuf, "%d/%d", sys.psi.current, sys.psi.target );
		tFont * p_font = &ArialBlack36;
		int16_t x = 64 - (tLCD_Get_String_Length(strBuf, p_font) / 2);
		int16_t y = 2;
		tLCD_Print_String( strBuf, p_font, x, y );

		sprintf( strBuf, "PSI", sys.psi.current, sys.psi.target );
        x = 64 - (tLCD_Get_String_Length(strBuf, p_font) / 2);
		//y += tLCD_Get_Font_Height(p_font);
		y += 30;
		tLCD_Print_String( strBuf, p_font, x, y );
	}
#endif

	EVENT_BACK_KEY {
		//MenuPop();
		MenuPush(RootMenu);
	}

	EVENT_EXIT_MENU {
		Relays_Off();
	}
}


void	MainMenu( void )
{
	EVENT_LOAD_MENU { SetCurrentUpdate( DRAW_UPDATE_MAJOR ); }
	EVENT_RETURN_TO_MENU { SetCurrentUpdate( DRAW_UPDATE_MAJOR ); }

	if ( lcdCurrentUpdate >= DRAW_UPDATE_MINOR )
		GLCD_Draw_tImage( &RocketBotSplash, 0, 0, DRAW_ON );
		//GLCD_Draw_tImage( &LegoBotSplash, 0, 0, DRAW_ON );

	EVENT_UPDATE_INTERVAL( 200 ) {
		//MenuPush(ControlMenu);
		MenuPush(PSI_Menu);
	}
}

char *	GetBattStr( void )
{
	sprintf( strBuf, "Battery: %d", 0 );
	return strBuf;
}


//void	Menu_Auto_Discharge( void )
//{
//	MENU_START( PSTR("Batt Auto Discharge") );
//	MENU_ITEM( GetStrOnOff( PSTR("Enabled"), &sys.settings.auto_discharge.enable ), MenuToggleOnOff( &sys.settings.auto_discharge.enable ) );
//	if ( sys.settings.auto_discharge.enable ) {
//		MENU_ITEM( PSTR(" Delay"), MenuLoadSliderI8( "Discharge Delay", " days", &sys.settings.auto_discharge.delay, 1, 7, 1 ) );
//		MENU_ITEM( PSTR(" Voltage"), MenuLoadSliderI16fp( "Discharge Voltage", "V", &sys.settings.auto_discharge.voltage, 360, 400, 5, 2 ) );
//	}
//	MENU_END();

//	EVENT_BACK_KEY { MenuPop(); }
//}

//################################################################################
//	Settings menus
//################################################################################
void	Menu_Settings_Air( void )
{
	MENU_START( "Air Settings" );
	MENU_ITEM( "Fire Pulse Time", MenuLoadSliderI16( "Fire Pulse Time", AIR_FIRE_PULSE_UNIT, &sys.settings.air.fire_pulse, AIR_FIRE_PULSE_MIN, AIR_FIRE_PULSE_MAX, AIR_FIRE_PULSE_INC ) );
	MENU_ITEM( "Max Pressure", MenuLoadSliderI16( "Max Pressure", AIR_FIRE_PRESSURE_UNIT, &sys.settings.air.max_pressure, AIR_FIRE_PRESSURE_MIN, AIR_FIRE_PRESSURE_MAX, AIR_FIRE_PRESSURE_INC ) );
	MENU_END();
	EVENT_BACK_KEY { MenuPop(); }
}

void	Menu_Settings_Display( void )
{
	MENU_START( "Display Settings" );
	MENU_ITEM( "Contrast", MenuLoadSliderI8( "Contrast", LCD_CONTRAST_UNIT, &sys.settings.display.contrast, LCD_CONTRAST_MIN, LCD_CONTRAST_MAX, LCD_CONTRAST_INC ) );
	MENU_ITEM( "Brightness", MenuLoadSliderI8( "Brightness", BL_BRIGHT_UNIT, &sys.settings.display.bl_bright, BL_BRIGHT_MIN, BL_BRIGHT_MAX, BL_BRIGHT_INC ) );
	MENU_ITEM( "Backlight Delay", MenuLoadSliderI8( "Backlight Delay", BL_DELAY_UNIT, &sys.settings.display.bl_delay, BL_DELAY_MIN, BL_DELAY_MAX, BL_DELAY_INC ) );
	MENU_END();
	EVENT_BACK_KEY { MenuPop(); }
}

void	Menu_Settings_Power( void )
{
	MENU_START( "Power Settings" );
	MENU_ITEM( "Activity Timeout", MenuLoadSliderI8( "Activity Timeout", "min", &sys.settings.power.activity_delay, 1, 10, 1 ) );
	if ( sys.settings.power.auto_discharge.enable ) MENU_LINE(1);
	MENU_ITEM( GetStrOnOff( "Batt Auto-Discharge", &sys.settings.power.auto_discharge.enable ), MenuToggleOnOff( &sys.settings.power.auto_discharge.enable ) );
	if ( sys.settings.power.auto_discharge.enable ) {
		MENU_ITEM( " \x04" "Delay", MenuLoadSliderI8( "Discharge Delay", " days", &sys.settings.power.auto_discharge.delay, 1, 7, 1 ) );
		MENU_ITEM( " \x04" "Voltage", MenuLoadSliderI16fp( "Discharge Voltage", "V", &sys.settings.power.auto_discharge.voltage, 360, 400, 5, 2 ) );
		MENU_LINE(1);
	}
	MENU_END();
	EVENT_BACK_KEY { MenuPop(); }
}

void	Menu_Settings( void )
{
	MENU_START( "Settings" );
	MENU_ITEM( "Air Settings", MenuPush( Menu_Settings_Air ) );
	MENU_ITEM( "Display Settings", MenuPush( Menu_Settings_Display ) );
	MENU_ITEM( "Power Settings", MenuPush( Menu_Settings_Power ) );


	MENU_ITEM( PSTR(hdrAbout), MenuPush(Menu_About); SetNextUpdateOverride(DRAW_UPDATE_MAJOR) );
	MENU_ITEM( PSTR("Firmware Update"), Menu_Null_Action() );
	MENU_ITEM( GetBattStr(), Menu_Null_Action() );
	MENU_END();

	EVENT_BACK_KEY { MenuPop(); }
}




void	Menu_About( void )
{
	EVENT_LOAD_MENU {
		// Create and show empty popup
		Menu_CreatePopup( PSTR("About"), NULL, btnOk, 122, 53, false );
		SetCurrentUpdate( DRAW_UPDATE_MAJOR );
		Menu_ShowPopup();

		// Fill popup with custom info
		uint8_t x,y;
		x = popupFrame.inner.box.p1.x;
		y = popupFrame.inner.box.p1.y;

		SEQ_LOOP {
			SEQ_ITEM { sprintf( strBuf, "App Version  %d.%d", 0, 0 ); }
			SEQ_ITEM { sprintf( strBuf, "%s", "0/0/0" ); }
			SEQ_ITEM { sprintf( strBuf, "Boot Version %d.%d", 0, 0 ); }
			SEQ_ITEM { sprintf( strBuf, "HW Version   %d", 0 ); }
			GLCD_Goto( x, y );
			GLCD_PutString( strBuf );
			y += 8;
		}

		MenuPush( Menu_Popup );						//	Load menu
		SetNextUpdateOverride(DRAW_UPDATE_MINOR);			//	Drop back to minor update
	}

	EVENT_RETURN_TO_MENU {
		MenuPop();
	}

}


void	Menu_FirmwareUpdate( void )
{
//	SetCurrentUpdateOverride(DRAW_UPDATE_NONE);
//	EVENT_LOAD_MENU {
////		BLE_StartSlaveMode();
//		lcdCurrentUpdate = DRAW_UPDATE_FULL;
//	}

	MENU_START( PSTR("Firmware Update") );
	MENU_ITEM( PSTR("Cancel"), Menu_Null_Action(); MenuPop() );
	MENU_END();
}



void	Menu_Debug( void )
{
	static int8_t slider = 50;
	static uint8_t tog;

	MENU_ALIGNMENT( ALIGN_CENTER );

	MENU_START( PSTR("Debug") );
//	MENU_ITEM( PSTR(hdrBack), MenuPop() );
//	MENU_ITEM_BACK();
	MENU_ITEM( PSTR("Test Keypad"), MenuLoadKeypad() );
	MENU_ITEM( PSTR("Test Popup"), Menu_CreatePopup( PSTR("Header"), "Text", "BTN1/BTN2/BTN3", 100, 50, true ) );
//	MENU_ITEM( PSTR("Test Popup"), Menu_CreatePopup( PSTR("Header"), "Text", "YES/NO/CANCEL", 120, 50, true ) );
//	MENU_ITEM( PSTR("Test Popup"), Menu_CreatePopup( PSTR("Header"), "Text", "BTN1", 100, 50, true ) );
	MENU_ITEM( PSTR("Test Slider"), MenuLoadSliderI8( "Test Slider", strPercent, &slider, 0, 100, 1 ) );
	MENU_ITEM( GetStrOnOff( PSTR("Test Toggle"), &tog ), MenuToggleOnOff( &tog ) );
	MENU_ITEM( GetBattStr(), Menu_Null_Action() );
	MENU_ITEM( PSTR("Reset Bluetooth"), Menu_Null_Action() );
	MENU_ITEM( PSTR("Reset System"), Menu_Null_Action() );
	MENU_ITEM( PSTR("Sleep"), Menu_Null_Action() );
	MENU_END();

	EVENT_BACK_KEY { MenuPop(); }

//	SetNextUpdate( DRAW_UPDATE_MINOR );
//	SetNextUpdate( DRAW_UPDATE_MAJOR );
}


//################################################################################
//	User menus
//################################################################################
static bool stick_cal = 0;

static void _draw_joystick( int16_t x_center, int16_t y_center, int16_t w_2, int16_t h_2, int16_t x_val, int16_t y_val )
{
	// Draw box
	int16_t sz_2 = 2;
	int16_t x1 = x_center-w_2;
	int16_t x2 = x_center+w_2;
	int16_t y1 = y_center-h_2;
	int16_t y2 = y_center+h_2;
	GLCD_PlotRect( x1, y1, x2, y2 );

	int16_t w = 2;
	int16_t h = 2;
	//int16_t w = w_2-2;
	//int16_t h = h_2-2;
	int16_t x_new = MapLimit16( x_val, -980, 980, x1+w, x2-w );
	int16_t y_new = MapLimit16( y_val, -980, 980, y2-h, y1+h );
	//int16_t x_new = Map16( x_val, -995, 995, x1, x2 );
	//int16_t y_new = Map16( y_val, -995, 995, y2, y1 );
	GLCD_PlotRect( x_new-w, y_new-h, x_new+w, y_new+h );
	//GLCD_PlotPixel( x_new, y_new );
}

void	Menu_IO_Display( void )
{
	EVENT_LOAD_MENU {
		stick_cal = 0;
	}
 //   EVENT_SELECT_KEY {
	//	stick_cal = !stick_cal;
 //       if ( stick_cal ) {
	//		analog_in_start_cal( &sys.io.sticks.joy1x );
	//		analog_in_start_cal( &sys.io.sticks.joy1y );
	//		analog_in_start_cal( &sys.io.sticks.joy2x );
	//		analog_in_start_cal( &sys.io.sticks.joy2y );
	//	} else {
	//		analog_in_end_cal( &sys.io.sticks.joy1x );
	//		analog_in_end_cal( &sys.io.sticks.joy1y );
	//		analog_in_end_cal( &sys.io.sticks.joy2x );
	//		analog_in_end_cal( &sys.io.sticks.joy2y );
	//	}
	//}

	SetCurrentUpdate( DRAW_UPDATE_MAJOR );
	GLCD_ClearAll();
	_draw_status_bar();
	//_draw_joystick( 31, 31, 20, 15, sys.io.sticks.joy1x.value, sys.io.sticks.joy1y.value );
	//_draw_joystick( 95, 31, 20, 15, sys.io.sticks.joy2x.value, sys.io.sticks.joy2y.value );
    if ( stick_cal ) {
		GLCD_Goto( 64, 56 );
		GLCD_PutStringAligned( "Calibrating", ALIGN_CENTER );
	}

	EVENT_BACK_KEY { MenuPop(); }
}
