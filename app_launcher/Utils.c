#include "Utils.h"

//########################################################################################################################
//	Copy from one buffer to another
//########################################################################################################################
void	Copy ( uint8_t *src, uint8_t *dest, uint16_t len )
{
	while( len-- )
		*dest++ = *src++;
}

//########################################################################################################################
//	Copy from one buffer to another and get checksum
//########################################################################################################################
uint16_t	CopyCKSM ( uint8_t *src, uint8_t *dest, uint16_t len )
{
	uint16_t	cksm = 0;
	while( len-- ) {
		uint8_t data = *src++;
		*dest = data;
		cksm += data;
	}
	return cksm;
}

//########################################################################################################################
//	Fill a buffer with a value
//########################################################################################################################
void	Fill (uint8_t *dest, uint8_t len, uint8_t val )
{
	while( len-- )
		*dest++ = val;
}

//########################################################################################################################
//	Compare buffers to see if values are equal
//########################################################################################################################
bool	Equals ( uint8_t *buf1, uint8_t* buf2, uint8_t len )
{
	while ( len-- )
		if ( *buf1++ != *buf2++)
			return false;
	return true;
}

//########################################################################################################################
//	L-Point filter
//########################################################################################################################

//	Initializes L-point filter with specified bit weight, and initial value
int16_t	L_Filter16_Init( L_Filter16_t *filter, uint8_t bitWeight, int16_t val )
{
	filter->bitWeight = bitWeight;
	filter->sum = (int32_t)val << bitWeight;
//	filter->rounding = (1 << bitWeight) >> 1;							//	2^bitWeight / 2
	filter->rounding = 0;
	return filter->average = val;
}

//	Processes L-point filter with specified value. Result is stored in average
int16_t	L_Filter16( L_Filter16_t *filter, int16_t val )
{
	filter->sum = filter->sum - filter->average + val;
	return filter->average = (filter->sum + filter->rounding) >> filter->bitWeight;
}

int16_t	L_Filter16_Offset( L_Filter16_t *filter, int16_t val )
{
	filter->sum += (int32_t)val << filter->bitWeight;						//	Add weighted value
	return filter->average = (filter->sum + filter->rounding) >> filter->bitWeight;
}

int16_t	L_Filter16_Set( L_Filter16_t *filter, int16_t val )
{
	filter->sum = (int32_t)val << filter->bitWeight;
	return filter->average = val;
}

int16_t	L_Filter16_Decimate( L_Filter16_t * filter, uint8_t bitWeight )
{
	int8_t shift = filter->bitWeight - bitWeight;
	if ( shift < 0 ) shift = 0;
	return filter->sum >> shift;
}

//########################################################################################################################
//	24 bit L-Point filter
//########################################################################################################################
//	Initializes L-point filter with specified bit weight, and initial value
int32_t	L_Filter32_Init( L_Filter32_t *filter, uint8_t bitWeight, int32_t val )
{
	filter->bitWeight = bitWeight;
	filter->sum = (int32_t)val << bitWeight;
//	filter->rounding = (1 << bitWeight) >> 1;							//	2^bitWeight / 2
	filter->rounding = 0;
	return filter->average = val;
}

//	Processes L-point filter with specified value. Result is stored in average
int32_t	L_Filter32( L_Filter32_t *filter, int32_t val )
{
	filter->sum = filter->sum - filter->average + val;
	return filter->average = (filter->sum + filter->rounding) >> filter->bitWeight;
}

int32_t	L_Filter32_Offset( L_Filter32_t *filter, int32_t val )
{
	filter->sum += (int32_t)val << filter->bitWeight;						//	Add weighted value
	return filter->average = (filter->sum + filter->rounding) >> filter->bitWeight;
}

int32_t	L_Filter32_Set( L_Filter32_t *filter, int32_t val )
{
	filter->sum = (int32_t)val << filter->bitWeight;
	return filter->average = val;
}

//########################################################################################################################
//	Window Filter
//########################################################################################################################
void	WindowFilterInit( WindowFilter_t *window, int16_t min, int16_t max, int16_t step )
{
	window->value = min;
	window->min = min;
	window->max = max;
	window->step = step;
}

