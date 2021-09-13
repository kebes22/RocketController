#ifndef MOTOR_DRIVER_H__
#define MOTOR_DRIVER_H__

#include <stdint.h>
//#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void	motor_init( void );
void	motor_run( int16_t m1, int16_t m2 );

#ifdef __cplusplus
}
#endif

#endif // MOTOR_DRIVER_H__