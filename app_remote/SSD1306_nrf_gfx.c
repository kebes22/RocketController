#include "sdk_common.h"
#include "Hardware.h"

#if NRF_MODULE_ENABLED(SSD1306)

#include "nrf_lcd.h"
//#include "nrf_drv_spi.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"

//#include "nrf_drv_twi.h"
#include "nrf_twi_mngr.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"


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
	uint8_t display_buf[SSD1306_ROWS][SSD1306_WIDTH+1];	// Each byte is 8 vertical pixels
	//uint8_t dirty_flags[DIRTY_Y_SEGMENTS];				// Each 8 rows x

	// TODO add flags/state tracking for update/brightness/etc.
	volatile bool	runUpdate;
	volatile bool	twi_is_busy;

}	ssd1306_t;

static ssd1306_t m_ssd1306;

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

//################################################################################
//	Init/uninit commands
//################################################################################
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

static uint8_t uninit_commands[] = {
	SSD1306_CMD_DISP_ON(0),
};

static uint8_t display_commands[] = {
	SSD1306_CMD_ADDR_MODE(0),		// Continuous horizontal addressing with wrap
	SSD1306_CMD_SET_PAGE(0),		// Row 0
	SSD1306_CMD_COL_UPPER(0),		// Col 0
	SSD1306_CMD_COL_LOWER(0),		// Col 0
};

//################################################################################
//	TWI interface
//################################################################################
static SemaphoreHandle_t	m_twi_semaphore = NULL;
static SemaphoreHandle_t	m_twi_mutex = NULL;
static volatile ret_code_t	m_twi_result = 0;

#define COMMS_TIMEOUT 100

/**
 * @brief Callback for I2C writes
 *
 * This function is called from interrupt level.
 */
static void twi_cb(ret_code_t result, void * p_user_data)
{
	//if (result != NRF_SUCCESS) {
	//	NRF_LOG_ERROR("LSM303AGR TWI Error: %d", (int)result);
	//}
	m_twi_result = result;
	BaseType_t yield_req = pdFALSE;
	UNUSED_VARIABLE(xSemaphoreGiveFromISR(m_twi_semaphore, &yield_req));
	portYIELD_FROM_ISR(yield_req);
}

// Take back semaphore if last transaction timed out.
static void _transaction_clear( void )
{
//	Try to take semaphore, in case given late from last transaction
	UNUSED_RETURN_VALUE( xSemaphoreTake( m_twi_semaphore, 0 ) );
}

// Wait for TWI transaction to complete.
static ret_code_t _transaction_wait( const nrf_twi_mngr_t* p_twi )
{
	//	Wait for completion
	if ( pdFALSE == xSemaphoreTake( m_twi_semaphore, COMMS_TIMEOUT ) ) {
//		NRF_LOG_ERROR("LSM303AGR TWI driver timeout" );
		//uint8_t trash[p_twi->p_queue->element_size];
//		nrf_queue_pop(p_twi->p_queue, trash);
		m_twi_result = NRF_ERROR_IO_PENDING;
	}
	return m_twi_result;
}

static ret_code_t _guard_start( void )
{
	if ( pdFALSE == xSemaphoreTake( m_twi_mutex, COMMS_TIMEOUT ) ) {
		return NRF_ERROR_BUSY;
	}
	return NRF_SUCCESS;
}

static void _guard_end( void )
{
	xSemaphoreGive( m_twi_mutex );
}

//################################################################################
//	Write functions
//################################################################################

static ret_code_t _i2c_write( uint8_t* p_data, uint16_t len, bool is_data, bool wait )
{
	ret_code_t error = _guard_start();
	if ( error ) return error;

	uint8_t control[] = { CONT_DATA };
	nrf_twi_mngr_transfer_t const cmd_transfers[] =
	{
		NRF_TWI_MNGR_WRITE(SSD1306_TWI_ADDR, p_data, len, false),
	};
	nrf_twi_mngr_transfer_t const data_transfers[] =
	{
		NRF_TWI_MNGR_WRITE(SSD1306_TWI_ADDR, control, 1, NRF_TWI_MNGR_NO_STOP),
		NRF_TWI_MNGR_WRITE(SSD1306_TWI_ADDR, p_data, len, 0),
	};
	nrf_twi_mngr_transaction_t const transaction =
	{
		.callback				= twi_cb,
		.p_user_data			= NULL,
		.p_transfers			= is_data? data_transfers: cmd_transfers,
		.number_of_transfers	= is_data? 2: 1,
	};
	_transaction_clear();
	error = nrf_twi_mngr_schedule(hardware.drivers.p_twi_0, &transaction);
	APP_ERROR_CHECK(error);
	error = _transaction_wait(hardware.drivers.p_twi_0);
	//if(error){
	//	NRF_LOG_ERROR("%s LSM303AGR TWI driver timeout on reg 0x%02x", (uint32_t)__func__, reg );}

	_guard_end();
	return error;
}

//################################################################################
//	Display update code
//################################################################################
typedef enum {
	UPDATE_PHASE_COMMAND,
	UPDATE_PHASE_DATA,
} m_update_phase_t;

