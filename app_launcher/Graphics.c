#include "Graphics.h"
#include "Utils.h"
//#include "StringUtils.h"
#include <string.h>

//TODO add ability to do partial updates on all graphics objects

#define STR_BUF_LEN		33					//	Size of temporary string buffer
static char strBuf[STR_BUF_LEN];				//	Buffer for temporary string assembly

//########################################################################################################################
//	Utilities
//########################################################################################################################
int8_t	StrReadLine( char *dest, const char * src )
{
	char c;
	uint8_t len = 0;
	while ( (c=*src++) != NULL && c != '\n' ) {		//	Read from source, make sure not null or newline
		*dest++ = c;				//	Copy to destination
		len++;
	}
	*dest++ = NULL;					//	Null terminate
	return len;						//	Return length
}

//########################################################################################################################
//	Default values for graphics objects
//########################################################################################################################
// TODO make these const?
gBorderStyle_t	gFrame_DefaultBorder = { 0, 0 };

gBorderStyle_t	gPopup_DefaultOuterBorder = { 1, 1 };
gBorderStyle_t	gPopup_DefaultInnerBorder = { 0, 0 };
uint8_t			gPopup_DefaultMargins[] = { 1, 1, 1, 1 };

gBorderStyle_t	gButton_DefaultBorder = { 1, 0 };
gBorderStyle_t	gProgressBar_DefaultBorder = { 1, 0 };


//########################################################################################################################
//	Generic graphics frame
//########################################################################################################################
void	gFrame_Create( gFrame_t *frame, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2 )
{
	gFrame_t frm;
	frm.box.p1.x = x1;
	frm.box.p1.y = y1;
	frm.box.p2.x = x2;
	frm.box.p2.y = y2;
	frm.dim.x = x2-x1+1;
	frm.dim.y = y2-y1+1;
	frm.mid.x = frm.box.p1.x+(frm.dim.x>>1);
	frm.mid.y = frm.box.p1.y+(frm.dim.y>>1);
	frm.border = gFrame_DefaultBorder;
	*frame = frm;
}

void	gFrame_CreateCentered( gFrame_t *frame, uint8_t width, uint8_t height )
{
	uint8_t xm = GLCD_WIDTH>>1;
	uint8_t ym = GLCD_HEIGHT>>1;
	uint8_t x1 = xm-(width>>1);
	uint8_t y1 = ym-(height>>1);
	gFrame_Create( frame, x1, y1, x1+width-1, y1+height-1 );
}

void	gFrame_SetBorder( gFrame_t *frame, uint8_t border, uint8_t clear )
{
	frame->border.width = border;
	frame->border.clear = clear;
}

//	Clears the inner rectangle of the frame (excluding border)
void	gFrame_Fill( gFrame_t *frame, drawMode_t mode )
{
	gRect_t box = frame->box;
	GLCD_DrawFill( box.p1.x, box.p1.y, box.p2.x, box.p2.y, mode );
}

//	Clears the inner rectangle of the frame (excluding border)
void	gFrame_Clear( gFrame_t *frame )
{
	gFrame_Fill( frame, DRAW_OFF );
}

//	Draws border only
void	gFrame_DrawBorder( gFrame_t *frame )
{
	gRect_t box = frame->box;
	int8_t b1 = frame->border.width;
	int8_t i = b1+frame->border.clear;
	GLCD_SetMode( DRAW_OFF );												//	Start with clear area
	while ( i ) {																//	Check for lines to draw
		if ( i==b1 ) GLCD_SetMode( DRAW_ON );								//	Switch to solid border
		GLCD_PlotRect( box.p1.x-i, box.p1.y-i, box.p2.x+i, box.p2.y+i );		//	Draw rectangle
		i--;																	//	Decrement count
	}
	GLCD_SetMode( DRAW_ON );												//	Go back to normal mode
}

//	Draws an already created frame, including border if exists
void	gFrame_Draw( gFrame_t *frame )
{
	gFrame_Clear( frame );
	gFrame_DrawBorder( frame );
}


void	gFrame_DrawText( gFrame_t *frame, const char *pText, char *strBuf, gAlignment_e align, int8_t xOff, int8_t yOff )
{
	if ( !strBuf ) return;

	uint8_t x, y;
	const char *strPtr = pText;
	int8_t len;
	char last;

	y = frame->box.p1.y + yOff;
	switch ( align )
	{
		case ALIGN_LEFT:	x = frame->box.p1.x + xOff;		break;
		case ALIGN_CENTER:	x = frame->mid.x + xOff;		break;
		case ALIGN_RIGHT:	x = frame->box.p2.x + xOff;		break;
	}
	do {
		if ( pText ) {									//	Check for multi-line constant
			len = StrReadLine( strBuf, strPtr );
		} else {
			len = strlen( strBuf );
		}
		GLCD_Goto( x, y );
		GLCD_PutStringAligned( strBuf, align );
		y += 8;	//TODO should be font height
		if ( pText ) {
			strPtr += len+1;
			last = strPtr[-1];
		} else {
			strBuf += len+1;
			last = strBuf[-1];
		}
	} while ( len && last );
}

//########################################################################################################################
//	Popup frame
//########################################################################################################################
void	gPopup_Create( gPopup_t *pop, uint8_t width, uint8_t height )
{
	gFrame_CreateCentered( &pop->outer, width, height );
	gPopup_SetMargins( pop, gPopup_DefaultMargins[0], gPopup_DefaultMargins[1], gPopup_DefaultMargins[2], gPopup_DefaultMargins[3]  );
	pop->outer.border = gPopup_DefaultOuterBorder;
	pop->inner.border = gPopup_DefaultInnerBorder;
}

