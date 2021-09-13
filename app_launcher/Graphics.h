/* 
 * File:   Graphics.h
 * Author: Kevin
 *
 * Created on October 17, 2014, 10:47 PM
 */

#ifndef GRAPHICS_H
#define	GRAPHICS_H

//#include "Globals.h"
#include "GLCD.h"
#include "stdbool.h"



//########################################################################################################################
//	Basic Types
//########################################################################################################################
typedef struct
{
	uint8_t		x;
	uint8_t		y;
}	gPoint_t;

typedef struct
{
	gPoint_t	p1;			//	Top left corner
	gPoint_t	p2;			//	Bottom right corner
}	gRect_t;

typedef struct
{
	uint8_t		width;					//	Width (in pixels) of border
	uint8_t		clear;					//	Clear area (in pixels) beyond border
}	gBorderStyle_t;


//########################################################################################################################
//	Intermediate Types
//########################################################################################################################
typedef struct
{
	gRect_t			box;					//	Outer box
	gPoint_t		mid;					//	Midpoint of box
	gPoint_t		dim;					//	Dimensions (width, height)
	gBorderStyle_t	border;					//	Border style
}	gFrame_t;


//########################################################################################################################
//	Advanced Types
//########################################################################################################################
typedef struct
{
	gFrame_t		outer;					//	Outer box outline
	gFrame_t		inner;					//	Inner usable area box (if header or button area has been added)
}	gPopup_t;

//typedef struct
//{
//	uint8_t	x;
//	uint8_t	y;
//	uint8_t	width;
//	uint8_t	height;
//}	gScrollBar_t;

typedef struct
{
	gFrame_t		frame;					//	Button frame
//	uint8_t			selState;				//	Select state
}	gButton_t;

typedef struct
{
	gFrame_t		frame;					//	Button frame
	int16_t			min;					//	Min value (nothing filled in)
	int16_t			max;					//	Max value (everything filled in)
}	gProgressBar_t;


typedef enum
{
	GBTN_SEL_NONE,
	GBTN_SEL_HIGHLIGHT,
	GBTN_SEL_ACTIVATE,
}	gButton_sel_t;
//########################################################################################################################
//	Function Prototypes
//########################################################################################################################
void	gFrame_Create( gFrame_t *frame, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2 );
void	gFrame_CreateCentered( gFrame_t *frame, uint8_t width, uint8_t height );
void	gFrame_SetBorder( gFrame_t *frame, uint8_t border, uint8_t clear );

void	gFrame_Clear( gFrame_t *frame );
void	gFrame_Fill( gFrame_t *frame, drawMode_t mode );
void	gFrame_DrawBorder( gFrame_t *frame );
void	gFrame_Draw( gFrame_t *frame );

//void	gFrame_DrawHeader( gFrame_t *frame, char *title );
void	gFrame_DrawText( gFrame_t *frame, const char *pText, char *strBuf, gAlignment_e align, int8_t xOff, int8_t yOff );
void	gFrame_DrawScrollBar( gFrame_t *frame, int8_t selected, uint8_t total, bool drawFull );

void	gPopup_Create( gPopup_t *pop, uint8_t width, uint8_t height );
void	gPopup_SetMargins( gPopup_t *pop, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2 );
void	gPopup_Draw( gPopup_t *pop, char *title );
void	gPopup_DrawHeader( gPopup_t *pop, char *title );

void	gButton_Create( gButton_t *btn, uint8_t x, uint8_t y, uint8_t width, uint8_t height );
void	gButton_Draw( gButton_t *button, const char *label, gButton_sel_t sel );

void	gProgressBar_Create( gProgressBar_t *bar, uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2 );
void	gProgressBar_SetRange( gProgressBar_t *bar, int16_t min, int16_t max );
void	gProgressBar_Draw( gProgressBar_t *bar, int16_t val );



#endif	/* GRAPHICS_H */

