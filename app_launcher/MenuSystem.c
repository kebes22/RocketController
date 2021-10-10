#include "MenuSystem.h"
//#include "Globals.h"
//#include "Menus.h"
#include "Utils.h"
#include "Stack.h"
//#include "UI.h"
#include "io_config.h"
#include "nrf_gpio.h"

#include <stdio.h>
#include <string.h>



//########################################################################################################################
//	String helpers and program memory access
//########################################################################################################################

#define STR_BUF_LEN		33					//	Size of temporary string buffer
#define PSTR_BUF_LEN	21					//	Max size of temp buffer for ROM string extraction

static char strBuf[STR_BUF_LEN];				//	Buffer for temporary string assembly
static char nameBuf[STR_BUF_LEN];				//	Buffer for name string editing
static char pStrBuf[PSTR_BUF_LEN];				//	Buffer for extraction of strings from ROM
//char	hdrBuf[HDR_BUF_LEN];				//	Buffer for storage of dynamic header strings



//########################################################################################################################
//	LCD update flag stuff
//########################################################################################################################

uint8_t	lcdCurrentUpdate = DRAW_UPDATE_FULL;
uint8_t	lcdNextUpdate = DRAW_UPDATE_FULL;

//#define SetNextUpdate(val)		lcdNextUpdate |= val
//#define SetCurrentUpdate(val)	lcdCurrentUpdate |= val

//extern uint8_t	lcdCurrentUpdate;
//extern uint8_t	lcdNextUpdate;

//// Or the new level with the old level, just in case it is already set higher
//#define SetNextUpdate(val)					lcdNextUpdate |= (val)
//#define SetNextUpdateOverride(val,ovr)		lcdNextUpdate = (val)
//#define SetCurrentUpdate(val)				lcdCurrentUpdate |= (val)
//#define SetCurrentUpdateOverride(val,ovr)	lcdCurrentUpdate = (val)
//
//inline void SetNextUpdate(drawUpdate_t val) {
//	lcdNextUpdate |= (val);
//}
//inline void SetNextUpdateOverride(drawUpdate_t val) {
//	lcdNextUpdate = (val);
//}
//inline void SetCurrentUpdate(val) {
//	lcdCurrentUpdate |= (val)
//}
//inline void SetCurrentUpdateOverride(val,ovr) {
//	lcdCurrentUpdate = (val)
//}

//########################################################################################################################
//	Common Helper Menu Prototypes
//########################################################################################################################
//void		MenuLoadSliderI8( const char * str, int8_t *slider, int8_t min, int8_t max, int8_t inc );


int8_t		selected1 = 0;
int8_t		selected2 = 0;
int8_t		selected1_dir = 0;
int8_t		selected2_dir = 0;

Menu_Keys_t currentKeys;


const MenuStackFrame_t nullMenu = { MENU_STATE_LOAD, NULL, 0, 0, 0, 0 };


typedef struct
{
	MenuStackFrame_t	menus[MENU_MAX_DEPTH];
	Stack_t				stack;
}	MenuStack_t;

static MenuStack_t			menuStack;						//	Stack used for menu navigation
static MenuStackFrame_t		*nextMenu = NULL;				//	Next menu stack frame to be loaded


MenuStackFrame_t	*currentMenu = NULL;			//	Pointer to current menu stack frame
MenuParams_t		menuTemp;						//	Temporary menu tracking variables


//########################################################################################################################
//	Graphics objects
//########################################################################################################################
gPopup_t		menuFrame;
gPopup_t		popupFrame;
gButton_t		menuButtons[MENU_MAX_BUTTONS];
gProgressBar_t	progressBar;

//########################################################################################################################
//	"Private" helper functions used by the macros
//########################################################################################################################

