#include "sdk_common.h"
#include "lvgl.h"

typedef struct
{
	lv_obj_t * main_menu;
	lv_obj_t * connecting;

} menus_t;

menus_t menus;


#define BTN_HEIGHT	12
#define BTN_WIDTH	35

static void button_handler(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}
}

static void main_menu_init( void )
{
	if ( menus.main_menu == NULL ) {
		menus.main_menu = lv_obj_create(NULL);

		static lv_style_t style;
		lv_style_init(&style);
		lv_style_set_border_width(&style, 2);
		//lv_style_set_outline_color(&style, lv_palette_main(LV_PALETTE_BLUE));
		//lv_style_set_outline_pad(&style, 8);

		lv_obj_add_style(menus.main_menu, &style, 0);

		static lv_obj_t * btn1;
		btn1 = lv_btn_create( menus.main_menu );
		lv_obj_add_event_cb( btn1, button_handler, LV_EVENT_ALL, NULL );
		lv_obj_set_size( btn1, BTN_WIDTH, BTN_HEIGHT );
		lv_obj_align( btn1, LV_ALIGN_LEFT_MID, 2, 0 );

		static lv_obj_t * lbl1;
		lbl1 = lv_label_create(btn1);
		lv_label_set_text( lbl1, "Test" );
		lv_obj_center(lbl1);

		static lv_obj_t * btn2;
		btn2 = lv_btn_create( menus.main_menu );
		lv_obj_add_event_cb( btn2, button_handler, LV_EVENT_ALL, NULL );
		lv_obj_set_size( btn2, BTN_WIDTH, BTN_HEIGHT );
		lv_obj_align_to( btn2, btn1, LV_ALIGN_OUT_RIGHT_MID, 3, 0 );

		static lv_obj_t * lbl2;
		lbl2 = lv_label_create(btn2);
		lv_label_set_text( lbl2, "Test2" );
		lv_obj_center(lbl2);
	}
}

void menus_init( void )
{
	main_menu_init();
	lv_scr_load( menus.main_menu );
}
