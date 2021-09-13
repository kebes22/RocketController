#include "analog_stick.h"
#include "Utils.h"

static int16_t _get_calibrated( analog_cal_t * p_cal, int16_t val )
{
	if ( val >= p_cal->in.mid ) {	// Upper Half
		MapLimit16( val, p_cal->in.mid, p_cal->in.max, p_cal->out.mid, p_cal->out.max );
	} else {						// Lower Half
		MapLimit16( val, p_cal->in.min, p_cal->in.mid, p_cal->out.min, p_cal->out.mid );
	}
}

// Starts calibration
//void analog_in_start_cal( analog_in_t * p_an, int16_t in )
void analog_in_start_cal( analog_in_t * p_an )
{
	//p_an->cal.in.min = p_an->cal.in.mid = p_an->cal.in.max = in;
	p_an->cal.in.min = p_an->cal.in.mid = p_an->cal.in.max = p_an->raw;
	p_an->isCalibrating = true;
}

static inline void _process_cal( analog_in_t * p_an, int16_t val )
{
	if ( val < p_an->cal.in.min )
		p_an->cal.in.min = val;
	if ( val > p_an->cal.in.max )
		p_an->cal.in.max = val;

	// TODO add center cal function
	p_an->cal.in.mid = (p_an->cal.in.min + p_an->cal.in.max) / 2;
}

void analog_in_end_cal( analog_in_t * p_an )
{
	p_an->isCalibrating = false;
}

void analog_in_set_input_range( analog_in_t * p_an, int16_t min, int16_t mid, int16_t max )
{
	p_an->cal.in.min = min;
	p_an->cal.in.mid = mid;
	p_an->cal.in.max = max;	
}

void analog_in_set_output_range( analog_in_t * p_an, int16_t min, int16_t mid, int16_t max )
{
	p_an->cal.out.min = min;
	p_an->cal.out.mid = mid;
	p_an->cal.out.max = max;	
}

int16_t analog_in_update( analog_in_t * p_an, int16_t val )
{
	p_an->raw = val;
	if ( p_an->isCalibrating )
		_process_cal( p_an, val );
	p_an->value = _get_calibrated( &p_an->cal, val );
	return p_an->value;
}

void analog_in_init( analog_in_t * p_an )
{
	//analog_in_set_input_range( p_an, 0, 8192, 16383 );
 //   analog_in_set_output_range( p_an, -1000, 0, 1000 );
	analog_in_set_input_range( p_an, 0, 8192, 16383 );
    analog_in_set_output_range( p_an, -1000, 0, 1000 );
}

