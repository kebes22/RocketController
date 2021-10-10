#include "sdk_common.h"

#if NRF_MODULE_ENABLED(SSD1306)

#include "nrf_lcd.h"
//#include "nrf_drv_spi.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"

#include "nrf_drv_twi.h"

#define USE_ISR

#define SSD1306_TWI_ADDR	(0x3C)

#define SSD1306_WIDTH		128
#define SSD1306_HEIGHT		32

#define SSD1306_ROWS		4
#define SSD1306_COLS		SSD1306_WIDTH

// Control block (managed by nrf_gfx)
static lcd_cb_t ssd1306_cb = {
	.height = SSD1306_HEIGHT,
	.width = SSD1306_WIDTH
};

// Entire screen gets divided into an 8x8 grid, where each square can be updated independently
// 8x8 works well because each screen byte covers 8 vertical pixels (1 vertical dirty segment),
// and each byte in the dirty array is 8 bits wide, so each bit represents a different grid segment
#define DIRTY_Y_SEGMENTS	(SSD1306_HEIGHT/8)
#define DIRTY_X_SEGMENTS	(SSD1306_WIDTH/8)

// FIXME duplicated from GLCD
typedef enum
{
	DRAW_OFF = 0,
	DRAW_ON,
	DRAW_XOR,
}	drawMode_t;

typedef struct
{
	uint8_t		page;		// Page of y coordinate
	uint8_t		offset;		// Offset of pixel within page
	uint8_t		col;		// Column of x coordinate
	uint8_t *	p_buf;		// Pointer to buffer byte containing this coordinate
}	_bufferCursor_t;

_bufferCursor_t m_cursor;

typedef struct
{
	uint8_t display_buf[SSD1306_ROWS * SSD1306_WIDTH];	// Each byte is 8 vertical pixels
	uint8_t dirty_flags[DIRTY_Y_SEGMENTS];				// Each 8 rows x
}	ssd1306_t;

static ssd1306_t m_ssd1306;


#ifdef USE_ISR
static volatile bool m_runUpdate = false;
static volatile bool m_twi_is_busy = false;
#endif


// TODO move external to allow sharing? Would also need to use spi manager lib with queue.
//static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SSD1306_TWI_INSTANCE);  /**< SPI instance. */

static const nrf_drv_twi_t twi_inst = NRF_DRV_TWI_INSTANCE(SSD1306_TWI_INSTANCE);  /**< SPI instance. */

//################################################################################
//	Command list
//################################################################################

#define CONT							0x80
#define CONT_DATA						0x40

#define SSD1306_CMD_CONTRAST(x)			CONT, (0x81), CONT, (x)			// x=0-255 level
#define SSD1306_CMD_ALL_ON(x)			CONT, (0xA4|(x&0b1))			// x=0 - Normal output, x=1 - All on
#define SSD1306_CMD_INVERT(x)			CONT, (0xA6|(x&0b1))			// x=0 - Normal, x=1 - Invert
#define SSD1306_CMD_DISP_ON(x)			CONT, (0xAE|(x&0b1))			// x=0 - Off, x=1 - On
#define SSD1306_CMD_COL_LOWER(x)		CONT, (0x00|(x&0b1111))			// x=0x00-0x0F
#define SSD1306_CMD_COL_UPPER(x)		CONT, (0x10|(x&0b1111))			// x=0x00-0x0F
#define SSD1306_CMD_ADDR_MODE(x)		CONT, (0x20)					// followed by 0=Horizontal 1=Vertical 2=page
#define SSD1306_CMD_SET_PAGE(x)			CONT, (0xB0|(x&0b111))			// page = 0-3 (0-7 for 128x64)
#define SSD1306_CMD_START_LINE(x)		CONT, (0x40|(x&0b111111))		// line = 0-31 (0-63 for 128x64)
#define SSD1306_CMD_SEG_REMAP(x)		CONT, (0xA0|(x&0b1))			// x=0 normal, x=1 reverse
#define SSD1306_CMD_SET_MUX(x)			CONT, (0xA8), CONT, (x)			// followed by 15-63 depending on lines used
#define SSD1306_CMD_COM_DIR(x)			CONT, (0xC0|((x&0b1)<<3))		// x=0 COM scan forward, x=1 COM scan reverse
#define SSD1306_CMD_DISP_OFFSET(x)		CONT, (0xD3), CONT, (x)			// followed by offset 0-63
#define SSD1306_CMD_COM_PINS(s,e)		CONT, (0xDA), CONT, (0b10|((s&0b1)<<4)|((e&0b1)<<5))	// s=0 Sequential, s=1 Alt, e=0 No remap, e=1 l/r remap
#define SSD1306_CMD_CLK_DIV(f,d)		CONT, (0xD5), CONT, (((f&0b1111)<<4)|(d&0b1111))		// f=0=15 (frequency), d=0-15 (divide)
#define SSD1306_CMD_CHG_PUMMP(x)		CONT, (0x8D), CONT, (0b10|((x&0b1)<<2))					// x=0 off, x=1 on
#define SSD1306_CMD_PRE_CHG(p2,p1)		CONT, (0xD9), CONT, (((p2&0b1111)<<4)|(p1&0b1111))		// p2=0=15 (phase 2), p1=0=15 (phase 1)
#define SSD1306_CMD_VCOM_DESEL(x)		CONT, (0xDB), CONT, ((x&0b111)<<4)						// x=0-7

