/* 
 * File:   Utils.h
 * Author: krees
 *
 */

#ifndef UTILS_H
#define	UTILS_H

//#include "Globals.h"
#include "stdint.h"
#include "stdbool.h"

//########################################################################################################################
//	L-Point filter
//########################################################################################################################
typedef struct
{
	int32_t		sum;
	int16_t		average;
	uint8_t		bitWeight;
	uint8_t		rounding;
}	L_Filter16_t;

typedef struct
{
	int32_t		sum;
	int32_t		average;
	uint8_t		bitWeight;
	uint8_t		rounding;
}	L_Filter32_t;


//########################################################################################################################
//	Window filter
//########################################################################################################################
typedef struct
{
	int16_t		value;
	int16_t		min;
	int16_t		max;
	int16_t		step;
}	WindowFilter_t;


//########################################################################################################################
//	Rate limiting filter
//########################################################################################################################
typedef struct
{
	int16_t	rise;					//	Maximum value increase per run increment
	int16_t	run;					//	Number of time increments to divide rise over

	int16_t	err;					//	Error accumulator for ramp approximation
	int16_t	minInc;					//	Minimum (unscaled) increment to be added to output each cycle
	int16_t	remainder;				//	Remainder (scaled) to be added to the error accumulator each cycle
}	RateLimit_t;

//########################################################################################################################
//	Remapping function structure
//########################################################################################################################
typedef struct
{
	int32_t	multiplier;
	int32_t	rounding;
	uint8_t	shiftBits;
	int16_t	inRange;
	int16_t	outRange;
	uint8_t	inBits;
	uint8_t	outBits;
	bool	negative;

	int16_t	inMin;
	int16_t	outMin;
}	MapParams_t;

//########################################################################################################################
//	CRC
//########################################################################################################################
typedef struct
{
	union
	{
		struct
		{
			uint8_t sum1;
			uint8_t sum2;
		};
		uint16_t result;
	};
}	crcFletcher16_t;

//########################################################################################################################
//	Function Prototypes
//########################################################################################################################
bool	Equals				( uint8_t *buf1, uint8_t* buf2, uint8_t len );

int16_t	L_Filter16_Init		( L_Filter16_t * filter, uint8_t bitWeight, int16_t val );
int16_t	L_Filter16			( L_Filter16_t * filter, int16_t val );
int16_t	L_Filter16_Offset	( L_Filter16_t * filter, int16_t val );
int16_t	L_Filter16_Set		( L_Filter16_t * filter, int16_t val );
int16_t	L_Filter16_Decimate	( L_Filter16_t * filter, uint8_t bitWeight );

int32_t	L_Filter32_Init		( L_Filter32_t *filter, uint8_t bitWeight, int32_t val );
int32_t	L_Filter32			( L_Filter32_t *filter, int32_t val );
int32_t	L_Filter32_Offset	( L_Filter32_t *filter, int32_t val );
int32_t	L_Filter32_Set		( L_Filter32_t *filter, int32_t val );

void	WindowFilterInit	( WindowFilter_t *window, int16_t min, int16_t max, int16_t step );
bool	WindowFilter		( WindowFilter_t *window, int16_t diff );

void	RateLimitInit		( RateLimit_t * ramp, int16_t rise, int16_t run );
int16_t	RateLimitIncrement	( RateLimit_t * ramp, int16_t current, int16_t target );

int16_t	Abs					( int16_t val );
//int32_t	Abs32				( int32_t val );

//int16_t	Max					( int16_t val1, int16_t val2 );
//int16_t	Min					( int16_t val1, int16_t val2 );
int16_t	Sign				( int16_t val );
int16_t	Limit				( int32_t val, int16_t min, int16_t max );
int32_t	Limit32				( int32_t val, int32_t min, int32_t max );
int16_t	Map16				( int16_t value, int16_t fromLow, int16_t fromHigh, int16_t toLow, int16_t toHigh );
int16_t	Map16R				( int16_t value, int16_t fromLow, int16_t fromHigh, int16_t toLow, int16_t toHigh );
int32_t	Map32				( int32_t value, int32_t fromLow, int32_t fromHigh, int32_t toLow, int32_t toHigh );
int16_t	MapLimit16			( int16_t value, int16_t fromLow, int16_t fromHigh, int16_t toLow, int16_t toHigh );

