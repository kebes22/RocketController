#ifndef DEBOUNCE_H
#define	DEBOUNCE_H

#include "stdint.h"

typedef struct
{
	uint32_t		val;
	uint32_t		last;
	uint32_t		dCount;
	uint32_t		dTarget;
}	Debounce_t;


void		DebounceInit( Debounce_t *debounce, uint32_t count );
uint32_t	DebounceProcess( Debounce_t *debounce, uint32_t val );

#endif	/* DEBOUNCE_H */

