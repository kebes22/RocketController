#include "sdk_common.h"
#include "lib_core.h"

#include "nrf_pwr_mgmt.h"
#include "nrf_soc.h"
#include "nrf_sdh_freertos.h"

//#include "AppComms.h"
//#include "advertising.h"
#include "Hardware.h"
#include "UI.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "central.h"

#include "suicide_pin.h"

#define NRF_LOG_MODULE_NAME main
#include "log.h"

/**@brief Function called on start of soft device task. */
static void sdh_start_hook( void * p_context )
{
	UNUSED_PARAMETER(p_context);
}

/**@brief A function which is hooked to idle task.
 * @note Idle hook must be enabled in FreeRTOS configuration (configUSE_IDLE_HOOK).
 */
void vApplicationIdleHook( void )
{
	core_run_idle();
}

/**@brief Function for application main entry.
 */
int main(void)
{
	suicide_pin_init( PWR_ON_PIN, 1 );	// Initialize suicide circuit to keep on

	core_init(); // common init for projects

	nrf_pwr_mgmt_init();
//	sd_power_dcdc_mode_set( NRF_POWER_DCDC_ENABLE );

//	advertising_init();	// must come after comms_init

	Hardware_Init();
	
	UI_Init();

	central_init();

	// Create a FreeRTOS task for the BLE stack.
	nrf_sdh_freertos_init( sdh_start_hook, NULL );

	NRF_LOG_INFO( LOG_COLOR_GREEN "TransModule init done" );

	NRF_LOG_FLUSH();
	// Start FreeRTOS scheduler.
	vTaskStartScheduler();
	for (;;)
	{
		APP_ERROR_HANDLER(NRF_ERROR_FORBIDDEN);
	}
}
