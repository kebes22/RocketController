#ifndef COMMS_H__
#define COMMS_H__

#include "txrx_ble.h"
#include "RGB.h"


#ifdef __cplusplus
extern "C" {
#endif

#define TXRX_COMMS_MAX_DATA (TXRX_BLE_MAX_DATA_LEN - sizeof(comms_header_t))

//################################################################################
//	Comms packet commands
//################################################################################
typedef enum {
	CMD_PWR_OFF			= 0xFF,

	CMD_BUTTON_STATE	= 0xB0,

	CMD_RC_CHANNELS_0	= 0xC0,
	CMD_RC_CHANNELS_1	= 0xC1,

	CMD_SET_COLOR		= 0xD0,

	CMD_LAUNCH_DFU		= 0xDF,
} comms_cmd_t;

//################################################################################
//	Comms packet payloads
//################################################################################
typedef struct {
	int16_t		ch[8];
	uint16_t	cksm;
} comms_payload_channels_t;

//typedef struct {
//	uint8_t		r;
//	uint8_t		g;
//	uint8_t		b;
//} comms_payload_color_t;

typedef struct {
	uint8_t		state;
} comms_payload_button_t;


//################################################################################
//	Comms packet structures
//################################################################################

typedef struct __PACKED {
	comms_cmd_t		cmd;
	uint8_t			len;
} comms_header_t;

typedef struct {
	comms_header_t					hdr;
	union {
		uint8_t						raw[TXRX_COMMS_MAX_DATA];
		comms_payload_channels_t	rc;
        RGB_Color_t					color;

		comms_payload_button_t		button;
	} payload;
} comms_packet_t;



//################################################################################
//	Comms api
//################################################################################

void	comms_init( void );


void	comms_send_button( uint8_t state );


#ifdef __cplusplus
}
#endif

#endif // COMMS_H__
