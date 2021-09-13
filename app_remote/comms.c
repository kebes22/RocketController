#include "comms.h"
#include "motor_driver.h"
#include "UI.h"

#define NRF_LOG_MODULE_NAME txrx
#ifdef TXRX_COMMS_LOG_LEVEL
#define NRF_LOG_LEVEL TXRX_COMMS_LOG_LEVEL
#else
#define NRF_LOG_LEVEL 1
#endif
#include "log.h"

//#define COMMS_ECHO	1



TXRX_BLE_DEF(txrx);

//################################################################################
//	Comms packet parsing
//################################################################################
static void _parse_rx_packet( comms_packet_t * packet )
{
	
	switch ( packet->hdr.cmd ) {

//		case CMD_RC_CHANNELS_0:
//			motor_run( packet->payload.rc.ch[0], packet->payload.rc.ch[1] );
//			break;

		case CMD_RC_CHANNELS_0:
			{
				uint16_t ch0 = packet->payload.rc.ch[0];
				uint16_t ch1 = packet->payload.rc.ch[1];
				if ( ch0 > 500 && ch1 > 500 ) {
					ui_set_motors( (ch0 - 1500) * 2, (ch1 - 1500) * 2 );
				} else {
					ui_set_motors( 0, 0 );
				}
			}
			break;

		case CMD_SET_COLOR:
			ui_set_color( packet->payload.color );
			break;

		case CMD_LAUNCH_DFU:
			ui_launch_dfu();
			break;
	}
}

//################################################################################
//	Event handling
//################################################################################

static void txrx_ble_event_handler( txrx_ble_evt_t *p_evt )
{
#if 0
	txrx_trans_t * p_trans = (txrx_trans_t*)p_evt->p_context;

	txrx_trans_evt_t evt;
	evt.p_trans = p_trans;
	evt.p_context = p_trans->p_context;
	evt.type = p_evt->type; // Default to same event type as current event

	switch ( p_evt->type ) {
		CASE_TXRX_EVT(TXRX_EVT_RX_DATA, p_trans, _GET_CONNECTION(p_trans))
			LOG_HEXDUMP_TRACE(p_evt->params.rx_data.p_data, p_evt->params.rx_data.length);
			_handle_rx( p_trans, (uint8_t *)p_evt->params.rx_data.p_data, p_evt->params.rx_data.length );
			break;

		CASE_TXRX_EVT(TXRX_EVT_TX_DONE, p_trans, _GET_CONNECTION(p_trans))
		{
			LOG_TRACE(
				"We have buffers and lengths %d, %d, sum %d",
				p_trans->tx.bufs.buf1.length, p_trans->tx.bufs.buf2.length,
				p_trans->tx.bufs.buf1.length + p_trans->tx.bufs.buf2.length
			);
			if (p_trans->tx.stop) {
				LOG_TRACE("Reset and trigger");
				_reset_tx(p_trans);

				// Tell the higher layer we've finished its packet
				evt.type = TXRX_EVT_TX_DONE;
				p_trans->event_handler(&evt);
			} else if (p_trans->tx.bufs.buf1.length + p_trans->tx.bufs.buf2.length) {
				// And we're not done yet
				LOG_TRACE("Send next packet");
				_process_tx(p_trans);
			}
			break;
		}

		CASE_TXRX_EVT(TXRX_EVT_CONNECTED, p_trans, _GET_CONNECTION(p_trans))
			_reset_all(p_trans);
			p_trans->event_handler(&evt);
			break;

		CASE_TXRX_EVT(TXRX_EVT_COMM_STARTED, p_trans, _GET_CONNECTION(p_trans))
			p_trans->event_handler(&evt);
			break;

		CASE_TXRX_EVT(TXRX_EVT_COMM_STOPPED, p_trans, _GET_CONNECTION(p_trans))
			p_trans->event_handler(&evt);
			break;

		CASE_TXRX_EVT(TXRX_EVT_DISCONNECTED, p_trans, _GET_CONNECTION(p_trans))
			_reset_all(p_trans);
			p_trans->event_handler(&evt);
			break;

		default:
			NRF_LOG_ERROR("Invalid event: %d", p_evt->type);
	}
#else
	switch ( p_evt->type ) {
		case TXRX_EVT_RX_DATA:
			NRF_LOG_DEBUG( "RX Data:" );
			NRF_LOG_HEXDUMP_DEBUG( p_evt->params.rx_data.p_data, p_evt->params.rx_data.length );
#if COMMS_ECHO
			txrx_ble_send( &txrx, p_evt->params.rx_data.p_data, p_evt->params.rx_data.length );
#endif
			_parse_rx_packet( (comms_packet_t*)p_evt->params.rx_data.p_data );
			break;

		case TXRX_EVT_TX_DONE:
			NRF_LOG_DEBUG( "TX Done" );
			LOG_TRACE(
				"We have buffers and lengths %d, %d, sum %d",
				p_trans->tx.bufs.buf1.length, p_trans->tx.bufs.buf2.length,
				p_trans->tx.bufs.buf1.length + p_trans->tx.bufs.buf2.length
			);
			break;

		case TXRX_EVT_CONNECTED:
			NRF_LOG_DEBUG( "Connected" );
			ui_update_connection( true );
			break;

		case TXRX_EVT_COMM_STARTED:
			NRF_LOG_DEBUG( "Comms Started" );
			break;

		case TXRX_EVT_COMM_STOPPED:
			NRF_LOG_DEBUG( "Comms Stopped" );
			break;

		case TXRX_EVT_DISCONNECTED:
			NRF_LOG_DEBUG( "Disconnected" );
			ui_update_connection( false );
			break;

		default:
			NRF_LOG_ERROR("Invalid event: %d", p_evt->type);
	}
#endif
}


//void comms_init( txrx_trans_t * p_trans, txrx_trans_init_t const * init )
void comms_init( void )
{
//	ASSERT(p_trans != NULL);
//	if (p_trans->type != TXRX_INST_CENTRAL) {
//		ASSERT(p_trans->p_txrx_ble != NULL);
//	}
//	ASSERT(p_trans->p_rxBuffer != NULL);
//	ASSERT(p_trans->rxBufLen > 0);
//	ASSERT(init != NULL);
//	ASSERT(init->event_handler != NULL);

	// Initialize lower bluetooth service layer
	txrx_ble_init_t txrx_init = {
		.data_handler	= txrx_ble_event_handler,
		.p_context		= NULL
	};

	uint32_t err_code = txrx_ble_init( &txrx, &txrx_init);
	APP_ERROR_CHECK(err_code);

//	// Initialize Transport layer
//	_reset_all(p_trans);
//	p_trans->event_handler	= init->event_handler;
//	p_trans->p_context		= init->p_context;
}