void	MenuStartProcess( char * title )
{
	if ( currentKeys.onEvent.select ) {
		SetCurrentUpdate( DRAW_UPDATE_MINOR );
	}

	menuTemp.pixelRow = 1;

	bool update = false;
	if ( selected1 < 0 ) {
		selected1 = 0;
		update = true;
	}
	if ( selected1 < menuTemp.firstVisible ) {
		menuTemp.firstVisible = selected1;
		update = true;
	}
	if ( selected1 >= menuTemp.numItems ) {
		selected1 = menuTemp.numItems - 1;
		update = true;
	}
	if ( selected1 > menuTemp.lastVisible ) {
		menuTemp.firstVisible += selected1-menuTemp.lastVisible;
		update = true;
	}
	if ( update )
		SetCurrentUpdate( DRAW_UPDATE_MINOR );

	if ( lcdCurrentUpdate >= DRAW_UPDATE_MAJOR ) {
		gPopup_Draw( &menuFrame, title );
		//TODO should be based on font height/width
//		menuTemp.numRows = menuFrame.inner.dim.y / 8;
		menuTemp.numCols = (menuFrame.inner.dim.x / 6)-1;
	}
	if ( lcdCurrentUpdate >= DRAW_UPDATE_MINOR ) {
		gFrame_Draw( &menuFrame.inner );
	}
}

bool	MenuItemProcess( char * name, bool selectable )
{
	bool execute = false;
	bool isSelected = (menuTemp.itemIndex == selected1);
	if ( isSelected && !selectable ) {
		isSelected = false;
		selected1 += selected1_dir;
	}
	bool topVisible = ( menuTemp.pixelRow <= menuFrame.inner.dim.y );
	if ( topVisible ) {
		// Only process if it is the current item
		if ( menuTemp.itemIndex == menuTemp.currentItem ) {
			if ( lcdCurrentUpdate ) {
				uint8_t row = menuTemp.pixelRow;
				int8_t xOff = -(int8_t)menuTemp.alignment;
				gFrame_DrawText( &menuFrame.inner, name, strBuf, menuTemp.alignment, xOff, row );
				row += menuFrame.inner.box.p1.y;
				if ( isSelected ) {
					if ( currentKeys.onState.select )
						GLCD_DrawFill( menuFrame.inner.box.p1.x, row-1, menuFrame.inner.box.p2.x, row+7, DRAW_XOR );
					else
						GLCD_PlotRect( menuFrame.inner.box.p1.x, row-1, menuFrame.inner.box.p2.x, row+7 );
				}
			}
			if ( isSelected && currentKeys.offEvent.select ) {
				execute = true;
				SetNextUpdate( DRAW_UPDATE_MAJOR );
			}

			//TODO should be based on font height
			menuTemp.pixelRow += 8;
			if ( menuTemp.pixelRow <= menuFrame.inner.dim.y )
				menuTemp.lastVisible = menuTemp.itemIndex; // Update last visible, if whole thing is visible

		}
	}
	// Always increment index
	menuTemp.itemIndex++;
	return execute;
}

bool	MenuDrawLine( uint8_t thickness )
{
	if ( lcdCurrentUpdate && menuTemp.itemIndex == menuTemp.currentItem ) {
		uint8_t row = menuTemp.pixelRow + 1;
		row += menuFrame.inner.box.p1.y;
//		GLCD_PlotLine( menuFrame.inner.box.p1.x, row-1, menuFrame.inner.box.p2.x, row-1 );
		GLCD_PlotRect( menuFrame.inner.box.p1.x, row-1, menuFrame.inner.box.p2.x, row-1+thickness-1 );
		menuTemp.pixelRow += 1+thickness;
	}
}


void	MenuEndProcess( void )
{
	menuTemp.numItems = menuTemp.itemIndex;
	if ( lcdCurrentUpdate && (menuTemp.firstVisible > 0 || menuTemp.lastVisible < menuTemp.numItems - 1) )
		gFrame_DrawScrollBar( &menuFrame.inner, selected1, menuTemp.numItems, true );
}

//########################################################################################################################
//	Menu control functions
//########################################################################################################################
static void	LoadNextMenu( void )
{
	if ( nextMenu ) {
		currentMenu = nextMenu;										//	Switch pointers
		nextMenu = NULL;											//	No more next
		//	Reload saved parameters
		menuTemp.firstVisible = currentMenu->firstVisible;
		menuTemp.numItems = currentMenu->numItems;
		menuTemp.alignment = ALIGN_LEFT;
		selected1 = currentMenu->selected1;
		selected2 = currentMenu->selected2;
	}
}


