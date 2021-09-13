#ifndef GFX_H__
#define GFX_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

//################################################################################
//	Primitive types
//################################################################################
typedef struct
{
	int16_t			x;
	int16_t			y;
} gfx_point_t;

typedef struct
{
	int16_t			w;
	int16_t			h;
} gfx_dim_t;

typedef struct
{
	gfx_point_t		p1;
	gfx_point_t		p2;
} gfx_rect_t;

//################################################################################
//	
//################################################################################
//TODO split this out to menu specific GFX
typedef enum
{
	GFX_UPDATE_NONE		= 0x00,		// No update required
    GFX_UPDATE_MINOR	= 0x01,		// Minor update, old image still exists, (value change, etc.)
	GFX_UPDATE_MAJOR	= 0x02,		// Major update, old image still exists, (size change, etc.)
	GFX_UPDATE_FULL		= 0x04,		// Full update, screen has been erased, redraw everything
} gfx_update_type_t;

//################################################################################
//	Macros
//################################################################################

#define GFX_P2_TO_DIM(p1,p2)	(p2-p1+1)
#define GFX_DIM_TO_P2(p1,d)		(p1+d-1)

#endif //GFX_H__