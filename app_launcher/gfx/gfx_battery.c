#include "gfx_battery.h"
#include "GLCD.h"
#include "Utils.h"

void	gfx_battery_create( gfx_battery_t *p_batt, int16_t x, int16_t y, int16_t width, int16_t height, bool invert )
{
	gfx_battery_t batt = {
		.outline = { {x,y}, {GFX_DIM_TO_P2(x,width),GFX_DIM_TO_P2(y,height)} },
		.dim = { width, height },
		.cap = { (width/16), (height/4) },
		.border = (height >= 16? 2: 1),
		.invert = invert,
	};
	if ( batt.cap.w < 1 ) batt.cap.w = 1;
	if ( batt.cap.h < 1 ) batt.cap.h = 1;
	*p_batt = batt;
}


#define X1		p_batt->outline.p1.x
#define X2		p_batt->outline.p2.x
#define Y1		p_batt->outline.p1.y
#define Y2		p_batt->outline.p2.y
#define WIDTH	p_batt->dim.w
#define HEIGHT	p_batt->dim.h
#define BORDER	p_batt->border

void	gfx_battery_draw( gfx_battery_t *p_batt, gfx_update_type_t update, int8_t percent, bool show_percent, bool warn )
{
	uint8_t strBuf[5];

	// Draw full battery if major update
	if ( update >= GFX_UPDATE_MAJOR ) {
		drawMode_t mode1 = (p_batt->invert)? DRAW_OFF: DRAW_ON;
		GLCD_DrawFill( X1, Y1, X2-p_batt->cap.w, Y2, mode1 );
		GLCD_DrawFill( X2-p_batt->cap.w, Y1+p_batt->cap.h, X2, Y2-p_batt->cap.h, mode1 );
	}

	if ( update >= GFX_UPDATE_MINOR ) {
		// Draw battery level
		drawMode_t mode2 = (p_batt->invert ^ warn)? DRAW_ON: DRAW_OFF;
		drawMode_t mode3 = (mode2==DRAW_ON)? DRAW_OFF: DRAW_ON;
		int16_t w = X2-X1-p_batt->cap.w-(2*BORDER)-1;
		int16_t val = Limit( Map16R( percent, 0, 100, 1, w), 1, w );
		X1 += BORDER; X2 -= BORDER+p_batt->cap.w;
		Y1 += BORDER; Y2 -= BORDER;
		GLCD_DrawFill( X1, Y1, X2, Y2, mode2 );
		GLCD_DrawFill( X1+1, Y1+1, X1+val, Y2-1, mode3 );
		
		// Draw percentage if big enough
		if ( show_percent ) {
			font_t * newFont = NULL;
			uint8_t yOffset = 0;	// Compensates for font gutters
			SEQ_LOOP {
				SEQ_ITEM { newFont = (font_t*)Arial_bold_14; yOffset = 1; }
				SEQ_ITEM { newFont = (font_t*)sysFont5x7; yOffset = 0; }
				SEQ_ITEM { newFont = NULL; }
				uint8_t minMargin = 2+(BORDER*2);
				if ( HEIGHT >= (newFont->height + minMargin) && WIDTH >= ((newFont->width*4) + minMargin + p_batt->cap.h) )
					break;
			}
			if ( newFont != NULL ) {
				font_t * oldFont = GLCD_SetFont( newFont );
				sprintf( strBuf, "%d%%", percent );
				GLCD_SetMode( DRAW_XOR );
				GLCD_Goto( ((X1+X2+1)>>1), ((Y1+Y2+1)>>1) - (newFont->height>>1) + yOffset );
				GLCD_PutStringAligned( strBuf, ALIGN_CENTER );
				GLCD_SetFont( oldFont );
			}
		}
		GLCD_SetMode( DRAW_ON );
	}
}