void	MenuPush( MENU_FUNC nextMenuPtr )
{
	if ( nextMenu ) return;											//	Don't load another until one is processed !!! Debug only
	int8_t nextIdx = StackPush( &menuStack.stack );
	if ( nextIdx >= 0 ) {
		if ( currentMenu ) {
			currentMenu->firstVisible = menuTemp.firstVisible;
			currentMenu->numItems = menuTemp.numItems;
			currentMenu->selected1 = selected1;
			currentMenu->selected2 = selected2;
		}
		nextMenu = &menuStack.menus[nextIdx];						//	Get pointer
		*nextMenu = nullMenu;										//	Initialize parameters
		nextMenu->menu = nextMenuPtr;								//	Set menu pointer
		SetNextUpdate( DRAW_UPDATE_FULL );							//	Request screen update
	} else {														//	Menu stack is full
	}
}

void	MenuPop( void )
{
	int8_t last = StackPop( &menuStack.stack );
	if ( last >= 0 ) {
		nextMenu = &menuStack.menus[last];							//	Get pointer
		nextMenu->state = MENU_STATE_RETURN;						//	Flag return from another menu
		currentMenu->state = MENU_STATE_BACK;						//	Flag going back
		SetNextUpdate( DRAW_UPDATE_FULL );							//	Request screen update
	}
}

void	MenuReset( void )
{
	StackInit( &menuStack.stack, MENU_MAX_DEPTH );
	MenuPush( MainMenu );
}



//########################################################################################################################
//	Base menu routines
//########################################################################################################################
bool	menuInit = false;
extern const nrf_lcd_t nrf_lcd_st7565;

void	MenuInit( void )
{

	GLCD_Init( &nrf_lcd_st7565, NRF_LCD_ROTATE_180 );

	gPopup_Create( &menuFrame, GLCD_WIDTH, GLCD_HEIGHT );						//	Create main frame
	gFrame_SetBorder( &menuFrame.outer, 0, 0 );									//	No border
	//TODO should be based on font height
	gPopup_SetMargins( &menuFrame, 0, 7, 0, 0 );								//	Set margins for header

	MenuReset();

	DebounceInit( &currentKeys.keyDebounce, MENU_KEY_DEBOUNCE_TIME );
	menuInit = true;
}

static bool _processKeys( Menu_KeyBits_t keyBits )
{
	static uint32_t old = 0;
	static int16_t repeatDelay = 0;
	bool repeat = false;
	uint32_t newBits = DebounceProcess( &currentKeys.keyDebounce, keyBits.all );

	uint32_t changed = newBits ^ old;
	currentKeys.onState.all = newBits;
	currentKeys.onEvent.all |= changed & newBits;
	currentKeys.offEvent.all |= changed & old;
	if ( newBits && !changed && repeatDelay ) {
		if ( --repeatDelay <= 0 ) {
			currentKeys.repeatEvent.all = newBits;
			repeatDelay = MENU_KEY_REPEAT_DELAY;
			repeat = true;
		}
	} else {
		repeatDelay = MENU_KEY_REPEAT_FIRST_DELAY;
	}
	currentKeys.anyOnEvent.all = currentKeys.onEvent.all | currentKeys.repeatEvent.all;

	old = newBits;
//	if ( changed ) {
////		BacklightOn();
////		SleepSuspend( 0, 0 );
//	}

	//	Process automatic selectors
	int8_t old1 = selected1;
	int8_t old2 = selected2;
	if ( currentKeys.onEvent.up || currentKeys.repeatEvent.up ) selected1--;
	if ( currentKeys.onEvent.down || currentKeys.repeatEvent.down ) selected1++;
	if ( currentKeys.onEvent.left || currentKeys.repeatEvent.left ) selected2--;
	if ( currentKeys.onEvent.right || currentKeys.repeatEvent.right ) selected2++;

	if ( currentKeys.onEvent.minus || currentKeys.repeatEvent.minus ) selected1--;
	if ( currentKeys.onEvent.plus || currentKeys.repeatEvent.plus ) selected1++;
	if ( currentKeys.onEvent.minus || currentKeys.repeatEvent.minus ) selected2--;
	if ( currentKeys.onEvent.plus || currentKeys.repeatEvent.plus ) selected2++;

//	if ( currentKeys.onEvent.up || currentKeys.repeatEvent.up ) selected2--;
//	if ( currentKeys.onEvent.down || currentKeys.repeatEvent.down ) selected2++;

//	if ( selected1 != old1 || selected2 != old2 )
	if ( changed || repeat ) {	// At least may be a minor update if any keys changed, or repeat is executed
		SetNextUpdate( DRAW_UPDATE_MINOR );
	}

    selected1_dir = selected1 > old1? 1: selected1 < old1? -1: 0;
    selected2_dir = selected2 > old2? 1: selected2 < old2? -1: 0;
}

