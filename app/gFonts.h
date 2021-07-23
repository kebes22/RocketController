#ifndef GFONTS_H
#define	GFONTS_H

#include "stdint.h"

typedef struct {
	uint16_t	size;			//	Total size of table (if variable width)
	uint8_t	width;			//	Width of characters in table
	uint8_t	height;			//	Height of characters in table
	uint8_t	first;			//	First character number (offset)
	uint8_t	count;			//	Total characters
//	uint8_t	table;			//	Zero for no table, same as count if table exists

	uint8_t	data[];			//	Font table, starting with width table (if exists)
} font_t;

typedef struct {
	uint8_t	index;			//	Index offset of character
	uint8_t	width;			//	Actual width of character
	uint8_t	height;			//	Height of character
	uint8_t	rows;			//	number of page rows spanned
	uint8_t	*data;			//	Pointer to actual character data
}	fontChar_t;


//	Fonts
//extern const byte monoFont5x7[];
extern const uint8_t sysFont5x7[];
extern const uint8_t Corsiva_12[];
extern const uint8_t Arial_14[];
extern const uint8_t Arial_bold_14[];


enum
{
	CHAR_AR_UP			= 1,
	CHAR_AR_DOWN,
	CHAR_AR_LEFT,
	CHAR_AR_RIGHT,

	CHAR_DOT_OPEN		= 20,
	CHAR_DOT_CLOSED,
	CHAR_CHAIN,
	CHAR_CHECK,

	CHAR_SPINNER_0,
	CHAR_SPINNER_1,
	CHAR_SPINNER_2,
	CHAR_SPINNER_3,
	CHAR_SPINNER_4,
	CHAR_SPINNER_5,
	CHAR_SPINNER_6,
	CHAR_SPINNER_7,
};


#endif	/* GFONTS_H */

