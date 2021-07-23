#ifndef MCP73833_LIPO_H__
#define MCP73833_LIPO_H__

#include "stdint.h"
#include "stdbool.h"

typedef struct
{
	uint32_t			stat1_pin;
	uint32_t			stat2_pin;
	uint32_t			pg_pin;
} mcp73833_config_t;


typedef enum
{
	CHG_SHUTDOWN,
//	CHG_STANDBY,
	CHG_CHARGING,
	CHG_CHARGED,
	CHG_FAULT,
} charger_state_t;

typedef enum
{
	CHG_EMPTY,
	CHG_LOW,
	CHG_MID,
	CHG_HIGH,
	CHG_FULL,
} charger_level_t;

typedef struct
{
	bool				pgState;
	bool				stat1;
	bool				stat2;

	float				voltage;
    charger_state_t		chargeState;
	uint8_t				percent;
//	charger_level_t		chargeLevel;
	bool				isBusy;

	mcp73833_config_t	config;

} mcp73833_t;



void charger_init( mcp73833_t * p_inst, mcp73833_config_t * p_config );
void charger_update( mcp73833_t * p_inst, float vbatt );



#endif