static void _clearKeyEvents( void )
{
	currentKeys.onEvent.all = 0;
	currentKeys.offEvent.all = 0;
	currentKeys.repeatEvent.all = 0;
}

bool	MenuUpdate( Menu_KeyBits_t keyBits )
{
TP1_ON();

	if ( !menuInit ) MenuInit();

	LoadNextMenu();																//	Check for next menu to load

	_processKeys( keyBits );

	SetCurrentUpdateOverride(lcdNextUpdate);
	SetNextUpdateOverride(DRAW_UPDATE_NONE);
	if ( lcdCurrentUpdate & DRAW_UPDATE_FULL )									//	Only erase all if full update is requested
		GLCD_ClearAll();
	TP1_OFF();
	TP1_ON();

	currentMenu->menu();														//	Call menu pointer

TP1_OFF();
TP1_ON();

	_clearKeyEvents();

	if ( lcdCurrentUpdate )
		GLCD_Update();

TP1_OFF();

	return lcdCurrentUpdate;													//	Return whether update is required to LCD
//	return true;													//	Return whether update is required to LCD
}

void	Menu_Null_Action( void )
{
	// Do nothing this is a dummy function
}



//########################################################################################################################
//	Menu graphics helpers, may be able to move to own library
//########################################################################################################################


static void _GrayOutBackground( void )
{
	drawMode_t old = GLCD_SetMode( DRAW_OFF );
	for ( int16_t y = 0; y < GLCD_HEIGHT; y++ ) {
		for ( int16_t x = 0; x < GLCD_WIDTH; x++ ) {
//			if ( (x+y) % 5 == 0 )
			if ( ((y%2)*2 + x) % 4 == 0 )
				GLCD_PlotPixel(x,y);
		}
	}
    GLCD_SetMode( old );
}




//#define KEYBOARD_WIDTH	10
//#define KEYBOARD_HEIGHT	4
//
//const char keyboard[4][KEYBOARD_WIDTH+1] =
//{
//	{ " 0123456789" },
//	{ "ABCDEFGHIJKLM" },
//	{ "NOPQRSTUVWXYZ" },
//	{ " " }
//};

//#define KEYBOARD_WIDTH	10
//#define KEYBOARD_HEIGHT	4
//
//const char keyboard[4][KEYBOARD_WIDTH+1] =
//{
//	{ "0123456789" },
//	{ "ABCDEFGHIJ" },
//	{ "KLMNOPQRST" },
//	{ "UVWXYZ" }
//};

//#define KEYBOARD_WIDTH	13
//#define KEYBOARD_HEIGHT	4
//
//const char keyboardLower[4][KEYBOARD_WIDTH+1] =
//{
//	{ "`1234567890-=" },
//	{ "qwertyuiop[]\\" },
//	{ "asdfghjkl;'" },
//	{ "zxcvbnm,./ \x7F" },
//};
//
//const char keyboardUpper[4][KEYBOARD_WIDTH+1] =
//{
//	{ "~!@#$%^&*()_+" },
//	{ "QWERTYUIOP{}|" },
//	{ "ASDFGHJKL:\"" },
//	{ "ZXCVBNM<>? \x7F" },
//};

#define KEYBOARD_WIDTH	12
#define KEYBOARD_HEIGHT	4
#define KEYBUTTON_WIDTH	6

const char keyboardLower[KEYBOARD_HEIGHT][KEYBOARD_WIDTH+1] =
{
	{ "@#${}abcdefg" },
	{ "%^&;'hijklmn" },
	{ "*_+[]opqrstu" },
	{ "-=|/\\vwxyz \x7F" },
};

const char keyboardUpper[KEYBOARD_HEIGHT][KEYBOARD_WIDTH+1] =
{
	{ "789()ABCDEFG" },
	{ "456:\"HIJKLMN" },
	{ "123<>OPQRSTU" },
	{ ",0.?!VWXYZ \x7F" },
};

const char keyButtonText[KEYBOARD_HEIGHT][KEYBUTTON_WIDTH+1] =
{
	{ "Shift" },
	{ "Clear" },
	{ "Cancel" },
	{ "Save" },
};

