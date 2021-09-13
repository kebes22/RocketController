#include "suicide.h"
#include "nrf_gpio.h"

#define INVALID_PIN_NUMBER	0xFFFFFFFF

uint32_t	m_pinNumber	= INVALID_PIN_NUMBER;
bool		m_onState	= 0;

void suicide( void )
{
// Kill device and wait for reset
	if ( m_pinNumber != INVALID_PIN_NUMBER ) {
		nrf_gpio_pin_write( m_pinNumber, !m_onState );
//		while (1);
	}
}

void suicide_init( uint32_t pinNum, bool onState )
{
	m_pinNumber = pinNum;
    m_onState = onState;

	nrf_gpio_cfg_output( m_pinNumber );
	nrf_gpio_pin_write( m_pinNumber, onState );
}