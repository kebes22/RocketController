#ifndef MENU_SYSTEM_H__
#define MENU_SYSTEM_H__

#include "stdint.h"
#include "stdbool.h"
#include "Debounce.h"
#include "Graphics.h"

#define MENU_KEY_DEBOUNCE_TIME		3
//#define MENU_KEY_REPEAT_FIRST_DELAY	50
//#define MENU_KEY_REPEAT_DELAY		10
#define MENU_KEY_REPEAT_FIRST_DELAY	35
#define MENU_KEY_REPEAT_DELAY		8
#define MENU_MAX_DEPTH				10
#define MENU_MAX_BUTTONS			5

#define MENU_BTN_DELIM				"/"

//########################################################################################################################
//	Keypad structures
//########################################################################################################################

typedef union
{
	struct
	{
		unsigned	plus	:1;
		unsigned	minus	:1;
		unsigned	up		:1;
		unsigned	down	:1;
		unsigned	left	:1;
		unsigned	right	:1;
		unsigned	select	:1;
		unsigned	back	:1;

//		unsigned	up1		:1;
//		unsigned	key3	:1;
//		unsigned	down1	:1;
//		unsigned	stop	:1;
		unsigned	up2		:1;	//FIXME
//		unsigned	down2	:1;
//		unsigned	keyC	:1;
//		unsigned	keyD	:1;
//		unsigned	keyE	:1;
//		unsigned	keyF	:1;
	};
	uint32_t		all;
}	Menu_KeyBits_t;

typedef struct
{
	Menu_KeyBits_t		onState;
	Menu_KeyBits_t		onEvent;
	Menu_KeyBits_t		offEvent;
	Menu_KeyBits_t		repeatEvent;
	Menu_KeyBits_t		anyOnEvent;	// Either on event or repeat event
	Debounce_t			keyDebounce;
}	Menu_Keys_t;

extern Menu_Keys_t currentKeys;



//########################################################################################################################
//	"Private" helper functions and structures used by the macros below
//########################################################################################################################
enum
{
	DRAW_UPDATE_NONE	= 0x00,
//	DRAW_UPDATE_MINOR	= BIT1,
//	DRAW_UPDATE_MAJOR	= BIT2,
//	DRAW_UPDATE_FULL	= BIT3,

    DRAW_UPDATE_MINOR	= 0x01,
	DRAW_UPDATE_MAJOR	= 0x02,
	DRAW_UPDATE_FULL	= 0x04,
};


void	MenuStartProcess( char * title );
bool	MenuItemProcess( char * name, bool selectable );
void	MenuEndProcess( void );

bool	MenuDrawLine( uint8_t thickness );



typedef void	(*MENU_FUNC)( void );			//	Function pointer to menu

typedef enum
{
	MENU_STATE_NORMAL,			//	Normal running
	MENU_STATE_LOAD,			//	First time menu loaded (on push)
	MENU_STATE_RETURN,			//	Return from another menu (on pop)
	MENU_STATE_BACK,			//	About to leave menu (on pop)
}	MenuState_t;


/* This structure tracks the persistent values and state of a menu
 * A new instance will be created and used each time a menu is created
 * The user does not need to access anything in this structure directly
 */
typedef struct
{
	MenuState_t	state;
	MENU_FUNC	menu;
	//	Items that need to be saved for return
	uint8_t		firstVisible;
	uint8_t		numItems;
	int8_t		selected1;
	int8_t		selected2;
}	MenuStackFrame_t;


/* This structure contains all of the temporary parameters used in menu construction
 * Only a single instance is needed to service all menus
 * The user does not need to access anything in this structure directly
 */
typedef struct
{
	uint8_t			firstVisible;	// First visible item if scrolled down
	uint8_t			lastVisible;	// Last visible item if more exist
	int16_t			pixelRow;		// Current pixel row on the screen

	uint8_t			numItems;		// Number of selectable menu items (determined at end of menu loop)
	uint8_t			currentItem;	// Current processing item. Starts at first visible
//	uint8_t			drawItem;		// Current draw loop item. This item will be drawn if visible
	uint8_t			itemIndex;		// Index of the currently processing selectable item in the list. Incremented at the end of item processing


//	uint8_t			numRows;
	uint8_t			numCols;
	gAlignment_e	alignment;
}	MenuParams_t;


extern MenuStackFrame_t		*currentMenu;				//	Pointer to current menu stack frame
extern MenuParams_t			menuTemp;					//	Temporary menu tracking variables

//########################################################################################################################
//	Menu control macros
//########################################################################################################################
/* These are helper macros for easily creating menus
 * Use START_MENU at the beginning and END_MENU at the end, and insert
 * as many menu items as needed using MENU_ITEM
 * Multiple menu items may be created dynamically with a loop, or conditionally
 * displayed with an if statement
 */