#define SSD1306_CMD_NOP					CONT, (0xE3)

#if 1
static uint8_t init_commands[] = {
	SSD1306_CMD_DISP_ON(0),
	SSD1306_CMD_CLK_DIV(8,0),
	SSD1306_CMD_SET_MUX(0x1F),
	SSD1306_CMD_DISP_OFFSET(0),
	SSD1306_CMD_START_LINE(0),
	SSD1306_CMD_CHG_PUMMP(1),
	SSD1306_CMD_SEG_REMAP(1),
	SSD1306_CMD_COM_DIR(1),
	SSD1306_CMD_COM_PINS(0,0),
	SSD1306_CMD_CONTRAST(0x80),
	SSD1306_CMD_PRE_CHG(1,15),
	SSD1306_CMD_VCOM_DESEL(4),
	SSD1306_CMD_ALL_ON(0),
	SSD1306_CMD_DISP_ON(1),
};
#else
static uint8_t init_commands[] = {
	SSD1306_CMD_DISP_ON(0),
};
#endif

//################################################################################
//	TWI interface
//################################################################################
static inline void twi_wait( void )
{
	//while ( m_twi_is_busy );
}

//----------------------------------------
// Non-Blocking transfers for update process
//----------------------------------------
static inline ret_code_t write_commands_start( uint8_t * p_cmd, uint16_t len )
{
	ret_code_t err_code = NRF_SUCCESS;
#ifdef USE_ISR
	m_twi_is_busy = true;
#endif
#if 0
	nrf_drv_twi_xfer_desc_t const xfer = {
		.type				= NRF_DRV_TWI_XFER_TX,
		.address			= SSD1306_TWI_ADDR,
		.primary_length		= len,
		.secondary_length	= 0,
		.p_primary_buf		= p_cmd,
		.p_secondary_buf	= NULL,
	};
	err_code = nrf_drv_twi_xfer( &twi_inst, &xfer, 0 );
#else
	err_code = nrf_drv_twi_tx( &twi_inst, SSD1306_TWI_ADDR, p_cmd, len, 0 );
#endif
	return err_code;
}

static inline ret_code_t write_data_start( uint8_t * p_data, uint16_t len )
{
	ret_code_t err_code = NRF_SUCCESS;
#ifdef USE_ISR
	m_twi_is_busy = true;
#endif
	uint8_t control[] = { CONT_DATA };
	nrf_drv_twi_xfer_desc_t const xfer = {
		.type				= NRF_DRV_TWI_XFER_TXTX,
		.address			= SSD1306_TWI_ADDR,
		.primary_length		= sizeof(control),
		.secondary_length	= len,
		.p_primary_buf		= control,
		.p_secondary_buf	= p_data,
	};
	err_code = nrf_drv_twi_xfer( &twi_inst, &xfer, 0 );
	return err_code;
}

