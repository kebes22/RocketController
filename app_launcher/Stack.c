#include "Stack.h"

//	Initializes a stack with no items, and a given size
void	StackInit( Stack_t *stk, int8_t size )
{
	stk->top = -1;
	stk->size = size;
}

//	Re-Initializes a stack with no items, and keeps existing size
void	StackClear( Stack_t *stk )
{
	StackInit( stk, stk->size );
}

//	Pushes an item onto the queue, and returns the index, or -1 if full
int8_t	StackPush( Stack_t *stk )
{
	int8_t top = stk->top+1;
	if ( top < stk->size ) {
		stk->top++;
		return top;
	} else
		return -1;
}

//	Pops an item off of the stack, and returns the index, or -1 if empty
//	Once popped, the item may be overwritten at any time, so use StackPeek
//	below if the contents need to persist for a while, then use StackPop once done
int8_t	StackPop( Stack_t *stk )
{
	int8_t top = stk->top-1;
	if ( top >= 0 ) {
		stk->top--;
		return top;
	} else
		return -1;
}

//	Peeks at the top item in the stack, and returns the index, or -1 if empty
//	Use this for actively used items, and then call StackPop once done
int8_t	StackPeek( Stack_t *stk )
{
	return stk->top;
}

