#ifndef ROCKETRELAY__
#define ROCKETRELAY__

#define RR_PIN_UNUSED	0xFF
#define RR_PIN_INVERT	0x80

typedef enum
{
	RR_STATUS_NONE,
	RR_STATUS_ERROR,
	RR_STATUS_READY,
	RR_STATUS_ON,
} RocketRelay_Status_t;

typedef struct
{
	uint8_t		outPin;
	uint8_t		inPin;
} RocketRelay_Config_t;

typedef struct
{
	bool					isOn;

	RocketRelay_Config_t	config;
	RocketRelay_Status_t	status;

	int16_t					onTime;
} RocketRelay_t;




void RocketRelay_Init( RocketRelay_t * p_relay, RocketRelay_Config_t * p_config );
void RocketRelay_Process( RocketRelay_t * p_relay, int16_t inc );

// Set time to be on, a time of 0 means stay on forever
void RocketRelay_On( RocketRelay_t * p_relay, int16_t time );
void RocketRelay_Off( RocketRelay_t * p_relay );
void RocketRelay_Set( RocketRelay_t * p_relay, bool on );


#endif // ROCKETRELAY__