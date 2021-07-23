#include "Debounce.h"

void	DebounceInit( Debounce_t *debounce, uint32_t count )
{
	debounce->val = 0;
	debounce->last = 0;
	debounce->dCount = 0;
	debounce->dTarget = count;
}

uint32_t	DebounceProcess( Debounce_t *debounce, uint32_t val )
{
	Debounce_t d = *debounce;						//	Copy local
	if ( val == d.last ) {							//	Same as last
		if ( val != d.val ) {						//	Make sure not original value
			if ( ++d.dCount >= d.dTarget ) {		//	Check if same for long enough
				d.val = val;						//	Store new val
				d.dCount = 0;						//	Reset debounce counter
			}
		} else {									//	It is the same as original
			d.dCount = 0;							//	Reset debounce counter
		}
	} else {										//	Different than last
		d.dCount = 0;								//	Reset debounce counter
	}

	d.last = val;									//	Store last
	*debounce = d;									//	Copy back
	return d.val;									//	Return value
}