// State machine run from the twi callback (isr)
static void _display_isr_state_machine(ret_code_t result, void * p_user_data)
{
	// Static update parameters
	static m_update_phase_t phase = UPDATE_PHASE_COMMAND;
	static uint8_t row = 0;

	bool repeat = false;
	do {
		// Check for update flag, and start process
		if ( phase == UPDATE_PHASE_COMMAND && m_ssd1306.runUpdate ) {
			// Send command list to prepare for data
			m_ssd1306.runUpdate = false;
			row = 0;
			static nrf_twi_mngr_transfer_t const commands_transfers[] =
			{
				NRF_TWI_MNGR_WRITE(SSD1306_TWI_ADDR, display_commands, sizeof(display_commands), 0),
			};
			static nrf_twi_mngr_transaction_t const commands_transaction =
			{
				.callback				= _display_isr_state_machine,
				.p_user_data			= NULL,
				.p_transfers			= commands_transfers,
				.number_of_transfers	= ARRAY_SIZE(commands_transfers)
			};
			nrf_twi_mngr_schedule(hardware.drivers.p_twi_0, &commands_transaction);
			phase = UPDATE_PHASE_DATA;
			return;
		} else if ( phase == UPDATE_PHASE_DATA ) {
			if ( row < SSD1306_ROWS ) {
				static nrf_twi_mngr_transfer_t display_transfers[] =
				{
					NRF_TWI_MNGR_WRITE(SSD1306_TWI_ADDR, m_ssd1306.display_buf[0], sizeof(m_ssd1306.display_buf[0]), 0),
				};
				display_transfers[0].p_data = m_ssd1306.display_buf[row++]; // Update pointer based on row
				display_transfers[0].p_data[0] = CONT_DATA;					// Make sure it is flagged as data
				static nrf_twi_mngr_transaction_t const display_transaction =
				{
					.callback				= _display_isr_state_machine,
					.p_user_data			= NULL,
					.p_transfers			= display_transfers,
					.number_of_transfers	= ARRAY_SIZE(display_transfers)
				};
				nrf_twi_mngr_schedule(hardware.drivers.p_twi_0, &display_transaction);
				return;
			} else { // Done with rows
				phase = UPDATE_PHASE_COMMAND;
				if ( m_ssd1306.runUpdate )
					repeat = true;
				else
					m_ssd1306.twi_is_busy = false;
			}
		}

	} while ( repeat );
}

// Kickstart transfer using NOP if not already running a transfer
static void twi_kickstart( void )
{
	if ( m_ssd1306.twi_is_busy )
		return;

	m_ssd1306.twi_is_busy = true;
	static uint8_t commands[] = {
		SSD1306_CMD_NOP,
	};
	static nrf_twi_mngr_transfer_t const transfers[] =
	{
		NRF_TWI_MNGR_WRITE(SSD1306_TWI_ADDR, commands, sizeof(commands), 0),
	};
	static nrf_twi_mngr_transaction_t const transaction =
	{
		.callback				= _display_isr_state_machine,
		.p_user_data			= NULL,
		.p_transfers			= transfers,
		.number_of_transfers	= ARRAY_SIZE(transfers)
	};
	nrf_twi_mngr_schedule(hardware.drivers.p_twi_0, &transaction);
}

//----------------------------------------
// Blocking transfers for normal use
//----------------------------------------

static inline ret_code_t write_commands_blocking( uint8_t * p_data, uint16_t len )
{
	return _i2c_write( p_data, len, false, true );
}

//static inline ret_code_t write_data_blocking( uint8_t * p_data, uint16_t len )
//{
//	return _i2c_write( p_data, len, true, true );
//}


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

static void * 	_SetBufferCursor( uint16_t x, uint16_t y )
{
	m_cursor.page = (y>>3);
	m_cursor.offset = (y&0x07);
	m_cursor.col = x;
	m_cursor.p_buf = &m_ssd1306.display_buf[m_cursor.page][x+1]; // +1 is for I2C cotrol byte at beginning of each page
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
	//m_ssd1306.dirty_flags[m_cursor.page] |= 1<<(m_cursor.col/DIRTY_X_SEGMENTS);	// Set dirty flag
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

static void _ssd1306_display(void)
{
	CRITICAL_REGION_ENTER();
		m_ssd1306.runUpdate = true;
		twi_kickstart();
	CRITICAL_REGION_EXIT();
}

static void _ssd1306_rotation_set(nrf_lcd_rotation_t rotation)
{
	/* Do nothing, coordinates are rotated as pixels are plotted */
}

static void _ssd1306_display_invert(bool invert)
{
	// FIXME
	//uint8_t cmd[] = { SSD1306_CMD_INVERT(invert) };
	//if ( !m_twi_is_busy )
	//	write_commands_blocking( cmd, sizeof(cmd) );
}

//################################################################################
//	Initialization
//################################################################################

static void _load_pattern( void )
{
	uint8_t val = 0;
	for ( int r=0; r<SSD1306_ROWS; r++ ) {
		for ( int c=0; c<SSD1306_COLS; c++ )
			m_ssd1306.display_buf[r][c+1] = val++;
	}
}

static ret_code_t _ssd1306_init(void)
{
	if ( !m_twi_semaphore )
		m_twi_semaphore = xSemaphoreCreateBinary();
	if ( !m_twi_mutex )
		m_twi_mutex = xSemaphoreCreateMutex();

	ret_code_t error = write_commands_blocking( init_commands, sizeof(init_commands) );
	if ( !error ) {
		_load_pattern();
		_ssd1306_display();
	}
	return error;
}

static void _ssd1306_uninit(void)
{
	write_commands_blocking( init_commands, sizeof(uninit_commands) );
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