static inline ret_code_t twi_kickstart ( void )
{
	ret_code_t err_code = NRF_SUCCESS;
	// Kickstart SPI if not already running
	static uint8_t dummy[] = { SSD1306_CMD_NOP };
	if ( !m_twi_is_busy )
		err_code = write_commands_start( dummy, sizeof(dummy) );
	return err_code;
}

//----------------------------------------
// Blocking transfers for normal use
//----------------------------------------

static inline ret_code_t write_commands_blocking( uint8_t * p_data, uint16_t len )
{
	ret_code_t err_code = NRF_SUCCESS;
	twi_wait();
	write_commands_start( p_data, len );
	twi_wait();
	return err_code;
}

static inline ret_code_t write_data_blocking( uint8_t * p_data, uint16_t len )
{
	ret_code_t err_code = NRF_SUCCESS;
	twi_wait();
	write_data_start( p_data, len );
	twi_wait();
	return err_code;
}

//################################################################################
//	Utils
//################################################################################
#define SWAP(a,b)			do { typeof(a) temp=a; a=b; b=temp; } while(0)
#define _LIMIT_RANGE(_var,_min,_max)	do { if (_var<(_min)) _var=(_min); if (_var>(_max)) _var=(_max); } while(0)

static void _rotate( int16_t *x, int16_t *y )
{
	int16_t x_old = *x;
	int16_t y_old = *y;
	switch ( ssd1306_cb.rotation )
	{
		case NRF_LCD_ROTATE_0:
			// Do nothing
			break;

		case NRF_LCD_ROTATE_90:
			*x = SSD1306_WIDTH - y_old - 1;
			*y = x_old;
			break;

		case NRF_LCD_ROTATE_180:
			*x = SSD1306_WIDTH - x_old - 1;
			*y = SSD1306_HEIGHT - y_old - 1;
			break;

		case NRF_LCD_ROTATE_270:
			*x = y_old;
			*y = SSD1306_HEIGHT - x_old - 1;
			break;
	}
}

static void * 	_SetBufferCursor( uint16_t x, uint16_t y/*, drawMode_t mode*/ )
{
	m_cursor.page = (y>>3);
	m_cursor.offset = (y&0x07);
	m_cursor.col = x;
	m_cursor.p_buf = &m_ssd1306.display_buf[x + (m_cursor.page*SSD1306_WIDTH)];
}

void	_WriteRaw( uint8_t mask, drawMode_t mode )
{
	uint8_t data = *m_cursor.p_buf;
	switch ( mode ) {
		case DRAW_OFF:
			data &= ~mask;
			break;

		case DRAW_ON:
			data |= mask;
			break;

		case DRAW_XOR:
			data ^= mask;
			break;
	}
	*m_cursor.p_buf++ = data;
	//TODO dirty flags are broken
	m_ssd1306.dirty_flags[m_cursor.page] |= 1<<(m_cursor.col/DIRTY_X_SEGMENTS);	// Set dirty flag
	m_cursor.col++;
}


//################################################################################
//	API functions
//################################################################################

static void _ssd1306_pixel_draw(uint16_t x, uint16_t y, uint32_t color)
{
	int16_t x1=x, y1=y;
	_rotate( &x1, &y1 );								// Rotate coordinates if needed
	if ( (uint16_t)x1 >= SSD1306_WIDTH || (uint16_t)y1 >= SSD1306_HEIGHT )	// Only draw if within range
		return;
	_SetBufferCursor( x1, y1 );						// Calculate position
	uint8_t pixelMask = 1<<m_cursor.offset;			// Get y subpixel offset mask
	_WriteRaw( pixelMask, color );					// Write data
}

