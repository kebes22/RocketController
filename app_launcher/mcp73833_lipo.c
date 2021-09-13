#include "mcp73833_lipo.h"
#include "nrf_gpio.h"


//typedef struct
//{
//	mcp73833_config_t	config;
//} mcp73833_lipo_t;
//
//mcp73833_lipo_t m_mcp73833;

//################################################################################
//	Charger
//################################################################################
/*	Charger state table from datasheet
	----------------------------------------
	Charge Cycle State	STAT1	STAT2	PG
	----------------------------------------
	Shutdown			Hi-Z	Hi-Z	Hi-Z
	Standby				Hi-Z	Hi-Z	L
	Charge in Progress	L		Hi-Z	L
	Charge Complete		Hi-Z	L		L
	Temperature Fault	Hi-Z	Hi-Z	L
	Timer Fault			Hi-Z	Hi-Z	L
	System Test Mode	L		L		L
	----------------------------------------
*/

#define BATT_FULL_VOLTAGE_MIN	4.0f
#define BATT_HIGH_VOLTAGE_MIN	3.8f
#define BATT_MID_VOLTAGE_MIN	3.6f
#define BATT_LOW_VOLTAGE_MIN	3.4f
#define BATT_EMPTY_VOLTAGE_MIN	3.2f

//################################################################################
//	Percentage mapping
//################################################################################

typedef struct
{
	uint8_t		percent;
	float		voltage;
} battery_map_item_t;

//static const battery_map_item_t battery_map[] = {
//	{ 0,	CHG_VBATT_0 },
//	{ 25,	CHG_VBATT_25 },
//	{ 50,	CHG_VBATT_50 },
//	{ 75,	CHG_VBATT_75 },
//	{ 100,	CHG_VBATT_100 },
//};

//// Battery mapping. first should be 0, last should be 100
//// Inputs will be clipped to the bounds
//static const battery_map_item_t battery_map[] = {
//	{ 0,	3.30f },
//	{ 5,	3.50f },
//	{ 25,	3.65f },
//	{ 50,	3.75f },
//	{ 75,	3.85f },
//	{ 95,	4.10f },
//	{ 100,	4.20f },
//};

// Battery mapping. first should be 0, last should be 100
// Inputs will be clipped to the bounds
static const battery_map_item_t battery_map[] = {
	{ 0,	3.30f },
	{ 5,	3.50f },
	{ 25,	3.65f },
	{ 50,	3.75f },
	{ 75,	3.85f },
	{ 80,	4.00f },	// Verified (ish) experementally
	{ 90,	4.10f },	// Verified (ish) experementally
	{ 100,	4.20f },	// Verified (ish) experementally
};


#define BATT_MAP_ITEMS	(sizeof(battery_map)/sizeof(battery_map[0]))

uint8_t _get_percentage( float voltage )
{
	float percent;

	int8_t idx = 0;
	while ( (idx < (int8_t)BATT_MAP_ITEMS) && (voltage > battery_map[idx].voltage) ) {
		idx++;
	}
	idx -= 1;
	if ( idx < 0 ) {
		percent = battery_map[0].percent;
	} else if ( idx >= BATT_MAP_ITEMS ) {
		percent = battery_map[BATT_MAP_ITEMS].percent;
	} else {
		float vStart = battery_map[idx].voltage;
		float vEnd = battery_map[idx+1].voltage;
		float pStart = battery_map[idx].percent;
		float pEnd = battery_map[idx+1].percent;
		percent = (voltage - vStart) * (pEnd - pStart) / (vEnd - vStart) + pStart;
	}
	return percent;
}

void charger_update( mcp73833_t * p_inst, float vbatt )
{
	p_inst->voltage = vbatt;
	p_inst->pgState = nrf_gpio_pin_read( p_inst->config.pg_pin );
	p_inst->stat1 = nrf_gpio_pin_read( p_inst->config.stat1_pin );
	p_inst->stat2 = nrf_gpio_pin_read( p_inst->config.stat2_pin );

	charger_state_t newState = CHG_SHUTDOWN;
	if ( p_inst->pgState == 0 ) {
		if ( p_inst->stat1 == 0 )
			newState = CHG_CHARGING;
		else if ( p_inst->stat2 == 0 )
			newState = CHG_CHARGED;
		else
			newState = CHG_FAULT;
	} else {
		newState = CHG_SHUTDOWN;
	}

	p_inst->chargeState = newState;
	p_inst->percent = _get_percentage( vbatt );

#if 0
// Check if charger was just disconnected
	bool doShutdown = false;
	if ( newState == CHG_SHUTDOWN && p_inst->isBusy )
		doShutdown = true;

	p_inst->isBusy = ( newState == CHG_CHARGING || newState == CHG_CHARGED );
	p_inst->chargeState = newState;

//	if ( doShutdown )
//		_do_suicide();

////	if ( color ) {
//		p_inst->ledColor = *color;
////	}
#endif
}




void charger_init( mcp73833_t * p_inst, mcp73833_config_t * p_config )
{
	ASSERT( p_config );
	p_inst->config = *p_config;
	
	nrf_gpio_cfg_input( p_inst->config.pg_pin, NRF_GPIO_PIN_PULLUP );
	nrf_gpio_cfg_input( p_inst->config.stat1_pin, NRF_GPIO_PIN_PULLUP );
	nrf_gpio_cfg_input( p_inst->config.stat2_pin, NRF_GPIO_PIN_PULLUP );
}

