/* 
 * File:   GLCD.h
 * Author: Kevin
 *
 * Created on August 15, 2014, 10:36 PM
 */

#ifndef GLCD_H
#define	GLCD_H

//#include "Globals.h"
#include <stdint.h>
#include <stdbool.h>

#include "gFonts.h"

#include "menu_images.h"	//TODO tImage should be moved to own header


#define	GLCD_WIDTH		128
#define	GLCD_HEIGHT		64
#define	GLCD_ROWS		(GLCD_HEIGHT/8)


//typedef enum
//{
//	DRAW_NORMAL,
//	DRAW_INVERSE,
//	DRAW_OR,
//	DRAW_XOR,
//}	drawMode_t;

typedef enum
{
	DRAW_OFF = 0,
	DRAW_ON,
	DRAW_XOR,
}	drawMode_t;


typedef enum
{
	ALIGN_LEFT			= -1,
	ALIGN_CENTER		= 0,
	ALIGN_RIGHT			= 1,
}	gAlignment_e;

typedef struct
{
	int8_t	min;
	int8_t	max;
}	rowData_t;

typedef volatile struct
{
	font_t		*font;
	drawMode_t	drawMode;					//	Draw mode (normal, invert, etc.)

	//	Pixel position (allows off screen values)
	int16_t		xPos;						//	Current column
	int16_t		yPos;						//	Current pixel row
	int16_t		polyX;
	int16_t		polyY;
}	glcdBuf_t;


extern glcdBuf_t	lcd;


void		GLCD_Init( void );
void		GLCD_ClearAll( void );

font_t *	GLCD_SetFont( font_t *font );
drawMode_t	GLCD_SetMode( drawMode_t mode );
void		GLCD_Goto( int16_t x, int16_t y );
void		GLCD_PutChar( char c );
void		GLCD_PutString( const char * str );
void		GLCD_PutStringAligned( const char * str, gAlignment_e align );

uint8_t		GLCD_CalcCharWidth( char c );
uint16_t	GLCD_CalcStrWidth( const char * str );


//	Drawing functions
void		GLCD_PlotPixel( int16_t x, int16_t y );
void		GLCD_PlotLine( int16_t x1, int16_t y1, int16_t x2, int16_t y2 );
void		GLCD_PlotRect( int16_t x1, int16_t y1, int16_t x2, int16_t y2 );
void		GLCD_PlotRectThick( int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t thickness );
void		GLCD_DrawFill( int16_t x, int16_t y, int16_t x2, int16_t y2, drawMode_t mode );

void		GLCD_StartPoly( int16_t x, int16_t y );
void		GLCD_PlotPoly( int16_t x, int16_t y );

//void		GLCD_DrawBitmap( const uint8_t *bmp, int16_t x, int16_t y );
void		GLCD_DrawBitmap( const uint8_t *bmp, int16_t x, int16_t y, bool invert );
//void		GLCD_DrawBitmap_xy( const uint8_t *bmp, int16_t x, int16_t y );

//void	GLCD_Draw_tImage( const tImage *p_img, int16_t x, int16_t y, bool invert );
void	GLCD_Draw_tImage( const tImage *p_img, int16_t x, int16_t y, drawMode_t mode );

void		GLCD_Update( void );



#endif	/* GLCD_H */

