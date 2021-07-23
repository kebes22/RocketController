#ifndef SUICIDE_H__
#define SUICIDE_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void suicide( void );
void suicide_init( uint32_t pinNum, bool onState );

#ifdef __cplusplus
}
#endif

#endif // SUICIDE_H__