void	gPopup_SetMargins( gPopup_t *pop, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2 )
{
	gFrame_t frm = pop->outer;
	gBorderStyle_t brd = pop->inner.border;
	gFrame_Create( &pop->inner, frm.box.p1.x+x1, frm.box.p1.y+y1, frm.box.p2.x-x2, frm.box.p2.y-y2 );
	pop->inner.border = brd;
}

void	gPopup_Draw( gPopup_t *pop, char *title )
{
	gFrame_Draw( &pop->outer );
	gFrame_DrawBorder( &pop->inner );
	if ( title ) gPopup_DrawHeader( pop, title );
}

//	Draws a header in the specified frame, with the specified text
//	Text must be in RAM
void	gPopup_DrawHeader( gPopup_t *pop, char *title )
{
	GLCD_Goto( pop->outer.mid.x, pop->outer.box.p1.y );
	GLCD_PutStringAligned( title, ALIGN_CENTER );
//	GLCD_DrawFill( pop->outer.box.p1.x, pop->outer.box.p1.y, pop->outer.box.p2.x, pop->inner.box.p1.y-1, DRAW_XOR );
	GLCD_DrawFill( pop->outer.box.p1.x, pop->outer.box.p1.y, pop->outer.box.p2.x, pop->outer.box.p1.y+7, DRAW_XOR );
}

//########################################################################################################################
//	Scroll Bar
//########################################################################################################################
void	gFrame_DrawScrollBar( gFrame_t *frame, int8_t selected, uint8_t total, bool drawFull )
{
	uint8_t x1, x2, y1, y2, val;

	x2 = frame->box.p2.x;
	x1 = x2-4;
	y1 = frame->box.p1.y;
	y2 = frame->box.p2.y;

	if ( drawFull ) {
		GLCD_SetMode( DRAW_ON );
		GLCD_DrawFill( x1, y1, x2, y2, DRAW_OFF );			//	Clear rectangle
		GLCD_PlotRect( x1, y1, x2, y2 );						//	Draw outline
		GLCD_DrawFill( x1, y1, x2, y1+3, DRAW_ON );			//	Top arrow background
		GLCD_DrawFill( x1, y2-3, x2, y2, DRAW_ON );			//	Bottom arrow background
		GLCD_SetMode( DRAW_OFF );
		GLCD_PlotLine( x1+2, y1+1, x2-2, y1+1 );				//	Top arrow
		GLCD_PlotLine( x1+1, y1+2, x2-1, y1+2 );				//	Top arrow
		GLCD_PlotLine( x1+2, y2-1, x2-2, y2-1 );				//	Bottom arrow
		GLCD_PlotLine( x1+1, y2-2, x2-1, y2-2 );				//	Bottom arrow
		GLCD_SetMode( DRAW_ON );
	} else {
		GLCD_DrawFill( x1+1, y1+4, x2-1, y2-4, DRAW_OFF );	//	Clear inner area
	}

	if ( selected < 0 ) selected = 0;
	//	Draw scroll mark
	int8_t range = y2-y1-13;
	val = (uint16_t)selected*range/(total-1);
	GLCD_DrawFill( x1+1, y1+5+val, x2-1, y1+8+val, DRAW_ON );
}

//########################################################################################################################
//	Buttons
//########################################################################################################################
void	gButton_Create( gButton_t *btn, uint8_t x, uint8_t y, uint8_t width, uint8_t height )
{
	gFrame_Create( &btn->frame, x+1, y+1, x+width-2, y+height-2 );
	btn->frame.border = gButton_DefaultBorder;
}

void	gButton_Draw( gButton_t *button, const char *label, gButton_sel_t sel )
{
	gButton_t btn = *button;
	gFrame_Draw( &btn.frame );
	if ( label ) {
		GLCD_Goto( btn.frame.mid.x, btn.frame.mid.y-3 );
		GLCD_PutStringAligned( label, ALIGN_CENTER );
	}
	if ( sel == GBTN_SEL_HIGHLIGHT )
		GLCD_PlotRect( btn.frame.box.p1.x, btn.frame.box.p1.y, btn.frame.box.p2.x, btn.frame.box.p2.y );
	else if ( sel == GBTN_SEL_ACTIVATE )
		GLCD_DrawFill( btn.frame.box.p1.x, btn.frame.box.p1.y, btn.frame.box.p2.x, btn.frame.box.p2.y, DRAW_XOR );
}

//########################################################################################################################
//	Progress bar
//########################################################################################################################
void	gProgressBar_Create( gProgressBar_t *bar, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2 )
{
	gFrame_Create( &bar->frame, x1, y1, x2, y2 );
	bar->frame.border = gProgressBar_DefaultBorder;
	gProgressBar_SetRange( bar, 0, x2-x1+1 );
}

void	gProgressBar_SetRange( gProgressBar_t *bar, int16_t min, int16_t max )
{
	bar->min = min;
	bar->max = max;
}

void	gProgressBar_Draw( gProgressBar_t *bar, int16_t val )
{
	uint8_t rawVal = Map16( val, bar->min, bar->max, 0, bar->frame.dim.x );
	gFrame_Draw( &bar->frame );
	if ( rawVal ) {
		gRect_t rect = bar->frame.box;
		GLCD_DrawFill( rect.p1.x, rect.p1.y, rect.p1.x+rawVal-1, rect.p2.y, DRAW_ON );
	}
}

