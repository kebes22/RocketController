#include "motor_driver.h"
#include "drv883x.h"
#include "io_config.h"



drv883x_t motor1;
drv883x_t motor2;
drv883x_t motor3;


void	motor_run( int16_t m1, int16_t m2 )
{
	drv883x_set_duty( &motor1, m1 );
	drv883x_set_duty( &motor2, m2 );
}



static bool rampOn = false;

void	motor_ramp_test_set_on( bool on )
{
	rampOn = on;
	if ( !on ) {
    	drv883x_set_duty( &motor1, 0 );
		drv883x_set_duty( &motor2, 0 );
	}
}

void	motor_ramp_test( void )
{
	static int16_t duty = 0;
	static int16_t inc = 10;

if ( rampOn ) {
	duty += inc;
	if ( duty > DRV883X_DUTY_MAX ) {
		duty = DRV883X_DUTY_MAX;
		inc = -inc;
	}
	if ( duty < DRV883X_DUTY_MIN ) {
		duty = DRV883X_DUTY_MIN;
		inc = -inc;
	}

	drv883x_set_duty( &motor1, duty );
	drv883x_set_duty( &motor2, duty );
//	drv883x_set_duty( &motor2, 0 );

} else {
	duty = 0;
	inc = 10;
}


}

void	motor_init( void )
{
// Configure motor 1
	drv883x_config_t m1_config = {
		.type			= DRV8837,
		.sleep_pin		= MOTOR_1_SLEEP_PIN,
		.in1_ph_pin		= MOTOR_1_EN_P1_PIN,
		.in2_en_pin		= MOTOR_1_PH_P2_PIN,
		.mode			= DRV883X_MODE_BRAKE,
//		.mode			= DRV883X_MODE_COAST
	};
	drv883x_register( &motor1, &m1_config );

// Configure motor 2
	drv883x_config_t m2_config = {
		.type			= DRV8837,
		.sleep_pin		= MOTOR_2_SLEEP_PIN,
		.in1_ph_pin		= MOTOR_2_EN_P1_PIN,
		.in2_en_pin		= MOTOR_2_PH_P2_PIN,
		.mode			= DRV883X_MODE_BRAKE
//		.mode			= DRV883X_MODE_COAST
	};
	drv883x_register( &motor2, &m2_config );

//// Configure motor 3
//	drv883x_config_t m3_config = {
//		.type			= DRV8837,
//		.sleep_pin		= MOTOR_3_SLEEP_PIN,
//		.in1_ph_pin		= MOTOR_3_EN_P1_PIN,
//		.in2_en_pin		= MOTOR_3_PH_P2_PIN,
//		.mode			= DRV883X_MODE_BRAKE
//	};
//	drv883x_register( &motor3, &m3_config );

    drv883x_init();

	drv883x_set_enable( &motor1, true );
	drv883x_set_enable( &motor2, true );
}