bool	WindowFilter( WindowFilter_t *window, int16_t diff )
{
	int16_t val = window->value;													//	Local copy for faster manipulation
	bool good = true;															//	Assume good initially
	int16_t absDiff = Abs(diff);
	if ( absDiff > val ) {
		good = false;															//	Current sample is bad
		if ( (val += window->step) > window->max )								//	Grow bubble a little for next cycle
			val = window->max;
	} else {
		if ( val - absDiff > window->step ) {									//	Check for enough margin below bubble to shrink
			if ( (val -= window->step) < window->min )							//	Shrink bubble a little for next cycle
				val = window->min;
		}
	}
	window->value = val;														//	Copy back to structure
	return good;
}

//########################################################################################################################
//	Rate limiting filter
//########################################################################################################################
void	RateLimitInit( RateLimit_t * ramp, int16_t rise, int16_t run )
{
	ramp->err = 0;																//	Clear out error accumulator
	ramp->rise = rise;
	ramp->run = run;
	ramp->minInc = rise / run;													//	Minimum integer increment per rate limited cycle
//	ramp->remainder = rise - ramp->minInc * rise;								//	Error remainder after integer adjustment
	ramp->remainder = rise - (ramp->minInc * run);								//	Error remainder after integer adjustment
//	NOP();
//	NOP();
}

int16_t	RateLimitIncrement( RateLimit_t * ramp, int16_t current, int16_t target )
{
	int16_t absDiff;																//	Absolute target difference
	int16_t err = ramp->err;														//	Get local copy of error (for speed)
	int16_t run = ramp->run;														//	Get local copy of run (for speed)
	int16_t rem = ramp->remainder;												//	Get local copy of remainder
	int16_t inc = ramp->minInc;													//	Start with minimum absolute increment	

	int16_t diff = target - current;												//	Get target difference
	bool positive = (diff > 0);													//	Flag if positive

	if ( positive ) {															//	Target moving up
		absDiff = diff;															//	Absolute is same as original
		err += rem;																//	Accumulate error (+)
		if ( err >= run ) {														//	Positive rollover
			inc += 1;															//	Adjust output
			err -= run;															//	Adjust error
		}
	} else {																	//	Target moving down
		absDiff = -diff;														//	Invert absolute 
		err -= rem;																//	Accumulate error (-)
		if ( err <= run ) {														//	Negative rollover
			inc += 1;															//	Adjust output
			err += run;															//	Adjust error
		}
	}	
	if ( absDiff <= inc ) {														//	Check if rate limit not required (at or close to target)
		err = 0;																//	Clear out error
		inc = absDiff;															//	limit increment to only what is needed
	}
	if ( !positive ) inc = -inc;												//	Restore sign
	ramp->err = err;															//	Save the new error accumulator
	
	return inc;																	//	Return value is desired increment
}

//########################################################################################################################
//	Math/limit
//########################################################################################################################

int16_t	Abs( int16_t val )
{
	return (val > 0)? val: -val;
}

//int32_t	Abs32( int32_t val )
//{
//	return (val > 0)? val: -val;
//}

//int16_t	Max( int16_t val1, int16_t val2 )
//{
//	return (val1 > val2)? val1: val2;
//}
//
//int16_t	Min( int16_t val1, int16_t val2 )
//{
//	return (val1 < val2)? val1: val2;
//}

//int16_t	Sign( int16_t val )
//{
//	return (val > 0)? val: -val;
//}


int16_t	Limit( int32_t val, int16_t min, int16_t max )
{
	if ( val > max ) val = max;
	else if ( val < min ) val = min;
	return val;
}

int32_t	Limit32( int32_t val, int32_t min, int32_t max )
{
	if ( val > max ) val = max;
	else if ( val < min ) val = min;
	return val;
}

int16_t	Map16( int16_t value, int16_t fromLow, int16_t fromHigh, int16_t toLow, int16_t toHigh )
{
	return ((int32_t)value - fromLow) * ((int32_t)toHigh - toLow) / (fromHigh - fromLow) + toLow;
}

int16_t	Map16R( int16_t value, int16_t fromLow, int16_t fromHigh, int16_t toLow, int16_t toHigh )
{
	int32_t numerator = ((int32_t)value - fromLow) * ((int32_t)toHigh - toLow);
	int16_t denominator = (fromHigh - fromLow);
	return (numerator + (denominator>>1)) / denominator + toLow;				//	!!! rounding factor may not work for negative numbers, need to test.  May need to be abs(denominator>>1)
}

