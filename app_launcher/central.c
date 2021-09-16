
//#include "app_error.h"
//#include <stdbool.h>
//#include <stdint.h>
//#include <stdio.h>
//#include "app_uart.h"
//#include "app_timer.h"
//#include "app_util.h"
//#include "bsp_btn_ble.h"
//#include "ble.h"
//#include "ble_gap.h"
//#include "ble_hci.h"
//#include "nrf_sdh.h"
//#include "nrf_sdh_ble.h"
//#include "nrf_sdh_soc.h"

#include "sdk_common.h"
#include "nordic_common.h"

#define NRF_LOG_MODULE_NAME central
#ifdef CENTRAL_LOG_LEVEL
#define NRF_LOG_LEVEL CENTRAL_LOG_LEVEL
#else
#define NRF_LOG_LEVEL 2
#endif
#include "log.h"

#include "nrf_ble_scan.h"
#include "ble_db_discovery.h"

#include "central.h"
#include "txrx_ble.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "comms.h"

#include "UI.h"

#define APP_BLE_CONN_CFG_TAG 1  /**< Tag that refers to the BLE stack configuration set with @ref sd_ble_cfg_set. The default tag is @ref BLE_CONN_CFG_TAG_DEFAULT. */

BLE_DB_DISCOVERY_DEF(m_db_disc); /**< Database discovery module instance. */
NRF_BLE_SCAN_DEF(m_scan);		 /**< Scanning Module instance. */

TXRX_BLE_DEF(m_txrx);
TXRX_BLE_CENTRAL_DEF(m_txrx_central);


//################################################################################
//	Comms handlers
//################################################################################

static void _parse_rx_packet( comms_packet_t * packet )
{

	switch ( packet->hdr.cmd ) {

		case CMD_BUTTON_STATE:
			UI_Fire();
			break;

		case CMD_LAUNCH_DFU:
			//ui_launch_dfu();
			break;
	}
}


void send_channels( uint16_t * channels, uint8_t count )
{
	static comms_packet_t packet;
	packet.hdr.cmd = CMD_RC_CHANNELS_0;
	packet.hdr.len = count * 2;
	uint8_t i = 0;
	for ( i=0; i<count; i++ ) {
		packet.payload.rc.ch[i] = channels[i];
	}

    txrx_ble_send( &m_txrx_central, (uint8_t*)&packet, sizeof(packet.hdr) + packet.hdr.len );
}

uint32_t	send_color( uint8_t * color )
{
	uint8_t msg[] = { 0xD0, 0x03, color[0], color[1], color[2] };
    return txrx_ble_send( &m_txrx_central, msg, sizeof(msg) );
}

//################################################################################
//	Event trigger
//################################################################################

//#define _TRIGGER_EVENT(from, evt_type, ...) do {	\
//	txrx_ble_evt_t evt;								\
//	evt.p_txrx = from;								\
//	evt.p_context = from->p_context;				\
//	evt.type = evt_type;							\
//	__VA_ARGS__										\
//	from->data_handler(&evt);						\
//} while(0)

//################################################################################
//	Scanner
//################################################################################

/**@brief TXRX UUID. */
static ble_uuid_t const m_txrx_uuid =
{
	.uuid = BLE_UUID_TXRX_SERVICE,
	.type = BLE_UUID_TYPE_VENDOR_BEGIN
};

/**@brief Function for starting scanning. */
static void scan_start(void) {
	ret_code_t ret;
	ret = nrf_ble_scan_start(&m_scan);
	APP_ERROR_CHECK(ret);
}

/**@brief Function for handling Scanning Module events.
 */
static void scan_evt_handler(scan_evt_t const *p_scan_evt) {
	ret_code_t err_code;

	switch (p_scan_evt->scan_evt_id) {
		case NRF_BLE_SCAN_EVT_CONNECTING_ERROR: {
			err_code = p_scan_evt->params.connecting_err.err_code;
			APP_ERROR_CHECK(err_code);
		} break;

		case NRF_BLE_SCAN_EVT_CONNECTED: {
			ble_gap_evt_connected_t const *p_connected =
				p_scan_evt->params.connected.p_connected;
			// Scan is automatically stopped by the connection.
			NRF_LOG_INFO("Connecting to target %02x:%02x:%02x:%02x:%02x:%02x",
				p_connected->peer_addr.addr[5],
				p_connected->peer_addr.addr[4],
				p_connected->peer_addr.addr[3],
				p_connected->peer_addr.addr[2],
				p_connected->peer_addr.addr[1],
				p_connected->peer_addr.addr[0]);
		} break;

		case NRF_BLE_SCAN_EVT_SCAN_TIMEOUT: {
			NRF_LOG_INFO("Scan timed out.");
			scan_start();
		} break;

		default:
			break;
	}
}

