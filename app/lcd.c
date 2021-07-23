#include "sdk_common.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

#include "nrf_gfx.h"

#define NRF_LOG_MODULE_NAME LCD
#ifdef LCD_LOG_LEVEL
#define	NRF_LOG_LEVEL LCD_LOG_LEVEL
#else
#define	NRF_LOG_LEVEL 1
#endif
#include "log.h"

#if 0
#define LCD_UPDATE_INTERVAL 50
#define LCD_USE_THREAD		0

static const char * test_text = "the quick brown fox jumps over the lazy dog. THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG";

extern const nrf_lcd_t nrf_lcd_st7565;
static const nrf_lcd_t * p_lcd = &nrf_lcd_st7565;

//extern const nrf_gfx_font_desc_t orkney_24ptFontInfo;
//static const nrf_gfx_font_desc_t * p_font = &orkney_24ptFontInfo;

//extern const nrf_gfx_font_desc_t orkney_8ptFontInfo;
//static const nrf_gfx_font_desc_t * p_font = &orkney_8ptFontInfo;

extern const nrf_gfx_font_desc_t adobeGothicStdB_6ptFontInfo;
static const nrf_gfx_font_desc_t * p_font = &adobeGothicStdB_6ptFontInfo;

static TaskHandle_t m_lcd_thread;
static TimerHandle_t m_lcd_timer;


#if LCD_USE_THREAD
static void lcd_thread( void * arg )
{
	UNUSED_PARAMETER(arg);
	NRF_LOG_INFO("Thread Started: %s", pcTaskGetName(NULL));

	const TickType_t xFrequency = LCD_UPDATE_INTERVAL;
	TickType_t xLastUpdateTime = xTaskGetTickCount();

	while ( 1 )
	{
#if 1
		vTaskDelayUntil( &xLastUpdateTime, xFrequency );
#else
        vTaskDelay( xFrequency );
#endif
		p_lcd->lcd_display();
	}
}

#else

static const char * spinner = "|/-\\";
static uint8_t spinnerIdx = 0;
static void _lcd_timer_handler( TimerHandle_t xTimer )
{
	static char sbuf[] = { 0, 0 };
	sbuf[0] = spinner[spinnerIdx++];
	if ( spinnerIdx >= sizeof(spinner) )
		spinnerIdx = 0;
	
	nrf_gfx_rect_t rect = NRF_GFX_RECT(0,0,5,10);
	nrf_gfx_rect_draw( p_lcd, &rect, 0, 0, true );
	nrf_gfx_point_t iconPoint = NRF_GFX_POINT(0, 0);
	APP_ERROR_CHECK( nrf_gfx_print(p_lcd, &iconPoint, 1, sbuf, p_font, false) );

	p_lcd->lcd_display();
}

#endif

void lcd_init( void )
{
#if LCD_USE_THREAD
	// Create LCD update thread
	if ( pdPASS != xTaskCreate(lcd_thread, "LCD", 128, NULL, 2, &m_lcd_thread) ) {
		APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
	}
#else
	m_lcd_timer = xTimerCreate( "LCD", LCD_UPDATE_INTERVAL, pdTRUE, NULL, _lcd_timer_handler );
	if ( m_lcd_timer == NULL ) {
		APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
	}
    xTimerStart( m_lcd_timer, 0 );
#endif

	APP_ERROR_CHECK( nrf_gfx_init(p_lcd) );
	nrf_gfx_rotation_set(p_lcd, NRF_LCD_ROTATE_180);
//	nrf_gfx_invert(p_lcd, true);

	nrf_gfx_point_t text_start = NRF_GFX_POINT(0, 10);
	APP_ERROR_CHECK( nrf_gfx_print(p_lcd, &text_start, 1, test_text, p_font, true) );
}

//#else
void lcd_init( void )
{
	GLCD_Init();
}
#endif