int16_t	MapLimit16( int16_t value, int16_t fromLow, int16_t fromHigh, int16_t toLow, int16_t toHigh )
{
	uint16_t limLow = toLow;
	uint16_t limHigh = toHigh;
    if ( toLow > toHigh ) {
		limLow = toHigh;
		limHigh = toLow;
    }
	return Limit( Map16( value, fromLow, fromHigh, toLow, toHigh ), limLow, limHigh );
}

int32_t	Map32( int32_t value, int32_t fromLow, int32_t fromHigh, int32_t toLow, int32_t toHigh )
{
	return (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow;
}


//########################################################################################################################
//	Remap16
//	Performs an efficient remapping of one range to another.  The mapper structure must be initialized first with the
//	range to be converted, and scale and offsets will be stored, and bit shifts will be used for division.
//########################################################################################################################

void	Remap16Init_ScaleOffset( MapParams_t *mapper, int16_t inMin, int16_t inMax, double scale, double offset )
{
//	int16_t outMin, int16_t outMax
}

void	Remap16Init( MapParams_t *mapper, int16_t inMin, int16_t inMax, int16_t outMin, int16_t outMax )
{
	MapParams_t map;
	map.negative = false;														//	Positive until proven negative
	map.inMin = inMin;
	map.outMin = outMin;
	if ( inMin > inMax ) {														//	Check for reversed input range
		Swap16(inMin,inMax);													//	Swap order
		map.negative ^= 1;														//	Invert negation
	}
	if ( outMin > outMax ) {													//	Check for reversed input range
		Swap16(outMin,outMax);													//	Swap order
		map.negative ^= 1;														//	Invert negation (double negative will make a positive)
	}
	map.inRange = inMax - inMin;												//	Get input range as unsigned absolute range
	map.outRange = outMax - outMin;												//	Get output range as unsigned absolute range
	map.inBits = GetNumBits(map.inRange);										//	Get number if bits in input range
	map.outBits = GetNumBits(map.outRange);										//	Get number if bits in output range
	
//	map.shiftBits = 16;															//	Calculate usable bits for denominator
	map.shiftBits = 32-map.outBits-1;												//	Calculate usable bits for denominator
	map.rounding = ((int32_t)1<<(map.shiftBits-1));
//	map.multiplier = (((int32_t)1<<map.shiftBits)*map.outRange)/map.inRange;		//	Calculate scaling factor (rounded)
	map.multiplier = (((int32_t)1<<map.shiftBits)*map.outRange+(map.inRange>>1))/map.inRange;		//	Calculate scaling factor (rounded)
	if ( map.negative ) {
		map.multiplier = -map.multiplier;
//		map.rounding = -map.rounding;
	}
	*mapper = map;
}

int16_t	Remap16( MapParams_t *map, int16_t value )
{
	int32_t val32 = ((int32_t)value - map->inMin) * map->multiplier + map->rounding;//	Offset input and scale, plus rounding factor
	return (val32>>map->shiftBits) + map->outMin;								//	Divide down
}
//########################################################################################################################
//	Bit manipulation
//########################################################################################################################
//	Gets the number of bits used by a given positive value
uint8_t	GetNumBits( uint32_t val )
{
	uint8_t bits = 0;
	uint8_t val8 = 0;
	if ( UPPER_WORD(val) ) {													//	Check upper half, if non zero, then lower half is all used
		bits += 16;																//	Lower half is 16 bits
		val = UPPER_WORD(val);													//	Move upper half to lower half
	}
	if ( UPPER_BYTE(val) ) {													//	Check upper half of lower word, if non zero, then lower half is all used
		bits += 8;																//	Lower half is 8 bits
		val8 = UPPER_BYTE(val);													//	Move upper half to lower half
	}
	while ( val8 ) {															//	Loop lower uint8_t to find how many bits are used
		bits++;																	//	Add a bit to the count
		val8 >>= 1;																//	Shift by a bit
	}
	return bits;
}

//########################################################################################################################
//	CRC/Checksum
//########################################################################################################################

void	crcFletcher16_Init( crcFletcher16_t *crc )
{
	crc->result = 0;
}

uint16_t	crcFletcher16( crcFletcher16_t *crc, uint8_t *data, uint8_t count )
{
	while( count-- )
	{
		crc->sum1 = ((uint16_t)crc->sum1 + *data++) % 255;
		crc->sum2 = ((uint16_t)crc->sum2 + crc->sum1) % 255;
	}
	return crc->result;
}

uint8_t	Checksum8( uint8_t *data, uint16_t len )
{
	uint8_t cksm = 0;
	while( len-- )
		cksm += *data++;
	return cksm;
}