//const char keyboardLower[KEYBOARD_HEIGHT][KEYBOARD_WIDTH+1] =
//{
//	{ "abcdefg@#${}" },
//	{ "hijklmn%^&;'" },
//	{ "opqrstu*_+[]" },
//	{ "vwxyz \x7F-=|/\\" },
//};
//
//const char keyboardUpper[KEYBOARD_HEIGHT][KEYBOARD_WIDTH+1] =
//{
//	{ "ABCDEFG789()" },
//	{ "HIJKLMN456:\"" },
//	{ "OPQRSTU123<>" },
//	{ "VWXYZ \x7F,0.?!" },
//};
//
//const char keyButtonText[KEYBOARD_HEIGHT][KEYBUTTON_WIDTH+1] =
//{
//	{ "Shift" },
//	{ "Clear" },
//	{ "Cancel" },
//	{ "Save" },
//};

//const char keyboardLower[KEYBOARD_HEIGHT][KEYBOARD_WIDTH+1] =
//{
//	{ "abcdefghij \x7F" },
//	{ "klmnopqrst;'" },
//	{ "uvwxyz[]{}/\\" },
//	{ "@#$%^&*_+-=|" },
//};
//
//const char keyboardUpper[KEYBOARD_HEIGHT][KEYBOARD_WIDTH+1] =
//{
//	{ "ABCDEFGHIJ \x7F" },
//	{ "KLMNOPQRST:\"" },
//	{ "UVWXYZ,.<>?!" },
//	{ "1234567890()" },
//};
//
//const char keyButtonText[KEYBOARD_HEIGHT][KEYBUTTON_WIDTH+1] =
//{
//	{ "Shift" },
//	{ "Clear" },
//	{ "Cancel" },
//	{ "Save" },
//};

//const char keyboardLower[KEYBOARD_HEIGHT][KEYBOARD_WIDTH+1] =
//{
//	{ "1234567890-=" },
//	{ "qwertyuiop[]" },
//	{ "asdfghjkl;'\\" },
//	{ "zxcvbnm,./ \x7F" },
//};
//
//const char keyboardUpper[KEYBOARD_HEIGHT][KEYBOARD_WIDTH+1] =
//{
//	{ "!@#$%^&*()_+" },
//	{ "QWERTYUIOP{}" },
//	{ "ASDFGHJKL:\"|" },
//	{ "ZXCVBNM<>? \x7F" },
//};

//const char keyButtonText[KEYBOARD_HEIGHT][KEYBUTTON_WIDTH+1] =
//{
//	{ "Clear" },
//	{ "Reset" },
//	{ "Cancel" },
//	{ "Save" },
//};



char	MenuDrawKeyboard( uint8_t selRow, uint8_t selCol, bool sel, bool upper )
{
	//	updateLevel: 0=no change, 1=selection change, 2=text change, 3=full draw
	static uint8_t w, h;
	static uint8_t xStart, yStart, yText, bStart;
	static uint8_t lastRow, lastCol;
	static bool lastSel;

	uint8_t x, y;

	const char (*keyboard)[KEYBOARD_WIDTH+1] = upper?keyboardUpper:keyboardLower;	//	Get pointer to current keyboard

	//	Full screen update
	if ( lcdCurrentUpdate >= DRAW_UPDATE_FULL ) {													//	Full update, calculate parameters, and draw whole frame
		w = (KEYBOARD_WIDTH+KEYBUTTON_WIDTH+1)*6;								//	Width (including buttons)
		h = (KEYBOARD_HEIGHT+2)*8;												//	Height (including header and text string + extra spacing)

		gPopup_Create( &popupFrame, w+2, h+6 );
		gPopup_SetMargins( &popupFrame, 2, 9, 2, 1 );
		gPopup_Draw( &popupFrame, PSTR("Enter New Name:") );

		xStart = popupFrame.inner.box.p1.x;										//	Left edge of keypad area
		yText = popupFrame.inner.box.p1.y + 1;									//	Top edge of displayed text string
		yStart = yText + 12;													//	Top edge of keypad area
		bStart = (KEYBOARD_WIDTH+1)*6 + xStart;									//	Left edge of buttons
	} else {																	//	Not a full update, deselect old value
		x = xStart + (lastCol*6);
		y = yStart + (lastRow*8);
		if ( lastSel ) {
			drawMode_t mode = GLCD_SetMode( DRAW_OFF );
			GLCD_PlotRect( x-1, y-1, x+5, y+7 );
			GLCD_SetMode( mode );
		}
		else
			GLCD_DrawFill( x-1, y-1, x+5, y+7, DRAW_XOR );
	}

	//	Name has changed
	if ( lcdCurrentUpdate >= DRAW_UPDATE_MAJOR ) {													//	Displayed text has changed
		GLCD_DrawFill( popupFrame.outer.box.p1.x, yText-1, popupFrame.outer.box.p2.x, yText+7, DRAW_OFF );	//	Erase text area
		GLCD_Goto( xStart, yText );
		GLCD_PutString( nameBuf );
		GLCD_PlotLine( lcd.xPos, lcd.yPos-1, lcd.xPos, lcd.yPos+7 );
		GLCD_PlotLine( popupFrame.outer.box.p1.x, yText+9, popupFrame.outer.box.p2.x, yText+9 );

		x = xStart;
		y = yStart;

		uint8_t r;
		for ( r=0; r<KEYBOARD_HEIGHT; r++ )
		{
			GLCD_Goto( x, y );
			GLCD_PutString( keyboard[r] );
			GLCD_Goto( bStart, y );
			GLCD_PutString( keyButtonText[r] );
			y += 8;
		}
	}

	//	Always draw new selection
	x = xStart + (selCol*6);
	y = yStart + (selRow*8);
	if ( sel )
		GLCD_PlotRect( x-1, y-1, x+5, y+7 );
	else
		GLCD_DrawFill( x-1, y-1, x+5, y+7, DRAW_XOR );

	//	Store last selected
	lastRow = selRow;
	lastCol = selCol;
	lastSel = sel;

	return keyboard[selRow][selCol];
}


