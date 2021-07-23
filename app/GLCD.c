#include "GLCD.h"
#include "Utils.h"

#include "sdk_common.h"
#include "nrf_gfx.h"

#define USE_NRF_GFX	0

glcdBuf_t	lcd;

extern const nrf_lcd_t nrf_lcd_st7565;
static const nrf_lcd_t * p_lcd = &nrf_lcd_st7565;


void	GLCD_Update( void )
{
	p_lcd->lcd_display();
}

void	GLCD_ClearAll( void )
{
	nrf_gfx_screen_fill( p_lcd, 0 );
}

void	GLCD_DrawFill( int16_t x1, int16_t y1, int16_t x2, int16_t y2, drawMode_t mode )
{
#if 0
	drawMode_t oldMode = GLCD_SetMode(mode);
	for ( int16_t x = x1; x <= x2; x++ ) {
		for ( int16_t y = y1; y <= y2; y++ ) {
			GLCD_PlotPixel(x,y);
		}
	}
    GLCD_SetMode(oldMode);
#else
	 p_lcd->lcd_rect_draw( x1, y1, x2-x1+1, y2-y1+1, mode );
#endif
}


//################################################################################
//	Text/Font related functions
//################################################################################
font_t *	GLCD_SetFont( font_t *font )
{
	font_t * old = lcd.font;
	lcd.font = font;
	return old;
}

drawMode_t	GLCD_SetMode( drawMode_t mode )
{
	drawMode_t old = lcd.drawMode;
	lcd.drawMode = mode;
	return old;
}

void	GLCD_Goto( int16_t x, int16_t y )
{
	lcd.xPos = x;
	lcd.yPos = y;
}

fontChar_t	FontGetChar( char c )
{
	fontChar_t chr;
	uint8_t first = lcd.font->first;
	if ( c < first || c > first+lcd.font->count ) c = '_';						//	!!! Replace invalid character with space
	chr.index = c - lcd.font->first;								//	Get index of character
	chr.height = lcd.font->height;									//	Get height
	chr.rows = (chr.height + 7) >> 3;
	if ( lcd.font->size )											//	Variable size
	{
		uint8_t *tPtr = &lcd.font->data[0];						//	Table pointer
		chr.data = &lcd.font->data[lcd.font->count];			//	buffer pointer
		uint8_t i = chr.index;									//	Load index as counter
		uint16_t offset = 0;
		while ( i-- )
			offset += *tPtr++;
		chr.width = *tPtr++;									//	Next value is size of current character
		chr.data += offset * chr.rows;
	}
	else														//	Fixed size
	{
		chr.width = lcd.font->width;
		chr.data = &lcd.font->data[chr.index*lcd.font->width];	//	Get offset within font table, and skip over width table
	}
	return chr;
}


void	GLCD_PutChar( char c )
{
	fontChar_t chr = FontGetChar( c );
	int16_t xTemp = lcd.xPos;
	int16_t yStart = lcd.yPos;
	uint8_t r = chr.rows;
	uint8_t shift = 0;	//TODO why is this needed? bug in font generator?
	while ( r-- ) {
		xTemp = lcd.xPos;
		uint8_t w = chr.width;
		while ( w--) {
			uint8_t data = (*chr.data++) >> shift;
			uint8_t bits = chr.height;
			if ( bits > 8 ) bits = 8;
			int16_t yTemp = yStart;
			while ( bits-- ) {
				if ( data & 1 )
					GLCD_PlotPixel( xTemp, yTemp );
				data >>= 1;
				yTemp++;
			}
			xTemp++;
		}
		chr.height -= 8;
		if ( chr.height<8 ) shift = 8-chr.height;
		yStart += 8;
	}
	GLCD_Goto( xTemp + 1, lcd.yPos );
}

void	GLCD_PutString( const char * str )
{
	char ch;
	do {
		ch = *str++;
		if ( ch ) GLCD_PutChar( ch );
	} while ( ch );
}

void	GLCD_PutStringAligned( const char * str, gAlignment_e align )
{
	int16_t width = GLCD_CalcStrWidth( str );
	int16_t xOff;
	switch ( align )
	{
		case ALIGN_LEFT:	xOff = 0;			break;
		case ALIGN_CENTER:	xOff = -(width>>1);	break;
		case ALIGN_RIGHT:	xOff = (width>>1);		break;
	}
	GLCD_Goto( lcd.xPos + xOff, lcd.yPos );
	GLCD_PutString( str );
}

uint8_t	GLCD_CalcCharWidth( char c )
{
	if ( lcd.font->size )											//	Check for variable width
	{
		uint8_t idx = c - lcd.font->first;							//	Get index of character
		return lcd.font->data[idx];									//	Get width from table
	}
	return lcd.font->width;											//	Else fixed width
}

uint16_t	GLCD_CalcStrWidth( const char * str )
{
	int16_t width = 0;
	char ch;
	while ( (ch = *str++) != 0 )
		width += GLCD_CalcCharWidth( ch ) + 1;						//	+1 is for character gap
	return width-(width?1:0);										//	Remove character gap from last character
}



//################################################################################
//	Graphics/shape related functions
//################################################################################

void	GLCD_PlotPixel( int16_t x, int16_t y )
{
//	uint32_t color = ((uint32_t)lcd.drawMode << 24) + 1;
	uint32_t color = lcd.drawMode;
#if USE_NRF_GFX
	nrf_gfx_point_t point = NRF_GFX_POINT(x,y);
	nrf_gfx_point_draw( p_lcd, &point, color );
#else
	p_lcd->lcd_pixel_draw(x, y, color);
#endif
}

