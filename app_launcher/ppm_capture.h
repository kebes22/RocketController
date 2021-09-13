#ifndef PPM_CAPTURE_H__
#define PPM_CAPTURE_H__


#ifdef __cplusplus
extern "C" {
#endif

#define PPM_MAX_CHANNELS	8
extern uint16_t ppm_samples[PPM_MAX_CHANNELS];


//################################################################################
//	Event structures
//################################################################################

/**@brief	TXRX Stack event types. */
typedef enum
{
	PPM_EVT_NONE,						/**< Null event (should not be triggered) */
	PPM_EVT_RX_DATA,					/**< Data received. */
	PPM_EVT_TIMEOUT,					/**< Timeout. */
}	ppm_capture_evt_type_t;


typedef struct
{
	uint16_t		*channels;
	uint8_t			count;
} ppm_capture_rx_data_t;

typedef struct
{
	ppm_capture_evt_type_t		type;			/**< Event type. */
	union
	{
		ppm_capture_rx_data_t	rx_data;		/**< @ref event data. */
	} params;
}	ppm_capture_evt_t;


//################################################################################
//	Init structures
//################################################################################

typedef void (*ppm_capture_data_handler_t) (ppm_capture_evt_t * p_evt);

typedef struct
{
	uint32_t						pin;
	ppm_capture_data_handler_t		handler;
	uint32_t						timeout;
} ppm_capture_config_t;


//################################################################################
//	Function prototypes
//################################################################################


void ppm_capture_init( ppm_capture_config_t * config );


#ifdef __cplusplus
}
#endif

#endif