void	MenuKeypad( void )
{
	static int8_t keyRow = 0;
	static int8_t keyCol = 0;
	static bool upper = true;

	EVENT_LOAD_MENU {								//	Check if this menu was just loaded
		upper = true;							//	Default is upper case keypad
		keyRow = keyCol = 0;					//	Reset row/col
		selected1 = selected2 = 0;				//	Reset selected to zero
		SetCurrentUpdate( DRAW_UPDATE_FULL );
//		SetCurrentUpdate( DRAW_UPDATE_MAJOR );
		_GrayOutBackground();
	}

	char c = MenuDrawKeyboard( keyRow, keyCol, currentKeys.onState.select, upper );

	if ( selected1 || selected2 ) {
		SetNextUpdate( DRAW_UPDATE_MINOR );
		//	Process move keys
		keyRow += selected1;
		if ( keyRow >= KEYBOARD_HEIGHT ) keyRow = 0;
		else if ( keyRow < 0 ) keyRow = KEYBOARD_HEIGHT-1;
		keyCol += selected2;
		if ( keyCol >= KEYBOARD_WIDTH + 1 ) keyCol = 0;
		else if ( keyCol < 0 ) keyCol = KEYBOARD_WIDTH;
		selected1 = selected2 = 0;				//	Reset selected to zero
	}

	if ( currentKeys.offEvent.select ) {
		SetNextUpdate( DRAW_UPDATE_MAJOR );
		uint8_t len = strlen( nameBuf );
		if ( len < sizeof(nameBuf) - 1 ) {
			switch ( c )
			{
				case '\x7F':
					if ( len ) nameBuf[len-1] = NULL;							//	Delete character
					break;

				default:
					sprintf( nameBuf, "%s%c", nameBuf, c );						//	Append character
			}
		}
	}


	//	Process keys
	if ( currentKeys.offEvent.up2 ) {
		upper = upper?false:true;
		SetNextUpdate( DRAW_UPDATE_MAJOR );
	}

	if ( currentKeys.offEvent.back )
		MenuPop();

}

//TODO add parameters etc.
void MenuLoadKeypad( void )
{
	MenuPush( MenuKeypad );
	SetNextUpdateOverride(DRAW_UPDATE_MAJOR);		//	Drop update back to major, instead of full
}


//########################################################################################################################
//	Popup menu handlers
//########################################################################################################################
typedef struct
{
//	const char *	header;
	char *			header;
	const char *	message;
	const char *	btnString;
	char *			btnLabels[MENU_MAX_BUTTONS];
	uint8_t			numButtons;
	int8_t			result;
	bool			firstRun;
}	popupParams_t;

