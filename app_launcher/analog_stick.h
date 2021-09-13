#ifndef ANALOG_STICK_H__
#define ANALOG_STICK_H__

#include <stdint.h>
#include <stdbool.h>

// Stores the points for a 3-point calibration of an analog input
typedef struct
{
	int16_t		min;		/*  */
	int16_t		mid;		/*  */	
	int16_t		max;		/*  */
} analog_cal_point_t;

typedef struct
{
	analog_cal_point_t		in;		/* Input range */
	analog_cal_point_t		out;	/* Output range */
} analog_cal_t;


typedef struct
{
	int16_t			value;	/* The current scaled/calibrated value */

// Intermediate/internal values
	int16_t			raw;			/* The current raw value */
	analog_cal_t	cal;			/* Calibration values */
	bool			isCalibrating;	/* Flag if currently calibrating */
} analog_in_t;



void		analog_in_init( analog_in_t * p_an );
int16_t		analog_in_update( analog_in_t * p_an, int16_t val );

//void		analog_in_start_cal( analog_in_t * p_an, int16_t in );
void		analog_in_start_cal( analog_in_t * p_an );
void		analog_in_end_cal( analog_in_t * p_an );
void		analog_in_set_input_range( analog_in_t * p_an, int16_t min, int16_t mid, int16_t max );
void		analog_in_set_output_range( analog_in_t * p_an, int16_t min, int16_t mid, int16_t max );


#endif // ANALOG_STICK_H__