/**@brief Function for initializing the scanning and setting the filters.
 */
static void scan_init(void) {
	ret_code_t err_code;
	nrf_ble_scan_init_t init_scan;

	memset(&init_scan, 0, sizeof(init_scan));

	init_scan.connect_if_match = true;
	init_scan.conn_cfg_tag = APP_BLE_CONN_CFG_TAG;

	err_code = nrf_ble_scan_init(&m_scan, &init_scan, scan_evt_handler);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_ble_scan_filter_set(&m_scan, SCAN_UUID_FILTER, &m_txrx_uuid);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_ble_scan_filters_enable(&m_scan, NRF_BLE_SCAN_UUID_FILTER, false);
	APP_ERROR_CHECK(err_code);
}

//################################################################################
//	Central event handler
//################################################################################

// enable/disable notifications
static uint32_t _cccd_configure(uint16_t conn_handle, uint16_t handle_cccd, bool enable)
{
	static uint8_t m_buf[BLE_CCCD_VALUE_LEN];
	m_buf[0] = enable ? BLE_GATT_HVX_NOTIFICATION : 0;
	m_buf[1] = 0;

	const ble_gattc_write_params_t write_params = {
		.write_op = BLE_GATT_OP_WRITE_REQ,
		.handle   = handle_cccd,
		.flags    = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE,
		.offset   = 0,
		.len      = BLE_CCCD_VALUE_LEN,
		.p_value  = m_buf
	};
	return sd_ble_gattc_write(conn_handle, &write_params);
}


static void _on_connected(txrx_ble_t *p_txrx, ble_gap_evt_t const * p_gap_evt)
{
	ret_code_t err_code;
	uint16_t conn_handle = p_gap_evt->conn_handle;
	uint8_t const *addr = p_gap_evt->params.connected.peer_addr.addr;

	NRF_LOG_INFO(
		"Connected to %02x:%02x:%02x:%02x:%02x:%02x",
		addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]
	);

	// Start discovery of services.
	err_code = ble_db_discovery_start( &m_db_disc, m_txrx_central.conn_handle );
	APP_ERROR_CHECK(err_code);
}

static void _on_disconnected(txrx_ble_t *p_txrx, ble_gap_evt_t const * p_gap_evt)
{
	NRF_LOG_WARNING ( "Disconnected" );

	// @see https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.s132.api.v6.0.0%2Fgroup___b_l_e___h_c_i___s_t_a_t_u_s___c_o_d_e_s.html
	switch(p_gap_evt->params.disconnected.reason) {

		case BLE_HCI_LOCAL_HOST_TERMINATED_CONNECTION:
//			NRF_LOG_INFO(
//				"Disconnected from %02x:%02x:%02x:%02x:%02x device: %d",
//				addr[5], addr[4], addr[3], addr[2], addr[1], device
//			);
			break;

		case BLE_HCI_CONN_FAILED_TO_BE_ESTABLISHED:
//			// We attempted to connect based on an advertising packet, but we
//			// didn't get a reply with a timeout interval, so try again.
//			// TODO limit retries.
//			retry = true;
//
//			NRF_LOG_WARNING(
//				"Connection attempt timed out for %02x:%02x:%02x:%02x:%02x:%02x, retrying",
//				addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]
//			);
			break;

		case BLE_HCI_CONNECTION_TIMEOUT:
//			retry = true;
//			NRF_LOG_WARNING(
//				"Disconnected (timed out) from %02x:%02x:%02x:%02x:%02x:%02x",
//				addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]
//			);
			break;

		case BLE_HCI_STATUS_CODE_LMP_RESPONSE_TIMEOUT:
//			NRF_LOG_WARNING(
//				"LMP response time out from %02x:%02x:%02x:%02x:%02x:%02x",
//				addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]
//			);
			break;

		default:
			NRF_LOG_WARNING(
				"Disconnected - unhandled reason, handle: %d, reason: 0x%x",
				p_gap_evt->conn_handle, p_gap_evt->params.disconnected.reason
			);
	}

	// Remove handles after handlers have settled
	p_txrx->is_notification_enabled = false;
	p_txrx->conn_handle = BLE_GATT_HANDLE_INVALID;

	scan_start();
}