//#define MENU_START(title) do { \
//	MenuStartProcess( title ); \
//	for ( menuTemp.drawItem = 0; menuTemp.drawItem < menuTemp.numRows; menuTemp.drawItem++, menuTemp.currentItem++ ) { \
//		menuTemp.itemIndex = 0;
//#define MENU_START(title) do { \
//	MenuStartProcess( title ); \
//	for ( menuTemp.currentItem = menuTemp.firstVisible; menuTemp.currentItem <= menuTemp.lastVisible+1; menuTemp.currentItem++ ) { \
//		menuTemp.itemIndex = 0;
#define MENU_START(title) do { \
	MenuStartProcess( title ); \
	if ( lcdCurrentUpdate )	   \
	for ( menuTemp.currentItem = menuTemp.firstVisible; menuTemp.currentItem <= menuTemp.lastVisible+1; menuTemp.currentItem++ ) { \
		menuTemp.itemIndex = 0;

#define MENU_ITEM( name, func ) do { if ( MenuItemProcess( name, true ) ) { func; } } while(0)
#define MENU_ITEM_TEXT( name ) do { MenuItemProcess( name, false ); } while(0)
//#define MENU_ITEM( name, func ) do { if ( lcdCurrentUpdate ) { if ( MenuItemProcess( name ) ) { func; } } } while(0)
#define MENU_END() } MenuEndProcess(); } while (0);

#define MENU_ALIGNMENT(align)	menuTemp.alignment = align;

//TODO restructure loop above to be more efficient (i.e. skip unused lines)

//	Event handler for first load of a menu function (after processing MenuPush())
//	Call at the beginning of the menu function
#define EVENT_LOAD_MENU			for ( ; currentMenu->state == MENU_STATE_LOAD; currentMenu->state = MENU_STATE_NORMAL )
//	Event handler for first return to a menu function (after processing MenuPop())
//	Call at the beginning of the menu function
#define EVENT_RETURN_TO_MENU	for ( ; currentMenu->state == MENU_STATE_RETURN; currentMenu->state = MENU_STATE_NORMAL )
//	Event handler before exiting menu function (after processing MenuPop())
//	Call at the end of the menu function
#define EVENT_EXIT_MENU			for ( ; currentMenu->state == MENU_STATE_BACK; currentMenu->state = MENU_STATE_NORMAL )

#define MENU_LINE(t)			MenuDrawLine(t);
#define MENU_ITEM_BACK()		MENU_ITEM( PSTR(hdrBack), MenuPop() )
#define EVENT_BACK_KEY			if ( currentKeys.offEvent.back )
#define EVENT_SELECT_KEY		if ( currentKeys.offEvent.select )

#define EVENT_UPDATE_INTERVAL(_int)	static uint32_t cnt=0; if (cnt==0) cnt=_int; if (--cnt == 0)


// Helper for accessing strings from flash memory
// Not needed on all architechtures
#if 0
char *	PSTR( const char * pStr )
{
	strncpy( pStrBuf, pStr, sizeof(pStrBuf) );	//	Copy to RAM (up to max size)
	return pStrBuf;
}
#else
#define PSTR(x) (char *)x
#endif


extern uint8_t	lcdCurrentUpdate;
extern uint8_t	lcdNextUpdate;

// Or the new level with the old level, just in case it is already set higher
#define SetNextUpdate(val)				lcdNextUpdate |= (val)
#define SetNextUpdateOverride(val)		lcdNextUpdate = (val)
#define SetCurrentUpdate(val)			lcdCurrentUpdate |= (val)
#define SetCurrentUpdateOverride(val)	lcdCurrentUpdate = (val)

void	Menu_Null_Action( void );//FIXME

//########################################################################################################################
//	Graphics objects
//########################################################################################################################
extern gPopup_t			menuFrame;
extern gPopup_t			popupFrame;
extern gButton_t		menuButtons[MENU_MAX_BUTTONS];
extern gProgressBar_t	progressBar;

//########################################################################################################################
//	Public API functions
//########################################################################################################################

// MainMenu must be provided by user, and is the main entry point for menu execution.
void	MainMenu( void );
// Must be called on a regular interval to process menus
bool	MenuUpdate( Menu_KeyBits_t keyBits );


void	MenuPush( MENU_FUNC nextMenuPtr );
void	MenuPop( void );
void	MenuReset( void );

void	MenuLoadSliderI16	( const char * str, const char * suffix, int16_t *p_val, int16_t min, int16_t max, int16_t inc );
void	MenuLoadSliderI16fp	( const char * str, const char * suffix, int16_t *p_val, int16_t min, int16_t max, int16_t inc, uint8_t fp );
void	MenuLoadSliderI8	( const char * str, const char * suffix, int8_t *p_val, int8_t min, int8_t max, int8_t inc );
void	MenuLoadSliderI8fp	( const char * str, const char * suffix, int8_t *p_val, int8_t min, int8_t max, int8_t inc, uint8_t fp );

void		MenuToggleOnOff( uint8_t *val );
void		MenuKeypad( void );
void		MenuLoadKeypad( void );
char *		GetStrOnOff( char * str, uint8_t *val );

void		Menu_CreatePopup( char * hdr, const char * msg, const char * btnString, uint8_t width, uint8_t height, bool loadNow );
int8_t		Menu_ShowPopup( void );

#endif
