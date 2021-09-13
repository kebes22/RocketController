#ifndef HARDWARE_H__
#define HARDWARE_H__


#include "io_config.h"
#include "nrf_spi_mngr.h"
#include "nrf_twi_mngr.h"

//#include "SPI_Flash.h"
//#include "timers.h"
//#include "led_flasher.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	nrf_twi_mngr_t const	*p_twi_0;
}	HW_Drivers_t;


typedef struct
{
	HW_Drivers_t			drivers;
}	Hardware_t;

extern Hardware_t hardware;

void Hardware_Init( void );
void Devices_Init( void );

#ifdef __cplusplus
}
#endif

#endif // HARDWARE_H__
