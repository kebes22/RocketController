#include "sdk_common.h"
#include "RocketRelay.h"
#include "nrf_gpio.h"

#define GET_PIN_NUM(_pin)		(_pin & ~(RR_PIN_INVERT))
#define GET_PIN_INV(_pin)		(_pin & RR_PIN_INVERT)

#define ON_FOREVER				(-1)	// Special value

static inline bool _read_pin( uint8_t pin )
{
	nrf_gpio_cfg_input(GET_PIN_NUM(pin), NRF_GPIO_PIN_NOPULL);
	bool state = nrf_gpio_pin_read(GET_PIN_NUM(pin));
	if ( GET_PIN_INV(pin) )
		state ^= 1;
	return state;
}

static inline void _write_pin( uint8_t pin, bool state )
{
	if ( GET_PIN_INV(pin) )
		state ^= 1;
	nrf_gpio_pin_write(GET_PIN_NUM(pin), state);
	nrf_gpio_cfg_output(GET_PIN_NUM(pin));
}

void RocketRelay_Process( RocketRelay_t * p_relay, int16_t inc )
{
	bool good = true; // Default to good
	if ( p_relay->config.inPin != RR_PIN_UNUSED ) {
		good = p_relay->isOn? true: _read_pin(p_relay->config.inPin);
	}

	p_relay->isOn = (p_relay->onTime != 0);
	_write_pin( p_relay->config.outPin, p_relay->isOn );
	if ( p_relay->onTime != ON_FOREVER ) {
		p_relay->onTime -= inc;
		if ( p_relay->onTime < 0 )
			p_relay->onTime = 0;
	}

    p_relay->status = p_relay->isOn? RR_STATUS_ON: (good? RR_STATUS_READY: RR_STATUS_ERROR);
}

// Set time to be on, a time of 0 means stay on forever
void RocketRelay_On( RocketRelay_t * p_relay, int16_t time )
{
	//if ( p_relay->status >= RR_STATUS_READY ) {
		p_relay->onTime = (time == 0)? ON_FOREVER: time;
        p_relay->isOn = true;
	//}
}

void RocketRelay_Off( RocketRelay_t * p_relay )
{
	p_relay->onTime = 0;
}

void RocketRelay_Set( RocketRelay_t * p_relay, bool on )
{
	if ( on )
		RocketRelay_On( p_relay, 0 );
	else
		RocketRelay_Off( p_relay );
}


void RocketRelay_Init( RocketRelay_t * p_relay, RocketRelay_Config_t * p_config )
{
	ASSERT(p_relay);
	ASSERT(p_config->outPin);

	p_relay->config = *p_config;
	p_relay->isOn = false;
	p_relay->onTime = 0;

	_write_pin(p_relay->config.outPin, 0);

	if ( p_relay->config.inPin != RR_PIN_UNUSED ) {
		(void)_read_pin(p_relay->config.inPin);
	}
}
