#include "sdk_common.h"

#if NRF_MODULE_ENABLED(ST7565)

#include "nrf_lcd.h"
#include "nrf_drv_spi.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"


#define USE_ISR


#define ST7565_WIDTH	128
#define ST7565_HEIGHT	64

#define ST7565_ROWS		8
#define ST7565_COLS		ST7565_WIDTH

// Control block (managed by nrf_gfx)
static lcd_cb_t st7565_cb = {
	.height = ST7565_HEIGHT,
	.width = ST7565_WIDTH
};

// Entire screen gets divided into an 8x8 grid, where each square can be updated independently
// 8x8 works well because each screen byte covers 8 vertical pixels (1 vertical dirty segment),
// and each byte in the dirty array is 8 bits wide, so each bit represents a different grid segment
#define DIRTY_Y_SEGMENTS	(ST7565_HEIGHT/8)
#define DIRTY_X_SEGMENTS	(ST7565_WIDTH/8)

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
	uint8_t display_buf[ST7565_ROWS * ST7565_WIDTH];	// Each byte is 8 vertical pixels
	uint8_t dirty_flags[DIRTY_Y_SEGMENTS];				// Each 8 rows x
}	st7565_t;

static st7565_t m_st7565;


#ifdef USE_ISR
static volatile bool m_runUpdate = false;
static volatile bool m_spiIsBusy = false;
#endif


// TODO move external to allow sharing? Would also need to use spi manager lib with queue.
static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(ST7565_SPI_INSTANCE);  /**< SPI instance. */

//################################################################################
//	Command list
//################################################################################

#define ST7565_CMD_NOP					( 0b11100011 )

#define ST7565_CMD_SET_ON(x)			( 0b10101110 | (x & 0b00000001) )

#define ST7565_CMD_SET_START_LINE(x)	( 0b01000000 | (x & 0b00111111) )
#define ST7565_CMD_SET_PAGE(x)			( 0b10110000 | (x & 0b00001111) )
#define ST7565_CMD_SET_COL_LOW(x)		( 0b00000000 | (x & 0b00001111) )
#define ST7565_CMD_SET_COL_HIGH(x)		( 0b00010000 | (x & 0b00001111) )
#define ST7565_CMD_SET_COL_REVERSE(x)	( 0b10100000 | (x & 0b00000001) )
#define ST7565_CMD_SET_ROW_REVERSE(x)	( 0b11000000 | ((x<<3) & 0b00001000) )

#define ST7565_CMD_SET_INVERT(x)		( 0b10100110 | (x & 0b00000001) )
#define ST7565_CMD_SET_ALL_ON(x)		( 0b10100100 | (x & 0b00000001) )
#define ST7565_CMD_SET_BIAS(x)			( 0b10100010 | (x & 0b00000001) )
#define ST7565_CMD_RESET				( 0b11100010 )

#define ST7565_CMD_SET_POWER(x)			( 0b00101000 | (x & 0b00000111) )
#define ST7565_CMD_SET_RATIO(x)			( 0b00100000 | (x & 0b00000111) )

#define ST7565_CMD_SET_VOLUME			( 0b10000001 )
#define ST7565_CMD_SET_VOLUME_VAL(x)	( 0b00000000 | (x & 0b00111111) )

#define ST7565_CMD_SET_BOOST			( 0b11111000 )
#define ST7565_CMD_SET_BOOST_VAL(x)		( 0b00000000 | (x & 0b00000011) )
#define BOOST_2X_3X_4X					( 0b00 )
#define BOOST_5X						( 0b01 )
#define BOOST_6X						( 0b11 )


//################################################################################
//	SPI interface
//################################################################################
static inline void spi_write_start( const void * data, size_t size, bool isData )
{
#ifdef USE_ISR
	m_spiIsBusy = true;
#endif
	nrf_gpio_pin_write( ST7565_A0_PIN, isData );
	APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, data, size, NULL, 0));
}

#ifdef USE_ISR
static inline void spi_wait( void )
{
	while ( m_spiIsBusy );
}

static inline void spi_kickstart ( void )
{
	// Kickstart SPI if not already running
	static uint8_t dummy = ST7565_CMD_NOP;
	if ( !m_spiIsBusy )
		spi_write_start( &dummy, 1, 0 );
}

static inline void spi_write_blocking( const void * data, size_t size, bool isData )
{
	spi_wait();
    spi_write_start( data, size, isData );
	spi_wait();
}

//----------------------------------------
// Non-Blocking transfers for update process
//----------------------------------------
static inline void write_command_list_start( uint8_t * p_data, uint16_t len )
{
	spi_write_start( p_data, len, 0 );
}

static inline void write_data_start( uint8_t * p_data, uint16_t len )
{
	spi_write_start( p_data, len, 1 );
}
#else
#define spi_write_blocking	spi_write_start
#endif

//----------------------------------------
// Blocking transfers for normal use
//----------------------------------------

