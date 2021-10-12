#include "Hardware.h"
#include "nrf_drv_gpiote.h"

#define NRF_LOG_MODULE_NAME HW
#ifdef HW_LOG_LEVEL
#define	NRF_LOG_LEVEL HW_LOG_LEVEL
#else
#define	NRF_LOG_LEVEL 1
#endif
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
NRF_LOG_MODULE_REGISTER();

Hardware_t hardware;

#include "LIS3DH.h"


//################################################################################
//	TWI/I2C Manager
//################################################################################
#if 1
#define TWI_INSTANCE_ID_0	0			/* Hardware ID for this TWI instance. Also shared with SPI, so they must be unique */
#define TWI_QUEUE_LENGTH_0	5			/* Queue size for TWI transactions. May need more if this TWI is shared on multiple threads. */

NRF_TWI_MNGR_DEF(app_twi_0, TWI_QUEUE_LENGTH_0, TWI_INSTANCE_ID_0);

nrf_drv_twi_config_t const twi_0_default_config = {
   .scl                = I2C_SCL_PIN,
   .sda                = I2C_SDA_PIN,
   .frequency          = NRF_TWI_FREQ_100K,
   .interrupt_priority = APP_IRQ_PRIORITY_LOWEST,
   .clear_bus_init     = true
};

// TWI (with transaction manager) initialization.
static void TWI_0_Init(void)
{
	uint32_t error;
	// First instance on I2C (on the board).
	error = nrf_twi_mngr_init( &app_twi_0, &twi_0_default_config ) ;
	APP_ERROR_CHECK( error );
	hardware.drivers.p_twi_0 = &app_twi_0;
	NRF_LOG_DEBUG("TWI0 initialized.");
}
#else
static void TWI_0_Init(void) {}
#endif

//################################################################################
//	Devices
//################################################################################

//################################################################################
//	Initialization
//################################################################################

//	Must be called before the RTOS is started, so hardware can be available for the tasks
void Hardware_Init( void )
{
	NRF_LOG_DEBUG( "Hardware Initialization" );
	nrf_drv_gpiote_init();
	TWI_0_Init();
}

//	Must be called from a thread, so it can block during initialization
void Devices_Init( void )
{
	NRF_LOG_DEBUG( "Device Initialization" );
}
