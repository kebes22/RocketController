#include "sdk_common.h"
#include "ppm_capture.h"

#include "nrfx_timer.h"
#include "nrfx_gpiote.h"
#include "nrfx_ppi.h"

#include "io_config.h"

#define NRF_LOG_MODULE_NAME PPM
#ifdef PPM_LOG_LEVEL
#define	NRF_LOG_LEVEL PPM_LOG_LEVEL
#else
#define	NRF_LOG_LEVEL 1
#endif
#include "log.h"


uint16_t ppm_samples[PPM_MAX_CHANNELS] = {0};


//TODO add timeout on no new update
//TODO add valid flag



// Create timer instance
const nrfx_timer_t ppm_timer = NRFX_TIMER_INSTANCE(2);				  

nrf_ppi_channel_t m_ppi_channel;

ppm_capture_config_t m_config = {0};

static uint16_t m_samples[PPM_MAX_CHANNELS];
static uint8_t m_sample_idx = 0;

//################################################################################
//	Interrupt handlers
//################################################################################
void ppm_timer_handler (nrf_timer_event_t event_type, void * p_context)
{
	// Do nothing. Timer requires valid handler.
}

void ppm_pin_handler ( nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action )
{
	static uint32_t oldCount = 0;
	static uint16_t sum = 0;

	bool rising = nrf_gpio_pin_read( pin );
	uint32_t newCount = nrfx_timer_capture_get( &ppm_timer, 0 );
	uint16_t diff = newCount-oldCount;
	
	sum += diff;
	if ( rising ) {		// Rising is end of last sample
		if ( sum < 3000 ) {
			m_samples[m_sample_idx++] = sum;
		} else {
			// Pulse train done, update samples
			LOG_RAW_DEBUG( LOG_COLOR_CYAN "PPM IN " );
			for ( uint8_t i = 0; i<m_sample_idx; i++ ) {
				LOG_RAW_DEBUG( "(%d) %4d, ", i, m_samples[i] );
				ppm_samples[i] = m_samples[i];
			}
			LOG_RAW_DEBUG( "\n" );
			if ( m_config.handler ) {
				ppm_capture_evt_t evt;
				evt.type = PPM_EVT_RX_DATA;
				evt.params.rx_data.channels = ppm_samples;
				evt.params.rx_data.count = m_sample_idx;
				m_config.handler( &evt );
			}
			m_sample_idx = 0;
		}
		sum = 0;
	}
	
	oldCount = newCount;
}


void ppm_capture_init( ppm_capture_config_t * p_config )
{
	ASSERT(p_config != NULL);
//	ASSERT(p_config->pin);
	ret_code_t err_code;

// Copy config
	m_config = *p_config;

// Set up timer
	nrfx_timer_config_t timer_cfg = {
		.frequency				= (nrf_timer_frequency_t)NRF_TIMER_FREQ_1MHz,
		.mode					= (nrf_timer_mode_t)NRF_TIMER_MODE_TIMER,
		.bit_width				= (nrf_timer_bit_width_t)NRF_TIMER_BIT_WIDTH_32,
		.interrupt_priority		= APP_IRQ_PRIORITY_LOW,
		.p_context				= NULL
	};
	err_code = nrfx_timer_init( &ppm_timer, &timer_cfg, ppm_timer_handler );
	APP_ERROR_CHECK(err_code);

// Set up pin interupt
	if ( !nrfx_gpiote_is_init() ) nrfx_gpiote_init();
	nrfx_gpiote_in_config_t pinConfig = NRFX_GPIOTE_CONFIG_IN_SENSE_TOGGLE( true );
	err_code = nrfx_gpiote_in_init( p_config->pin, &pinConfig, ppm_pin_handler );
	APP_ERROR_CHECK(err_code);

// Set up PPI for input capture
	err_code = nrfx_ppi_channel_alloc( &m_ppi_channel );
	APP_ERROR_CHECK(err_code);
	err_code = nrfx_ppi_channel_assign(
		m_ppi_channel,
		nrfx_gpiote_in_event_addr_get( p_config->pin ),
		nrfx_timer_capture_task_address_get( &ppm_timer, 0 )
	);
	APP_ERROR_CHECK(err_code);

	nrfx_ppi_channel_enable( m_ppi_channel );
	nrfx_timer_enable( &ppm_timer );
	nrfx_gpiote_in_event_enable( p_config->pin, true );
}