static inline void write_command_list( uint8_t * p_data, uint16_t len )
{
	spi_write_blocking( p_data, len, 0 );
}

static inline void write_command( uint8_t c )
{
	spi_write_blocking( &c, sizeof(c), 0 );
}

static inline void write_data( uint8_t * p_data, uint16_t len )
{
	spi_write_blocking( p_data, len, 1 );
}

static inline void write_byte( uint8_t c )
{
	spi_write_blocking( &c, 1, 1 );
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
	switch ( st7565_cb.rotation )
	{
		case NRF_LCD_ROTATE_0:
			// Do nothing
			break;

		case NRF_LCD_ROTATE_90:
			*x = ST7565_WIDTH - y_old - 1;
			*y = x_old;
			break;

		case NRF_LCD_ROTATE_180:
			*x = ST7565_WIDTH - x_old - 1;
			*y = ST7565_HEIGHT - y_old - 1;
			break;

		case NRF_LCD_ROTATE_270:
			*x = y_old;
			*y = ST7565_HEIGHT - x_old - 1;
			break;
	}
}

uint8_t * 	_SetBufferCursor( uint16_t x, uint16_t y/*, drawMode_t mode*/ )
{
	m_cursor.page = (y>>3);
	m_cursor.offset = (y&0x07);
	m_cursor.col = x;
    m_cursor.p_buf = &m_st7565.display_buf[x + (m_cursor.page*ST7565_WIDTH)];
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
	m_st7565.dirty_flags[m_cursor.page] |= 1<<(m_cursor.col/DIRTY_X_SEGMENTS);	// Set dirty flag
	m_cursor.col++;
}


//################################################################################
//	API functions
//################################################################################

static void _st7565_pixel_draw(uint16_t x, uint16_t y, uint32_t color)
{
	int16_t x1=x, y1=y;
	_rotate( &x1, &y1 );								// Rotate coordinates if needed
	if ( (uint16_t)x1 >= ST7565_WIDTH || (uint16_t)y1 >= ST7565_HEIGHT )	// Only draw if within range
		return;
	_SetBufferCursor( x1, y1 );						// Calculate position
	uint8_t pixelMask = 1<<m_cursor.offset;			// Get y subpixel offset mask
	_WriteRaw( pixelMask, color );					// Write data
}

static void _st7565_rect_draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color)
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
	if ( x1 > ST7565_WIDTH-1 || x2 < 0 || y1 > ST7565_HEIGHT-1 || y2 < 0 )
		return;
// Clip to visible area
	_LIMIT_RANGE( x1, 0, ST7565_WIDTH-1 );
	_LIMIT_RANGE( x2, 0, ST7565_WIDTH-1 );
	_LIMIT_RANGE( y1, 0, ST7565_HEIGHT-1 );
	_LIMIT_RANGE( y2, 0, ST7565_HEIGHT-1 );
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
		ST7565_CMD_SET_PAGE(row),
		ST7565_CMD_SET_COL_LOW(0),
		ST7565_CMD_SET_COL_HIGH(0),
	};
	write_command_list( commands, sizeof(commands) );
}
#endif