popupParams_t	popupParams;


uint8_t	ParseButtons( char * dest, const char * btnString )
{
	popupParams.numButtons = 0;
	strcpy( dest, btnString );
	char * strPtr = strtok(dest, MENU_BTN_DELIM);
	while( strPtr != NULL ) {
		popupParams.btnLabels[popupParams.numButtons++] = strPtr;
		strPtr = strtok(NULL, MENU_BTN_DELIM);
	}
	return popupParams.numButtons;
}

int8_t	Menu_ShowPopup( void )
{
	uint8_t i;
	gButton_t *btn;
	gButton_sel_t sel;

	if ( popupParams.firstRun ) {
		SetCurrentUpdate( DRAW_UPDATE_MAJOR );									//	Major update on first run
		popupParams.firstRun = false;											//	Clear first run flag
        _GrayOutBackground();
	}

	selected2 = Limit( selected2, 0, popupParams.numButtons-1 );				//	Limit button selection
//	if ( currentKeys.onState.select )
//		SetCurrentUpdate( DRAW_UPDATE_MINOR );									//	At least a minor update

	if ( lcdCurrentUpdate >= DRAW_UPDATE_MAJOR ) {
		gPopup_Draw( &popupFrame, popupParams.header );
		if ( popupParams.message )
			gFrame_DrawText( &popupFrame.inner, popupParams.message, strBuf, ALIGN_CENTER, 0, 0 );
	}

	if ( lcdCurrentUpdate >= DRAW_UPDATE_MINOR ) {
		ParseButtons( strBuf, popupParams.btnString );
		for ( i=0; i<popupParams.numButtons; i++ ) {
			btn = &menuButtons[i];
			if ( i == selected2 ) {
				sel = ( currentKeys.onState.select )? GBTN_SEL_ACTIVATE: GBTN_SEL_HIGHLIGHT;
			} else
				sel = GBTN_SEL_NONE;
			gButton_Draw( btn, popupParams.btnLabels[i], sel );
		}
	}

	if ( currentKeys.offEvent.select ) {
		popupParams.result = selected2;
		SetNextUpdate( DRAW_UPDATE_FULL );										//	Request full screen update
		return selected2;
	}
	return -1;
}

void	Menu_Popup()
{
	//TODO add way to handle result in menu system
	if ( Menu_ShowPopup() >= 0 )
		MenuPop();
}


void	Menu_CreatePopup( char * hdr, const char * msg, const char * btnString, uint8_t width, uint8_t height, bool loadNow )
{
	uint8_t x,y,w,h,i;
	gButton_t *btn;

	selected2 = 0;

	popupParams.header = hdr;
	popupParams.message = msg;
	popupParams.btnString = btnString;
	popupParams.result = -1;
	popupParams.firstRun = true;

	gPopup_Create( &popupFrame, width, height );
	gPopup_SetMargins( &popupFrame, 1, 9, 1, 13 );			//	Margins for header and buttons

	popupParams.numButtons = ParseButtons( strBuf, popupParams.btnString );

	h = 11;
	w = popupFrame.outer.dim.x / popupParams.numButtons;
	x = popupFrame.outer.box.p1.x+1;
	y = popupFrame.outer.box.p2.y-h;

	for ( i=0; i<popupParams.numButtons; i++ ) {
		btn = &menuButtons[i];
		gButton_Create( btn, x, y, w-2, h );
		x += w;
	}

	if ( loadNow ) {
		MenuPush( Menu_Popup );
		SetNextUpdateOverride(DRAW_UPDATE_MAJOR);		//	Drop update back to major, instead of full
	}
}


//########################################################################################################################
//	8/16 bit slider display menu
//########################################################################################################################

typedef enum
{
	VAL_TYPE_I8		= 0,
	VAL_TYPE_I16,
} val_type_t;

typedef struct
{
	val_type_t	type;
	void		*ptr;

	const char	*hdr;
	const char	*suffix;

	int16_t		min;
	int16_t		max;
	int16_t		inc;
	uint8_t		fp;
}	sliderInfo_t;

sliderInfo_t	sliderInfo;


