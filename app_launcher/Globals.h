#if 0
/*
 * File:   Globals.h
 * Author: krees
 *
 * Created on March 20, 2013, 2:50 PM
 */


#ifndef GLOBALS_H
#define	GLOBALS_H

#include "stdbool.h"
#include "stdint.h"

//#include <stddef.h>
//#include <GenericTypeDefs.h>


//#define		TRUE			1
//#define		FALSE			0

#define		HIGH			1
#define		LOW				0
#define		SET				1
#define		CLEAR			0
#define		INPUT			1
#define		OUTPUT			0

//typedef enum { OFF, ON } state_t;
//typedef unsigned char mutex_t;	//	!!! should be used as volatile
////typedef unsigned short mutex_t;	//	!!! should be used as volatile
//enum { IDLE=0, BUSY=1 };	//	!!! should be used as volatile

////typedef enum { FALSE, TRUE } bool;
//enum { FALSE, TRUE };
//typedef unsigned char 	bool;

//typedef unsigned short 	bool;
//typedef BOOL 	bool;
//#define bool			BOOL;


//#define 	NULL		0x00
//#define 	NULL		(0)



#define		RUN				1
#define		RESET_HOLD		0

#define		LESS_THAN		-1
#define		EQUAL			0
#define		GREATER_THAN	1

#define		BIT0		(0x0001)
#define		BIT1		(0x0002)
#define		BIT2		(0x0004)
#define		BIT3		(0x0008)
#define		BIT4		(0x0010)
#define		BIT5		(0x0020)
#define		BIT6		(0x0040)
#define		BIT7		(0x0080)
#define		BIT8		(0x0100)
#define		BIT9		(0x0200)
#define		BITA		(0x0400)
#define		BITB		(0x0800)
#define		BITC		(0x1000)
#define		BITD		(0x2000)
#define		BITE		(0x4000)
#define		BITF		(0x8000)

typedef unsigned char 	byte;

typedef unsigned char 	uchar;

typedef signed char 	int8;
typedef unsigned char 	uint8;

typedef signed short 	int16;
typedef unsigned short 	uint16;

//typedef signed short long	int24;	//	Not properly supported in signed math
//typedef unsigned short long	uint24;	//	Not properly supported in signed math

typedef signed long		int32;
typedef unsigned long	uint32;


#define LOWER_BYTE(v)	(*(((unsigned char *) (&v) )))
#define UPPER_BYTE(v)	(*(((unsigned char *) (&v) + 1)))
#define LOWER_WORD(v)	(*(((unsigned short *) (&v) )))
#define UPPER_WORD(v)	(*(((unsigned short *) (&v) + 1)))


#define JUMP_FUNC(func) asm("GOTO _" ___mkstr(func) )		//	!!! Move to utils or somewhere else? !!! Use with caution, compiler won't properly track tree of called function

//	Pin name passed in must also have *_TRIS defined as well
#define	PIN_IN(PIN)			PIN##_TRIS=INPUT
#define	PIN_IN_PU(PIN)		PIN##_TRIS=INPUT; PIN##_PUE=TRUE; nRBPU = 0;
#define	PIN_OUT(PIN,VAL)	PIN=VAL; PIN##_TRIS=OUTPUT


#define CS_FRAME(pin) for ( pin=LOW; pin==LOW; pin=HIGH )


//#define		_ISR_	__attribute__((__interrupt__,auto_psv))
#define		_ISR_


//#define		Reset()		asm("reset")


#endif	/* GLOBALS_H */

#endif