static void _st7565_display(void)
{
#ifdef USE_ISR
	CRITICAL_REGION_ENTER();
		m_runUpdate = true;
		spi_kickstart();
	CRITICAL_REGION_EXIT();
#elif 1
// NOTE currently, blasting the whole screen out is faster than updating only the changed parts
// this could be changed to use dirty flags per row, instead of multiple per row
	uint8_t * ptr = m_st7565.display_buf;
	for ( uint8_t row = 0; row < ST7565_ROWS; row++ ) {
		_set_address( row, 0 );
        write_data( ptr, ST7565_COLS );
		ptr += ST7565_COLS;
	}
#else
	// Iterate dirty flags and send only modified blocks
	uint8_t * ptr = m_st7565.display_buf;
	for ( uint16_t row = 0; row < DIRTY_Y_SEGMENTS; row++ ) {
		uint8_t flags = m_st7565.dirty_flags[row];
		m_st7565.dirty_flags[row] = 0;
		uint8_t colMask = 1;
		write_command( ST7565_CMD_SET_PAGE(row) );
		for ( uint16_t col = 0; col < ST7565_WIDTH; col += DIRTY_X_SEGMENTS ) {
			if ( flags & colMask ) {
				write_command( ST7565_CMD_SET_COL_LOW(col) );
				write_command( ST7565_CMD_SET_COL_HIGH(col>>4) );
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

static void _st7565_rotation_set(nrf_lcd_rotation_t rotation)
{
	/* Do nothing, coordinates are rotated as pixels are plotted */
}

static void _st7565_display_invert(bool invert)
{
	write_command( ST7565_CMD_SET_INVERT(invert) );
}

//################################################################################
//	Initialization & ISR handler
//################################################################################

static void _init_commands(void)
{
// Hard reset
	nrf_gpio_pin_clear(ST7565_RESET_PIN);
	nrf_delay_ms(1);
	nrf_gpio_pin_set(ST7565_RESET_PIN);
	nrf_delay_ms(1);

// Initialization commands
	uint8_t commands[] = {
		ST7565_CMD_SET_ON(0),
		ST7565_CMD_SET_BIAS(1),
		ST7565_CMD_SET_RATIO(4),
		ST7565_CMD_SET_VOLUME,
		ST7565_CMD_SET_VOLUME_VAL(35),
		ST7565_CMD_SET_BOOST,
		ST7565_CMD_SET_BOOST_VAL(0),
		ST7565_CMD_SET_POWER(0b111),
		ST7565_CMD_SET_ROW_REVERSE(1),
		ST7565_CMD_SET_COL_REVERSE(0),
		ST7565_CMD_SET_ON(1),
		ST7565_CMD_SET_START_LINE(0),
	};
	write_command_list( commands, sizeof(commands) );
// Clear screen and start first update to LCD
	_st7565_rect_draw( 0, 0, ST7565_WIDTH, ST7565_HEIGHT, 0 );
    _st7565_display();
}


#ifdef USE_ISR
typedef enum {
	UPDATE_PHASE_IDLE,
	UPDATE_PHASE_COMMAND,
	UPDATE_PHASE_DATA,
} m_update_phase_t;

static void _spi_handler( nrf_drv_spi_evt_t const * p_event, void * p_context )
{
//	TP1_ON();

// Static update parameters
	static m_update_phase_t phase = UPDATE_PHASE_IDLE;
	static uint8_t row = 0;
	static uint8_t * ptr = NULL;
	static uint8_t commands[] = {
		ST7565_CMD_SET_PAGE(0),
		ST7565_CMD_SET_COL_LOW(0),
		ST7565_CMD_SET_COL_HIGH(0),
	};

	bool repeat = false;
	do {
		// Process start of update
		if ( phase == UPDATE_PHASE_IDLE && m_runUpdate ) {
			row = 0;
			ptr = m_st7565.display_buf;
			m_runUpdate = false;
			phase = UPDATE_PHASE_COMMAND;
		}

		// Process update
		if ( phase == UPDATE_PHASE_COMMAND ) {
			commands[0] = ST7565_CMD_SET_PAGE(row);
			write_command_list_start( commands, sizeof(commands) );
			phase = UPDATE_PHASE_DATA;
		} else if ( phase == UPDATE_PHASE_DATA ) {
			write_data_start( ptr, ST7565_COLS );
			ptr += ST7565_COLS;
			if ( ++row >= ST7565_ROWS ) {
				// Update done, exit
				phase = UPDATE_PHASE_IDLE;
			} else {
				phase = UPDATE_PHASE_COMMAND;
			}
		}

		if ( phase == UPDATE_PHASE_IDLE ) {
			// Clear busy flag to notify anything blocking
			m_spiIsBusy = false;
			// Check if update went pending while already updating
			if ( m_runUpdate )
				repeat = true;
		}
	} while ( repeat );

//   	TP1_OFF();
}
#endif

static ret_code_t hardware_init(void)
{
	ret_code_t err_code;

	nrf_gpio_cfg_output(ST7565_A0_PIN);
	nrf_gpio_cfg_output(ST7565_RESET_PIN);

	nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;

	spi_config.sck_pin  = ST7565_SCK_PIN;
	spi_config.miso_pin = NRFX_SPI_PIN_NOT_USED;
	spi_config.mosi_pin = ST7565_MOSI_PIN;
	spi_config.ss_pin   = ST7565_SS_PIN;
	spi_config.frequency = NRF_SPI_FREQ_4M;

#ifdef USE_ISR
	err_code = nrf_drv_spi_init( &spi, &spi_config, _spi_handler, NULL );
#else
	err_code = nrf_drv_spi_init( &spi, &spi_config, NULL, NULL );
#endif
	return err_code;
}

static ret_code_t _st7565_init(void)
{
	ret_code_t err_code = hardware_init();
	if (err_code != NRF_SUCCESS) {
		return err_code;
	}

	_init_commands();

	return err_code;
}

static void _st7565_uninit(void)
{
	nrf_drv_spi_uninit(&spi);
}

//################################################################################
//	External interface structure
//################################################################################

const nrf_lcd_t nrf_lcd_st7565 = {
    .lcd_init			= _st7565_init,
    .lcd_uninit			= _st7565_uninit,
    .lcd_pixel_draw		= _st7565_pixel_draw,
    .lcd_rect_draw		= _st7565_rect_draw,
    .lcd_display		= _st7565_display,
    .lcd_rotation_set	= _st7565_rotation_set,
    .lcd_display_invert	= _st7565_display_invert,
    .p_lcd_cb			= &st7565_cb
};

#endif // NRF_MODULE_ENABLED(ST7565)