static void _ssd1306_rect_draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color)
{
// Get signed values
	int16_t x1 = x;
	int16_t y1 = y;
	int16_t x2 = x1+width-1;
	int16_t y2 = y1+height-1;
// Rotate and swap coordinates if needed
	_rotate( &x1, &y1 );
	_rotate( &x2, &y2 );
	if ( x1>x2 ) SWAP(x1,x2);
	if ( y1>y2 ) SWAP(y1,y2);
// Cancel if entirely off screen
	if ( x1 > SSD1306_WIDTH-1 || x2 < 0 || y1 > SSD1306_HEIGHT-1 || y2 < 0 )
		return;
// Clip to visible area
	_LIMIT_RANGE( x1, 0, SSD1306_WIDTH-1 );
	_LIMIT_RANGE( x2, 0, SSD1306_WIDTH-1 );
	_LIMIT_RANGE( y1, 0, SSD1306_HEIGHT-1 );
	_LIMIT_RANGE( y2, 0, SSD1306_HEIGHT-1 );
// Get new width/height (rotated and clipped)
	width = x2-x1+1;
	height = y2-y1+1;

	drawMode_t mode = color;

	uint8_t mask, h, i;
	_SetBufferCursor(x1,y1);
	y1 -= m_cursor.offset;
	mask = 0xFF;						//	Initial mask
	if(height < 8-m_cursor.offset) {	//	Check for less than remainder of byte
		mask >>= (8-height);			//	Fix mask
		h = height;
	} else {
		h = 8-m_cursor.offset;
	}
	mask <<= m_cursor.offset;

//	Draw first (possibly partial) row
	i=width;
	while ( i-- ) {
		_WriteRaw( mask, mode );
	}

//	Draw middle complete rows
	while(h+8 <= height) {
		h += 8; y1 += 8;
		_SetBufferCursor(x1, y1);
		i=width;
		while ( i-- )
			_WriteRaw( 0xFF, mode );
	}

//	Draw last partial row
	if(h < height) {
		mask = ~(0xFF << (height-h));
		_SetBufferCursor(x1, y1+8);
		i=width;
		while ( i-- )
			_WriteRaw( mask, mode );
	}
}


#ifndef USE_ISR
static void _set_address( uint8_t row, uint8_t col )
{
	uint8_t commands[] = {
		SSD1306_CMD_SET_PAGE(row),
		SSD1306_CMD_SET_COL_LOW(0),
		SSD1306_CMD_SET_COL_HIGH(0),
	};
	write_command_list( commands, sizeof(commands) );
}
#endif

static void _ssd1306_display(void)
{
#ifdef USE_ISR
	CRITICAL_REGION_ENTER();
		m_runUpdate = true;
		twi_kickstart();
	CRITICAL_REGION_EXIT();
#elif 1
// NOTE currently, blasting the whole screen out is faster than updating only the changed parts
// this could be changed to use dirty flags per row, instead of multiple per row
	uint8_t * ptr = m_ssd1306.display_buf;
	for ( uint8_t row = 0; row < SSD1306_ROWS; row++ ) {
		_set_address( row, 0 );
        write_data( ptr, SSD1306_COLS );
		ptr += SSD1306_COLS;
	}
#else
	// Iterate dirty flags and send only modified blocks
	uint8_t * ptr = m_ssd1306.display_buf;
	for ( uint16_t row = 0; row < DIRTY_Y_SEGMENTS; row++ ) {
		uint8_t flags = m_ssd1306.dirty_flags[row];
		m_ssd1306.dirty_flags[row] = 0;
		uint8_t colMask = 1;
		write_command( SSD1306_CMD_SET_PAGE(row) );
		for ( uint16_t col = 0; col < SSD1306_WIDTH; col += DIRTY_X_SEGMENTS ) {
			if ( flags & colMask ) {
				write_command( SSD1306_CMD_SET_COL_LOW(col) );
				write_command( SSD1306_CMD_SET_COL_HIGH(col>>4) );
#if 0
				uint8_t count = DIRTY_X_SEGMENTS;
				while ( count-- )
					write_data( *ptr++ );
#else
				write_data_len( ptr, DIRTY_X_SEGMENTS );
				ptr += DIRTY_X_SEGMENTS;
#endif
			} else {
				ptr += DIRTY_X_SEGMENTS;
			}
			colMask <<= 1;
		}
	}
#endif
}

static void _ssd1306_rotation_set(nrf_lcd_rotation_t rotation)
{
	/* Do nothing, coordinates are rotated as pixels are plotted */
}