/**@brief Function for handling central related BLE events.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 * @param[in]   p_context   Unused.
 */
void central_on_ble_central_evt(ble_evt_t const *p_ble_evt) {
	ret_code_t err_code;
	ble_gap_evt_t const *p_gap_evt = &p_ble_evt->evt.gap_evt;
	txrx_ble_t * p_txrx = &m_txrx_central;

	switch (p_ble_evt->header.evt_id) {
		case BLE_GAP_EVT_CONNECTED:
			_on_connected(p_txrx, p_gap_evt);
			break;

		case BLE_GAP_EVT_DISCONNECTED:
			_on_disconnected(p_txrx, p_gap_evt);
			break;

		case BLE_GAP_EVT_TIMEOUT:
			// We have not specified a timeout for scanning, so only connection attempts can timeout.
			if (p_gap_evt->params.timeout.src == BLE_GAP_TIMEOUT_SRC_CONN) {
				NRF_LOG_DEBUG("Connection Request timed out.");
			} else {
				NRF_LOG_ERROR("Unhandled BLE_GAP_EVT_TIMEOUT source %02x", p_gap_evt->params.timeout.src);
			}
			break;

		case BLE_GATTC_EVT_HVX:
			{
				ble_gattc_evt_hvx_t const * p_evt_hvx = &p_ble_evt->evt.gattc_evt.params.hvx;
				if (p_evt_hvx->handle == p_txrx->rx_handles.value_handle) {
					LOG_TRACE("RX Up %d bytes:", p_evt_hvx->len);
					LOG_HEXDUMP_TRACE(p_evt_hvx->data, p_evt_hvx->len);
					//FIXME
					_parse_rx_packet( (comms_packet_t*)p_evt_hvx->data );
//					_TRIGGER_EVENT(p_txrx, TXRX_EVT_RX_DATA, {
//						evt.params.rx_data.p_data = (uint8_t*)p_evt_hvx->data;
//						evt.params.rx_data.length = p_evt_hvx->len;
//					});
				}
			}
			break;

		case BLE_GATTC_EVT_WRITE_RSP:
			{
				ble_gattc_evt_write_rsp_t const * p_evt_write = &p_ble_evt->evt.gattc_evt.params.write_rsp;
				if (p_evt_write->handle == p_txrx->rx_handles.cccd_handle) {
					p_txrx->is_notification_enabled = true;
					NRF_LOG_DEBUG("notification enabled!");

//					test_message();	//TODO Debug only

					//FIXME
//					_TRIGGER_EVENT(p_txrx, TXRX_EVT_COMM_STARTED);
				} else if (p_evt_write->handle == p_txrx->tx_handles.value_handle) {
					// @see https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.s132.api.v6.0.0%2Fgroup___b_l_e___g_a_t_t_c___f_u_n_c_t_i_o_n_s.html&cp=2_3_1_1_1_2_2_2_11&anchor=ga90298b8dcd8bbe7bbe69d362d1133378
					// This is a BLE ACK of sorts w/write_op = 0x01
					//FIXME
//					_TRIGGER_EVENT(p_txrx, TXRX_EVT_TX_DONE);
				} else {
					NRF_LOG_WARNING(
						"BLE_GATTC_EVT_WRITE_RSP on unhandled handle 0x%04x of len %d",
						p_evt_write->handle, p_evt_write->len
					);
				}
			}
			break;

		case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
			// Pairing not supported.
			err_code = sd_ble_gap_sec_params_reply(p_ble_evt->evt.gap_evt.conn_handle, BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP, NULL, NULL);
			APP_ERROR_CHECK(err_code);
			break;

		case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
				// Accept parameters requested by peer.
			err_code = sd_ble_gap_conn_param_update(
				p_gap_evt->conn_handle,
				&p_gap_evt->params.conn_param_update_request.conn_params
			);
			// TODO find out why this returns NRF_ERROR_INVALID_STATE sometimes; we don't have the code for it
			if (err_code == NRF_ERROR_INVALID_STATE) {
				NRF_LOG_ERROR("on_ble_central_evt conn param state error 8 handle %d", p_gap_evt->conn_handle);
			} else {
				APP_ERROR_CHECK(err_code);
			}
			break;

		case BLE_GAP_EVT_PHY_UPDATE_REQUEST: {
			NRF_LOG_DEBUG("PHY update request.");
			ble_gap_phys_t const phys =
				{
					.rx_phys = BLE_GAP_PHY_AUTO,
					.tx_phys = BLE_GAP_PHY_AUTO,
				};
			err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
			APP_ERROR_CHECK(err_code);
		} break;

		case BLE_GATTC_EVT_TIMEOUT:
			// Disconnect on GATT Client timeout event.
			NRF_LOG_DEBUG("GATT Client Timeout.");
			err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
				BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
			APP_ERROR_CHECK(err_code);
			break;

		case BLE_GATTS_EVT_TIMEOUT:
			// Disconnect on GATT Server timeout event.
			NRF_LOG_DEBUG("GATT Server Timeout.");
			err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
				BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
			APP_ERROR_CHECK(err_code);
			break;

		default:
			NRF_LOG_DEBUG("Unhandled BLE event %d", p_ble_evt->header.evt_id);
			break;
	}
}