uint8_t	GetNumBits			( uint32_t val );

void	Copy				( uint8_t *src, uint8_t *dest, uint16_t len );
uint16_t	CopyCKSM			( uint8_t *src, uint8_t *dest, uint16_t len );
void	Fill				(uint8_t *dest, uint8_t len, uint8_t val );

void	crcFletcher16_Init	( crcFletcher16_t *crc );
uint16_t	crcFletcher16		( crcFletcher16_t *crc, uint8_t *data, uint8_t count );
uint8_t	Checksum8			( uint8_t *data, uint16_t len );

//########################################################################################################################
//	Helper Macros
//########################################################################################################################
#define STRa(a) #a
#define STR(a) STRa(a)

#define STR2a(a,b) a ## b
#define STR2(a,b) STR2a(a,b)

#define STR3a(a,b,c) a ## b ## c
#define STR3(a,b,c) STR3a(a,b,c)

#define Max(val1,val2)	((val1>val2)?val1:val2)
#define Min(val1,val2)	((val1<val2)?val1:val2)

#define Swap8(a,b)		do { uint8_t temp=a; a=b; b=temp; } while(0)
#define Swap16(a,b)		do { uint16_t temp=a; a=b; b=temp; } while(0)
#define Swap32(a,b)		do { uint32_t temp=a; a=b; b=temp; } while(0)

#define SingleBit(val)	!(val&(val-1))											//	Determines wheter a variable has only a single bit set

#define LOWER_BYTE(v)		(*(((unsigned char *) (&v) )))
#define UPPER_BYTE(v)		(*(((unsigned char *) (&v) + 1)))
#define LOWER_WORD(v)		(*(((unsigned short *) (&v) )))
#define UPPER_WORD(v)		(*(((unsigned short *) (&v) + 1)))

#define PIN_FRAME_LOW(pin) for ( pin=LOW; pin==LOW; pin=HIGH )					//	Asserts a pin low for the duration of the enclosing brackets
#define PIN_FRAME_HIGH(pin) for ( pin=HIGH; pin==HIGH; pin=LOW )				//	Asserts a pin high for the duration of the enclosing brackets

////	Disables interrupts for the duration of the enclosing brackets, then restores to previous state
//#define CRITICAL_SECTION	bool _gState_; for ( _gState_=INTCONbits.GIE, INTCONbits.GIE=OFF; _gState_!=INTCONbits.GIE; INTCONbits.GIE=_gState_ )

////	Allows concatenation of pre-expanded #define's
//#define TOKENPASTE1(x, y) x ## y
//#define TOKENPASTE(x, y) TOKENPASTE1(x, y)



/* Sequence loop macro.
 * Creates a loop where each SEQ_ITEM gets executed once, preceeded or followed
 * by some common code.
 * Usage:
 *	SEQ_LOOP {
 *		Common pre-code goes here
 *		SEQ_ITEM {
 *			Do first thing here
 *		}
 *		SEQ_ITEM {
 *			Do next thing here
 *		}
 *		Common post-code goes here
 *	}
 */
//#define	SEQ_LOOP	for ( int i1_=0, int i2_=0; i1_<4; i1_++ )
//#define SEQ_ITEM	if ( i1_==i2_++ )
#define	SEQ_LOOP	for ( int _i1_=0, _i2_=0, _i3_=1; _i1_<_i3_; _i3_=_i2_, _i1_++, _i2_=0 )
#define SEQ_ITEM	if ( _i1_==_i2_++ )

#define	SEQ_LOOP_STATIC	for ( static int _i1_=0, _i2_=0, _i3_=1; _i1_<_i3_; _i3_=_i2_, _i1_++, _i2_=0 )

//	Easily allows running of some code once only, and then never again. Code should follow contained in braces {}
#define RUN_ONCE static bool _firstRun_ = TRUE; for (;_firstRun_;_firstRun_=FALSE)

#endif	/* UTILS_H */