void	GLCD_PlotLine( int16_t x1, int16_t y1, int16_t x2, int16_t y2 )
{
	int16_t dx, dy, d, d2, i, i2;
	bool swap = false;
	if ( (dx=x2-x1)<0 ) dx=-dx;							//	Get x change
	if ( (dy=y2-y1)<0 ) dy=-dy;							//	Get y change
	if ( dx==0 ) {										//	Get x diff, and check for vertical line
		if ( y1>y2 ) Swap16( y1, y2 );					//	Compensate if swapped direction
		do {
			GLCD_PlotPixel( x1, y1++ );					//	Plot vertical
		} while ( dy-- );
	} else if ( dy==0 ) {								//	Get y diff, and check for horizontal line
		if ( x1>x2 ) Swap16( x1, x2 );					//	Compensate if swapped direction
		do {
			GLCD_PlotPixel( x1++, y1 );					//	Plot horizontal
		} while ( dx-- );
	} else if ( dx>dy ) {								//	Check for mostly horizontal
		swap=(x1>x2);									//	Check for swaped order
		d2=dy<<1;										//	2*dy (for rounding later)
		i=(dx>>1)+1;									//	Start at middle (for symmetry and speed)
		while ( i-- ) {									//	Scan horizontal from middle
			d = ((i*d2/dx)+1)>>1;						//	Calculate distance from endpoint (and round)
			i2=(swap)?-i:i;								//	Compensate if swapped direction
			GLCD_PlotPixel( x1+i2, y1+d );				//	Plot one half
			GLCD_PlotPixel( x2-i2, y2-d );				//	Plot other half !!! only if not same as last position
		}
	} else {											//	Otherwise, mostly vertical
		swap=(y1>y2);									//	Check for swaped order
		d2=dx<<1;										//	2*dx (for rounding later)
		i=(dy>>1)+1;									//	Start at middle (for symmetry and speed)
		while ( i-- ) {									//	Scan vertical from middle
			d = ((i*d2/dy)+1)>>1;						//	Calculate distance from endpoint (and round)
			i2=(swap)?-i:i;								//	Compensate if swapped direction
			GLCD_PlotPixel( x1+d, y1+i2 );				//	Plot one half
			GLCD_PlotPixel( x2-d, y2-i2 );				//	Plot other half !!! only if not same as last position
		}
	}
}

void	GLCD_PlotRect( int16_t x1, int16_t y1, int16_t x2, int16_t y2 )
{
	GLCD_PlotLine( x1, y1, x2, y1 );
	GLCD_PlotLine( x2, y1, x2, y2 );
	GLCD_PlotLine( x2, y2, x1, y2 );
	GLCD_PlotLine( x1, y2, x1, y1 );
}

void	GLCD_PlotRectThick( int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t thickness )
{
	thickness -= 1;
	GLCD_PlotRect( x1, y1, x2, y2 );
	if ( thickness > 0 )
		GLCD_PlotRect( x1+thickness, y1+thickness, x2-thickness, y2-thickness );
}

void	GLCD_StartPoly( int16_t x, int16_t y )
{
	lcd.polyX = x;
	lcd.polyY = y;
}

void	GLCD_PlotPoly( int16_t x, int16_t y )
{
	GLCD_PlotLine(lcd.polyX,lcd.polyY,x,y);
	lcd.polyX = x;
	lcd.polyY = y;
}

void	GLCD_DrawBitmap( const uint8_t *bmp, int16_t x, int16_t y, bool invert )
{
	uint16_t width = *((uint16_t*)bmp); bmp+=2;
	uint16_t height = *((uint16_t*)bmp); bmp+=2;

	uint8_t row, col;
	for ( row=y; row<y+height; row += 8 ) {
		for ( col = x; col<x+width; col++ ) {
			uint16_t rowTemp = row;
			uint8_t data = *bmp++;
			uint8_t bits = y+height - row;
			if ( bits > 8 ) bits = 8;
			while ( bits-- ) {
				bool on = (data & 1);
				if ( invert ) on = !on;
//				if ( on )
					p_lcd->lcd_pixel_draw(col, rowTemp, on);
				rowTemp++;
				data >>= 1;
			}
		}
	}
}


//void	GLCD_Draw_tImage( const tImage *p_img, int16_t x, int16_t y, bool invert )
void	GLCD_Draw_tImage( const tImage *p_img, int16_t x, int16_t y, drawMode_t mode )
{
	uint16_t width = p_img->width;
	uint16_t height = p_img->height;
	const uint8_t * p_data = p_img->data;

	uint8_t row, col;
	for ( row=y; row<y+height; row++ ) {
		uint16_t colCount = width;
		for ( col = x; col<x+width; ) {
			uint8_t data = *p_data++;
			uint8_t bits = 8;
			if ( bits > colCount ) bits = colCount;
			while ( bits-- ) {
				bool on = (data & 0x80);
//				if ( invert ) on = !on;
				if ( on )
					p_lcd->lcd_pixel_draw(col, row, mode);
				col++;
				colCount--;
				data <<= 1;
			}
		}
	}
}



//################################################################################
//	Initialization
//################################################################################

void	GLCD_Init( void )
{
	APP_ERROR_CHECK( nrf_gfx_init( p_lcd ) );
	nrf_gfx_rotation_set( p_lcd, NRF_LCD_ROTATE_180 );
//	nrf_gfx_rotation_set( p_lcd, NRF_LCD_ROTATE_90 );
//	nrf_gfx_rotation_set( p_lcd, NRF_LCD_ROTATE_0 );

	GLCD_Goto( 0, 0 );
	GLCD_SetMode( DRAW_ON );

	GLCD_SetFont( (font_t*)sysFont5x7 );
}