void	MenuSlider( void )
{
	int16_t val = 0;
	switch ( sliderInfo.type ) {
		case VAL_TYPE_I8:
			val = *((int8_t*)sliderInfo.ptr);
			break;
		case VAL_TYPE_I16:
			val = *((int16_t*)sliderInfo.ptr);
			break;
	}
	val = Limit( val + (selected2*sliderInfo.inc), sliderInfo.min, sliderInfo.max );
	selected2 = 0;
	switch ( sliderInfo.type ) {
		case VAL_TYPE_I8:
			*((int8_t*)sliderInfo.ptr) = val;
			break;
		case VAL_TYPE_I16:
			*((int16_t*)sliderInfo.ptr) = val;
			break;
	}

	if ( lcdCurrentUpdate >= DRAW_UPDATE_MAJOR ) {
		gPopup_Draw( &popupFrame, PSTR(sliderInfo.hdr) );
	}
	if ( lcdCurrentUpdate >= DRAW_UPDATE_MINOR ) {
		gFrame_Clear( &popupFrame.inner );
		gProgressBar_Draw( &progressBar, val );
		int count = sprintf( strBuf, "%d", val );
        if ( sliderInfo.fp ) {
			int i;
			for ( i=count; i>count-sliderInfo.fp; i-- )
				strBuf[i] = strBuf[i-1];
			strBuf[i] = '.';
			count++;
		}
		sprintf( strBuf+count, "%s", PSTR(sliderInfo.suffix) );
		GLCD_SetMode( DRAW_XOR );
		gFrame_DrawText( &popupFrame.inner, NULL, strBuf, ALIGN_CENTER, 0, 1 );
		GLCD_SetMode( DRAW_ON );
	}

	EVENT_BACK_KEY { MenuPop(); }
	EVENT_SELECT_KEY { MenuPop(); }
}

void	_LoadSlider( const char * str, const char * suffix, void *p_val, int16_t min, int16_t max, int16_t inc, uint8_t fp, val_type_t type )
{
	selected2 = 0;				//	Reset selected to zero

	sliderInfo.hdr = str;
	sliderInfo.suffix = suffix;
	sliderInfo.ptr = p_val;
	sliderInfo.type = type;
	sliderInfo.min = min;
	sliderInfo.max = max;
	sliderInfo.inc = inc;
	sliderInfo.fp = fp;

	_GrayOutBackground();

	gPopup_Create( &popupFrame, 104, 21 );
	gPopup_SetMargins( &popupFrame, 2, 10, 2, 2 );
	gProgressBar_Create( &progressBar, popupFrame.inner.box.p1.x, popupFrame.inner.box.p1.y, popupFrame.inner.box.p2.x, popupFrame.inner.box.p2.y );
	gProgressBar_SetRange( &progressBar, min, max );

	MenuPush( MenuSlider );
	SetNextUpdateOverride(DRAW_UPDATE_MAJOR);		//	Drop update back to major, instead of full
}

void	MenuLoadSliderI16( const char * str, const char * suffix, int16_t *p_val, int16_t min, int16_t max, int16_t inc )
{
	_LoadSlider( str, suffix, (void*)p_val, min, max, inc, 0, VAL_TYPE_I16 );
}

void	MenuLoadSliderI16fp( const char * str, const char * suffix, int16_t *p_val, int16_t min, int16_t max, int16_t inc, uint8_t fp )
{
	_LoadSlider( str, suffix, (void*)p_val, min, max, inc, fp, VAL_TYPE_I16 );
}

void	MenuLoadSliderI8( const char * str, const char * suffix, int8_t *p_val, int8_t min, int8_t max, int8_t inc )
{
	_LoadSlider( str, suffix, (void*)p_val, min, max, inc, 0, VAL_TYPE_I8 );
}

void	MenuLoadSliderI8fp( const char * str, const char * suffix, int8_t *p_val, int8_t min, int8_t max, int8_t inc, uint8_t fp )
{
	_LoadSlider( str, suffix, (void*)p_val, min, max, inc, fp, VAL_TYPE_I8 );
}


//########################################################################################################################
//	On/off byte menu manipulation
//########################################################################################################################
void	MenuToggleOnOff( uint8_t *val )
{
	if ( *val ) *val = 0;
	else *val = 1;
}

char *	GetStrOnOff( char * str, uint8_t *val )
{
	sprintf( strBuf, "%-*s%s", menuTemp.numCols-3, str, *val?"ON":"OFF" );
//	sprintf( strBuf, "%-*s%s", menuTemp.numCols-4, str, *val?"ON":"OFF" );
	return strBuf;
}