static void _ssd1306_display_invert(bool invert)
{
	uint8_t cmd[] = { SSD1306_CMD_INVERT(invert) };
	if ( !m_twi_is_busy )
		write_commands_blocking( cmd, sizeof(cmd) );
}

//################################################################################
//	Initialization & ISR handler
//################################################################################

static void _init_commands(void)
{
	// Initialization commands
	write_commands_blocking( init_commands, sizeof(init_commands) );

	// Clear screen and start first update to LCD
//	_ssd1306_rect_draw( 0, 0, SSD1306_WIDTH, SSD1306_HEIGHT, 0 );
//	_ssd1306_display();
}


#ifdef USE_ISR
typedef enum {
	UPDATE_PHASE_IDLE,
	UPDATE_PHASE_COMMAND,
	UPDATE_PHASE_DATA,
} m_update_phase_t;

static void _twi_handler( nrf_drv_twi_evt_t const * p_event, void * p_context )
{
// Static update parameters
	static m_update_phase_t phase = UPDATE_PHASE_IDLE;
	static uint8_t row = 0;
	static uint8_t * ptr = NULL;
	static uint8_t commands[] = {
		SSD1306_CMD_SET_PAGE(0),
		SSD1306_CMD_COL_LOWER(0),
		SSD1306_CMD_COL_UPPER(0),
	};

	bool repeat = false;
	do {
		// Process start of update
		if ( phase == UPDATE_PHASE_IDLE && m_runUpdate ) {
			row = 0;
			ptr = m_ssd1306.display_buf;
			m_runUpdate = false;
			phase = UPDATE_PHASE_COMMAND;
		}

		// Process update
		if ( phase == UPDATE_PHASE_COMMAND ) {
			commands[3] = row;
			write_commands_start( commands, sizeof(commands) );
			phase = UPDATE_PHASE_DATA;
		} else if ( phase == UPDATE_PHASE_DATA ) {
			write_data_start( ptr, SSD1306_COLS );
			ptr += SSD1306_COLS;
			if ( ++row >= SSD1306_ROWS ) {
				// Update done, exit
				phase = UPDATE_PHASE_IDLE;
			} else {
				phase = UPDATE_PHASE_COMMAND;
			}
		}

		if ( phase == UPDATE_PHASE_IDLE ) {
			// Clear busy flag to notify anything blocking
			m_twi_is_busy = false;
			// Check if update went pending while already updating
			if ( m_runUpdate )
				repeat = true;
		}
	} while ( repeat );
}
#endif

static ret_code_t hardware_init(void)
{
	ret_code_t err_code;

	nrf_drv_twi_config_t twi_config = NRF_DRV_TWI_DEFAULT_CONFIG;
	twi_config.scl = SSD1306_SCL_PIN;
	twi_config.sda = SSD1306_SDA_PIN;

	err_code = nrf_drv_twi_init( &twi_inst, &twi_config, _twi_handler, NULL );
	//err_code = nrf_drv_twi_init( &twi_inst, &twi_config, NULL, NULL );

	return err_code;
}

static ret_code_t _ssd1306_init(void)
{
	ret_code_t err_code = hardware_init();
	if (err_code != NRF_SUCCESS) {
		return err_code;
	}

	_init_commands();

	return err_code;
}

static void _ssd1306_uninit(void)
{
	nrf_drv_twi_uninit(&twi_inst);
}

//################################################################################
//	External interface structure
//################################################################################

const nrf_lcd_t nrf_lcd_ssd1306 = {
	.lcd_init			= _ssd1306_init,
	.lcd_uninit			= _ssd1306_uninit,
	.lcd_pixel_draw		= _ssd1306_pixel_draw,
	.lcd_rect_draw		= _ssd1306_rect_draw,
	.lcd_display		= _ssd1306_display,
	.lcd_rotation_set	= _ssd1306_rotation_set,
	.lcd_display_invert	= _ssd1306_display_invert,
	.p_lcd_cb			= &ssd1306_cb
};

#endif // NRF_MODULE_ENABLED(SSD1306)
