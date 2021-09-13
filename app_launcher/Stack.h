/* 
 * File:   Stack.h
 * Author: Kevin
 *
 * Created on December 27, 2014, 9:39 PM
 */

#ifndef STACK_H
#define	STACK_H

#include "stdint.h"

typedef volatile struct
{
	int8_t				top;		//	In index of the top item on the stack
	int8_t				size;		//	Maximum number of items in stack
}	Stack_t;


void	StackInit	( Stack_t *stk, int8_t size );
void	StackClear	( Stack_t *stk );
int8_t	StackPush	( Stack_t *stk );
int8_t	StackPop	( Stack_t *stk );
int8_t	StackPeek	( Stack_t *stk );

#endif	/* STACK_H */