//################################################################################
//	Service discovery
//################################################################################

/**@brief Function for handling database discovery events.
 *
 * @details This function is a callback function to handle events from the database discovery module.
 *          Depending on the UUIDs that are discovered, this function forwards the events
 *          to their respective services.
 *
 * @param[in] p_event  Pointer to the database discovery event.
 */
static void db_disc_handler(ble_db_discovery_evt_t *p_evt)
{
	// Store connection handle
	m_txrx_central.conn_handle = p_evt->conn_handle;
	// Check if txrx service was discovered.
	if (	(p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE)
		&&	(p_evt->params.discovered_db.srv_uuid.uuid == BLE_UUID_TXRX_SERVICE)
		&&	(p_evt->params.discovered_db.srv_uuid.type == BLE_UUID_TYPE_VENDOR_BEGIN)
	) {
		for (uint32_t i = 0; i < p_evt->params.discovered_db.char_count; i++) {
			const ble_gatt_db_char_t * p_char = &(p_evt->params.discovered_db.charateristics[i]);
			switch (p_char->characteristic.uuid.uuid) {
				case BLE_UUID_TXRX_RX_CHARACTERISTIC:
					m_txrx_central.tx_handles.value_handle = p_char->characteristic.handle_value;
					break;
				case BLE_UUID_TXRX_TX_CHARACTERISTIC:
					m_txrx_central.rx_handles.value_handle = p_char->characteristic.handle_value;
					m_txrx_central.rx_handles.cccd_handle = p_char->cccd_handle;
					break;
				default:
					break;
			}
		}
	}

	// Enable notification
	if (m_txrx_central.is_notification_enabled == false) {
		// turn on notifications
		NRF_LOG_DEBUG("turning on notifications");
		_cccd_configure(m_txrx_central.conn_handle, m_txrx_central.rx_handles.cccd_handle, true);
	} else {
		//FIXME
//		_TRIGGER_EVENT(p_txrx, TXRX_EVT_CONNECTED);
	}
}



//################################################################################
//	Initialization
//################################################################################

// register uuid with device discovery db
static void _central_register(ble_uuid128_t dev_uuid)
{
	ret_code_t err_code;

	uint8_t uuid_type;
	err_code = sd_ble_uuid_vs_add(&dev_uuid, &uuid_type);
	if (err_code != NRF_SUCCESS) {
		NRF_LOG_ERROR(
			"central_register: vs uuid add error %d; NRF_SDH_BLE_VS_UUID_COUNT might be too small", err_code
		);
	}

	ble_uuid_t uuid;
	uuid.type = uuid_type;
	uuid.uuid = BLE_UUID_TXRX_SERVICE;
	err_code = ble_db_discovery_evt_register(&uuid);
	if (err_code != NRF_SUCCESS) {
		NRF_LOG_ERROR("central_register err %d", err_code);
	} else {
//		registered_types++;
	}
}




void central_init(void)
{
	NRF_LOG_DEBUG("central_init");

	ret_code_t err_code = ble_db_discovery_init(db_disc_handler);
	APP_ERROR_CHECK(err_code);

	scan_init();

// This registers a vendor specific UUID
	ble_uuid128_t uuid = DEVICE_ID_GET(UUID_PARTIAL_KSE, DEV_ROCKET_REM);
	_central_register( uuid );


	ASSERT(NRF_SDH_BLE_CENTRAL_LINK_COUNT > 0);
	txrx_ble_init( &m_txrx, NULL );
	txrx_ble_init( &m_txrx_central, NULL );								\

	// start scanning for devices.
	scan_start();

	NRF_LOG_DEBUG("central_init complete");
}
