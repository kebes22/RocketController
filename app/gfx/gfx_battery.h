#ifndef GFX_BATTERY_H__
#define GFX_BATTERY_H__

#include "gfx.h"

typedef struct
{
	gfx_rect_t		outline;
	gfx_dim_t		dim;
    gfx_dim_t		cap;
	uint8_t			border;
	bool			invert;
} gfx_battery_t;


void	gfx_battery_create( gfx_battery_t *p_batt, int16_t x, int16_t y, int16_t width, int16_t height, bool invert );
void	gfx_battery_draw( gfx_battery_t *p_batt, gfx_update_type_t update, int8_t percent, bool show_percent, bool warn );


#endif //GFX_BATTERY_H__