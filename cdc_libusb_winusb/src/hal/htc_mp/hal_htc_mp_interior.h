#pragma once

#define MAX_MP_SET_PARAM_LEN    128
#define MAX_MP_GET_PARAM_LEN    1024

enum cfm_result {
	CFM_FAIL = -1,
	CFM_SUCC = 0,
};

typedef enum {
	MSG_TYPE_DTOP_NOTIFY,
	MSG_TYPE_WIFI_NOTIFY,
	MSG_TYPE_BT_NOTIFY,
} desc_msg_type;

//typedef enum {
//	MP_TONE_NOTIFY					= 0,
//	MP_CHIP_CHECK_PROC_NOTIFY		= 1,
//	MP_INTERNAL_TEMP_READ			= 2,
//	MP_AMBIENT_TEMP_WRITE			= 3,
//	MP_EFUSE_WRITE					= 4,
//	MP_EFUSE_READ					= 5,
//	MP_WIFI_INIT_NOTIFY				= 6,
//	MP_WIFI_SET_BAND				= 7,
//	MP_WIFI_SET_CHANNEL				= 8,
//	MP_WIFI_SET_MODE				= 9,
//	MP_WIFI_SET_TX_POWER			= 10,
//	MP_WIFI_CALC_TSSI				= 11,
//	MP_WIFI_GET_TSSI				= 12,
//	MP_WIFI_SET_TX_PATTERN			= 13,
//	MP_RYSTAL_SET_CAP_CODE			= 14,
//	MP_WIFI_ALWAYS_TX				= 15,
//	MP_WIFI_ALWAYS_RX				= 16,
//	MP_WIFI_SET_RX_GAIN				= 17,
//	MP_WIFI_START_RX_PKT			= 18,
//	MP_WIFI_GET_RX_PKT_NUM			= 19,
//	MP_WIFI_READ_RSSI				= 20,
//	MP_WIFI_CALIB_GET				= 21,
//	MP_WIFI_CALIB_SET				= 22,
//	MP_BT_INIT						= 23,
//	MP_BT_SET_FREQ					= 24,
//	MP_BT_SET_TX_POWER				= 25,
//	MP_BT_SET_DATA_RATE				= 26,
//	MP_BT_SET_TX_PKT				= 27,
//	MP_BT_ALWAYS_TX					= 28,
//	MP_BT_ALWAYS_RX					= 29,
//	MP_BT_START_RX_PKT				= 30,
//	MP_BT_GET_RX_PKT_NUM			= 31,
//	MP_BT_CALIB_GET					= 32,
//	MP_BT_CALIB_SET					= 33,
//} mp_notify_type;

#define	STAILQ_ENTRY(type)						\
struct {								\
	struct type *stqe_next;	/* next element */			\
}

typedef struct htc_msg_desc {
	UINT8 msg_idx;
	UINT8 reseverd_1;
	UINT8 reseverd_2;
	UINT8 reseverd_3;
} htc_msg_desc_t;

typedef struct mp_get_cfm {
	char result;
} mp_get_cfm_t;

#define HTC_CFM_DESC_LEN 4
typedef struct htc_cfm_desc {
	UINT16 cfm_type;
	UINT16 len;
} htc_cfm_desc_t;

typedef struct mp_tone_gen {
	uint8_t enable;
	uint8_t num;
	uint16_t freq;
	uint16_t amp;
} mp_tone_gen_t;

typedef struct mp_info {
	UINT16 param_len;
	UINT8 param[MAX_MP_SET_PARAM_LEN];
	UINT16 Frame_tail;
} mp_info_t;



//typedef struct mp_set_mode_rate {
//	UINT32 mode;
//	UINT32 rate;
//} mp_set_mode_rate_t;

//typedef struct mp_wifi_calc_tssi {
//	INT32 RF_path;
//	INT32 rf_channel;
//	INT32 target_pwr_qdbm;
//	INT32 measured_pwr_qdbm;
//} mp_wifi_calc_tssi_t;

//typedef struct mp_wifi_get_tssi {
//	INT32 RF_path;
//	INT32 rf_channel;
//	INT32 target_pwr_qdbm;
//} mp_wifi_get_tssi_t;

typedef struct mp_wifi_get_pwr_tssi {
	INT32 pwr_diff_qdb;
	INT32 tssi_diff_qdb;
} mp_wifi_get_pwr_tssi_t;

//typedef struct mp_wifi_set_pattern {
//	INT32 PktLen_ms;
//	INT32 Period_ms;
//} mp_wifi_set_pattern_t;

//typedef struct mp_wifi_read_rssi {
//	INT32 rf_path;
//	INT32 rf_channel;
//	INT32 gain_index;
//} mp_wifi_read_rssi_t;

typedef struct mp_bt_set_tx_pkt {
	INT32 pkt_len;
	INT32 payload;
} mp_bt_set_tx_pkt_t;

typedef struct mp_efuse_read {
	INT32 addr_offset;
	INT32 length;
} mp_efuse_read_t;

typedef struct mp_wifi_calib_get {
	INT32 data_len;
	char calib_data[1024];
} mp_wifi_calib_get_t;

typedef struct mp_bt_calib_get {
	char* calib_data;
	INT32* data_len;
}mp_bt_calib_get_t;

#pragma pack(push)
#pragma pack(1)

typedef struct mp_efuse_write {
	char data;
	INT32 addr_offset;
} mp_efuse_write_t;

#pragma pack(pop)