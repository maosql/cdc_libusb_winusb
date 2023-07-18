/****************************************************************************

Copyright(c) 2016 by WuQi Technologies. ALL RIGHTS RESERVED.

This Information is proprietary to WuQi Technologies and MAY NOT
be copied by any method or incorporated into another program without
the express written consent of WuQi. This Information or any portion
thereof remains the property of WuQi. The Information contained herein
is believed to be accurate and WuQi assumes no responsibility or
liability for its use in any way and conveys no license or title under
any patent or copyright and makes no representation or warranty that this
Information is free from patent or copyright infringement.

****************************************************************************/
#pragma once
#include <Windows.h>
#include <stdint.h>

#define MATHLIBRARY_EXPORTS

#ifdef MATHLIBRARY_EXPORTS
#define MATHLIBRARY_API __declspec(dllexport)
#else
#define MATHLIBRARY_API __declspec(dllimport)
#endif

#pragma pack(push)
#pragma pack(1)

extern "C" MATHLIBRARY_API typedef struct {
	unsigned char major;
	unsigned char minor;
	unsigned char build_hr;
	unsigned char build_min;
} hal_wifi_rom_ver_t;

extern "C" MATHLIBRARY_API typedef struct {
	uint8_t state; // 0: not ready, 1: ready
} hal_wifi_state_t;

extern "C" MATHLIBRARY_API typedef struct hci_en_tx_pkt_test_cmd {
	uint8_t en;
	uint8_t tx_mode;
	uint8_t type;
	uint8_t hopping_mode;
	uint8_t lt_addr;
	uint16_t data_len;
	uint8_t payload;
	uint8_t tx_pwr_lvl;
	uint16_t freq;
	uint32_t tx_interval;
	uint32_t tx_packet_num;
} hci_en_tx_pkt_test_cmd_t;

#pragma pack(pop)

#define HAL_USB_CDC

extern "C" MATHLIBRARY_API typedef void* hal_wifi_dev_t;

/*********************************************************************
 * @fn     DUT_usbcdc_init
 * @brief  Download wpk/zip
 * @param1 phSerial: get the serial port handle
 * @param2 wpk_fw: wpk or zip file path
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 DUT_usbcdc_init(HANDLE* phSerial, const char* wpk_fw);

/*********************************************************************
 * @fn     DUT_usbcdc_close
 * @brief  Close the DUT
 * @param1 hSerial: serial port handle
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 DUT_usbcdc_close(HANDLE hSerial);

/*********************************************************************
 * @fn     DUT_libusb_init
 * @brief  Download wpk/zip
 * @param1 device: USB device driver
 * @param2 wpk_fw: wpk or zip file path
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 DUT_libusb_init(hal_wifi_dev_t* device, const char* wpk_fw);

/*********************************************************************
 * @fn     DUT_libusb_close
 * @brief  Close the DUT
 * @param1 device: USB device driver
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 DUT_libusb_close(hal_wifi_dev_t device);

/*********************************************************************
 * @fn     hal_open_serial_port
 * @brief  Open Serial port of DUT in the mass production
 * @param1 com: com port name
 * @param2 BaudRate: com baud rate, such as:9600,115200,38400
 * @return Serial Handle
 **********************************************************************/
extern "C" MATHLIBRARY_API HANDLE hal_open_serial_port(const char* com, int BaudRate);

/*********************************************************************
 * @fn     hal_close_serial_port
 * @brief  Close Serial port of DUT in the mass production
 * @param  hSerial: Handle of the serial port
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_close_serial_port(HANDLE hSerial);

/*********************************************************************
 * @fn     hal_cdc_send_wpk
 * @brief  Download wpk to DUT
 * @param  hSerial: Handle of the serial port
 * @param  wpk_fw: Wpk file path
 * @return Error Code
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_cdc_send_wpk(HANDLE hSerial, const char* wpk_fw);

/*********************************************************************
 * @fn     hal_chip_check_proc
 * @brief  Selfcheck efuse,FEM, USB, Memory if Hardware of DUT is OK
 * @param1 hSerial: Handle of the serial port
 * @param2 Testmode: working mode, default value:0
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_chip_check_proc(HANDLE hSerial, INT32 Testmode);

/*********************************************************************
 * @fn     hal_internal_temp_read
 * @brief  Read DUT internal temperature value
 * @param1 hSerial: Handle of the serial port
 * @param2 temp: the point of temperature data，unit：degree
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_internal_temp_read(HANDLE hSerial, INT32* temp);

/*********************************************************************
 * @fn     hal_ambient_temp_write
 * @brief  Send ambient temperature to DUT for temperature calibration
 * @param1 hSerial: Handle of the serial port
 * @param2 rf_path: 2G or 5G path, 2G:0, 5G:1
 * @param3 temp: measured ambient temperature data，unit：degree
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_ambient_temp_write(HANDLE hSerial, INT32 rf_path, INT32 temp);

/*********************************************************************
 * @fn     hal_efuse_write_data
 * @brief  Write data in the specific adddress in DUT efuse
 * @param1 hSerial: Handle of the serial port
 * @param2 addr_offset: byte address offset in efuse
 * @param3 data: byte value
 * @param4 data_len: bytes length
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_efuse_write_data(HANDLE hSerial, INT32 addr_offset, char* data, INT32 data_len);

/*********************************************************************
 * @fn     hal_efuse_read_data
 * @brief  Read a segment of data from the specific adddress in DUT efuse
 * @param1 hSerial: Handle of the serial port
 * @param2 addr_offset: byte address offset in efuse
 * @param3 efuse_data: the point of read data
 * @param4 length: byte length of read data
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_efuse_read_data(HANDLE hSerial, INT32 addr_offset, char* efuse_data, INT32 length);

/*********************************************************************
 * @fn     hal_xtal_cap_code_read
 * @brief  read xtal cap code of DUT
 * @param1 hSerial: Handle of the serial port
 * @param2 cap_code: cap code reading from DUT
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_xtal_cap_code_read(HANDLE hSerial, INT32 * cap_code);

/*********************************************************************
 * @fn     hal_crystal_set_freq_ppm
 * @brief  Send current ppm to DUT
 * @param1 hSerial: Handle of the serial port
 * @param2 ppm: the ppm of DUT
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_crystal_set_freq_ppm(HANDLE hSerial, float ppm);

/*********************************************************************
 * @fn     hal_mac_addr_write
 * @brief  burn mac address
 * @param1 hSerial: Handle of the serial port
 * @param2 mac_type: 0:2G, 1:5G,2:bt
 # @param3 macaddr: 6 byte array point passed to DUT
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_mac_addr_write(HANDLE hSerial, INT32 mac_type, UINT8 * macaddr);

/*********************************************************************
 * @fn     hal_wifi_mac_addr_read
 * @brief  exit wifi test mode
 * @param1 hSerial: Handle of the serial port
 * @param2 macaddr:7*N bytes array from DUT, N is number of mac address number.there 7 bytes in each mac
 *         including 1 byte of mac id, 6 bytes of mac address
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_mac_addr_read(HANDLE hSerial, UINT8 * macaddr, INT32 * macaddr_len);

/*********************************************************************
 * @fn     hal_wifi_init
 * @brief  Set DUT in wifi mode and initialize wifi working parameters
 * @param  hSerial: Handle of the serial port
 * @param  work_mode:0: calibration mode, 1: verification mode
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_wifi_init(HANDLE hSerial, INT32 work_mode);

/*********************************************************************
 * @fn     hal_wifi_tx_mpdu
 * @brief  DUT start to send tx packets
 * @param1 hSerial: Handle of the serial port
 * @param2 pkt_num: send packet number
 * @param3 pkt_len: send packet length
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_wifi_tx_mpdu(HANDLE hSerial, INT32 pkt_num, INT32 pkt_len);

/*********************************************************************
 * @fn     hal_wifi_set_channel
 * @brief  Set RF working channel of DUT
 * @param1 hSerial: Handle of the serial port
 * @param2 channel_no: channel_no range(1,10) for 2.4G band, range(36,177) for 5G band
 * @param3 bw: 0: 20MHz, 1:40MHz, 2: 80MHz
 * @param4 prim_position:
			In 40MHz bw, 0 for 40M plus, 1 for 40M minus
			In 80MHz bw, 0~3 for four primary 20M choice in a 80Mhz bandwidth, 0 is the lowest and 3 is the highest
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_wifi_set_channel(HANDLE hSerial, INT32 channel, INT32 bw, INT32 prim_position);

/*********************************************************************
 * @fn     hal_wifi_set_mode_rate
 * @brief  Set DUT WiFi standard and data rate
 * @param1 hSerial: Handle of the serial port
 * @param2 mode
			0. legacy: 11b/g
			2. 802.11n: HT
			4. 802.11ac: VHT
			5. 802.11ax: HE_SU
 * @param3 rate
			1.当leagcy mode, 设置速率数值：1,2,5,11,6,9,12,18，24,36,48,54(Mbps)
			2.当11n模式时，设置0-7,对应MCS0-MCS7
			3.当11ac模式时，设置0-9, 对应MCS0-MCS9
			4.当11ax模式时，设置0-11,对应MCS0-MCS11
 * @param4 bw: 20,40,80,160 (MHz)
 * @param5 gi_or_preamble
			1.	legacy mode: 0 (short preamble),1(long preamble)
			2.	11n,11ac: 0(LGI), 1(SGI)
			3.	11ax: 0 (0.8us GI), 1(1.6us GI), 2(3.2us GI)
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_wifi_set_mode_rate(HANDLE hSerial, INT32 mode, INT32 rate, INT32 bw, INT32 gi_or_preamble);

/*********************************************************************
 * @fn     hal_wifi_set_target_tx_pwr
 * @brief  Set WiFi Tx Target Power
 * @param1 hSerial: Handle of the serial port
 * @param2 calib_mode: 0 scan mode, 1 shot mode
 * @param4 rf_path: 2G or 5G path, 2G:0, 5G:1
 * @param5 power_dbm: tx power，unit：dBm
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_wifi_set_target_tx_pwr(HANDLE hSerial, INT32 calib_mode, INT32 rf_path, float power_dbm);

/*********************************************************************
 * @fn     hal_wifi_stop_tx
 * @brief  stop dut send packet
 * @param1 hSerial: Handle of the serial port
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_wifi_stop_tx(HANDLE hSerial);

/*********************************************************************
 * @fn     hal_wifi_test_tssi
 * @brief  star or stop dut tssi test
 * @param1 hSerial: Handle of the serial port
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_wifi_test_tssi(HANDLE hSerial);

/*********************************************************************
 * @fn     hal_wifi_set_real_tx_pwr
 * @brief  trigger DUT to calculate TSSI value
 * @param1 hSerial: Handle of the serial port
 * @param2 rf_path: 2G or 5G path, 2G:0, 5G:1
 * @param3 real_pwr_dbm: real TX power of DUT
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_wifi_set_real_tx_pwr(HANDLE hSerial, INT32 rf_path, float real_pwr_dbm);

/*********************************************************************
 * @fn     hal_wifi_get_tssi_value
 * @brief  get DUT TSSI value
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_wifi_get_tssi_value(HANDLE hSerial, float* tssi_pwr);

/*********************************************************************
 * @fn     hal_wifi_set_rx_gain
 * @brief  set rx gain of DUT
  *@param1 hSerial: Handle of the serial port
  *@param2 calib_mode: 0 scan mode, 1 shot mode
 * @param3 rf_path:  0(2G), 1(5G)
 * #param4 gain_idx: 指定Rx要接收使用的Gain Index
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_wifi_set_rx_gain(HANDLE hSerial, INT32 calib_mode, INT32 rf_path, INT32 gain_idx);

/*********************************************************************
 * @fn     hal_wifi_set_rx_rate
 * @brief  Set DUT WiFi standard and data rate
 * @param1 mode
			0. legacy: 11b/g
			2. 802.11n: HT
			4. 802.11ac: VHT
			5. 802.11ax: HE_SU
 * @param2 rate
			1.当leagcy mode, 设置速率数值：1,2,5,11,6,9,12,18，24,36,48,54(Mbps)
			2.当11n模式时，设置0-7,对应MCS0-MCS7
			3.当11ac模式时，设置0-9, 对应MCS0-MCS9
			4.当11ax模式时，设置0-11,对应MCS0-MCS11
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_wifi_set_rx_rate(HANDLE hSerial, INT32 rate_mode, INT32 rate);

/*********************************************************************
 * @fn     hal_wifi_start_rx_pkt_count
 * @brief  DUT start to receive packet
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_wifi_start_rx_pkt_count(HANDLE hSerial, INT32 rf_path);

/*********************************************************************
 * @fn     hal_wifi_get_rx_pkt_num
 * @brief  get received packets number of DUT
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_wifi_get_rx_pkt_num(HANDLE hSerial, INT32 * pkt_num);

/*********************************************************************
 * @fn     hal_wifi_set_real_rx_pwr
 * @brief  send real power of vsg out
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_wifi_set_real_rx_pwr(HANDLE hSerial, INT32 rf_path, float rx_pwr_dbm);

/*********************************************************************
 * @fn     hal_wifi_read_rssi_value
 * @brief  read rssi power of DUT
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_wifi_read_rssi_value(HANDLE hSerial, float* rssi_pwr);

/*********************************************************************
 * @fn     hal_wifi_calib_set
 * @brief  Request WiFi calibration data to be saved in DUT
 * @param1 hSerial: Handle of the serial port
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_wifi_calib_set(HANDLE hSerial);

/*********************************************************************
 * @fn     hal_wifi_calbuf_get
 * @brief  Get WiFi calibration data saved in DUT
 * @param1 hSerial: Handle of the serial port
 * @param2 calib_buf: the point of the calibration buffer
 * @param3 data_len: byte number of the calibration buffer
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_wifi_calbuf_get(HANDLE hSerial, UINT8 * calib_buf, INT32 * data_len);

/*********************************************************************
 * @fn     hal_wifi_calib_get
 * @brief  Get WiFi calibration data saved in DUT
 * @param1 hSerial: Handle of the serial port
 * @param2 calib_data: the point of the calibration data
 * @param3 data_len: byte number of the calibration data
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_wifi_calib_get(HANDLE hSerial, UINT8 * calib_data, INT32 * data_len);

/*********************************************************************
 * @fn     hal_wifi_deinit
 * @brief  exit wifi test mode
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API UINT8 hal_wifi_deinit(HANDLE hSerial);

/*********************************************************************
 * @fn     hal_bt_set_dtest_start
 * @brief  all api test
 * @param1 hSerial: Handle of the serial port
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API BOOL hal_bt_dtest_start(HANDLE hSerial, hci_en_tx_pkt_test_cmd_t param, UINT8* rx_data);
/*********************************************************************
 * @fn     hal_bt_init
 * @brief  configure DUT in BT mode and initialize parameters
 * @param1 hSerial: Handle of the serial port
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API BOOL hal_bt_init(HANDLE hSerial);

/*********************************************************************
 * @fn     hal_bt_set_freq
 * @brief  configure BT RF channel of DUT
 * @param1 hSerial: Handle of the serial port
 * @param2 freq: 0～65535,unit: MHz
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API BOOL hal_bt_set_freq(HANDLE hSerial, INT32 freq);

/*********************************************************************
 * @fn     hal_bt_set_tx_power
 * @brief  configure BT RF Tx power of DUT
 * @param1 hSerial: Handle of the serial port
 * @param2 power_qdBm: tx power unit: 0.25dBm
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API BOOL hal_bt_set_tx_power(HANDLE hSerial, INT32 power_qdBm);

/*********************************************************************
 * @fn     hal_bt_set_data_rate
 * @brief  configure BT RF Tx data rate of DUT
 * @param1 hSerial: Handle of the serial port
 * @param2 data_rate: data rate type
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API BOOL hal_bt_set_data_rate(HANDLE hSerial, INT32 data_rate);

/*********************************************************************
 * @fn     hal_bt_set_tx_pkt
 * @brief  configure BT RF Tx data packet parameters of DUT
 * @param1 hSerial: Handle of the serial port
 * @param2 pkt_len: byte number of data packet
 * @param3 payload: data rate type,1: PRBS9,2:PRBSVAL_AA,3:PRBSVAL_F0
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API BOOL hal_bt_set_tx_pkt(HANDLE hSerial, INT32 pkt_len, INT32 payload);

/*********************************************************************
 * @fn     hal_bt_always_tx
 * @brief  Enable BT Tx work or Stop
 * @param1 hSerial: Handle of the serial port
 * @param2  onoff 0: turn off,1: turn on
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API BOOL hal_bt_always_tx(HANDLE hSerial, INT32 onoff);

/*********************************************************************
 * @fn     hal_bt_always_rx
 * @brief  Enable BT Rx work or Stop
 * @param1 hSerial: Handle of the serial port
 * @param2 onoff 0: turn off,1: turn on
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API BOOL hal_bt_always_rx(HANDLE hSerial, INT32 onoff);

/*********************************************************************
* @fn     hal_bt_start_rx_pkt
* @brief  begin making statistic of correctly received packet numbers
* @param  hSerial: Handle of the serial port
* @return Success : 0, Fail : 1
 **********************************************************************/
extern "C" MATHLIBRARY_API BOOL hal_bt_start_rx_pkt(HANDLE hSerial);

/*********************************************************************
 * @fn     hal_bt_get_rx_pkt_num
 * @brief  Get correctly received packet numbers from DUT
 * @param1 hSerial: Handle of the serial port
 * @param2 pkt_num: the point of correctly received packet number
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API BOOL hal_bt_get_rx_pkt_num(HANDLE hSerial, INT32* pkt_num);

/*********************************************************************
*@fn     hal_bt_calib_get
* @brief  Get BT calibration data saved in DUT
* @param1 hSerial: Handle of the serial port
* @param2 calib_data : the point of the calibration data
* @param3 data_len : byte number of the calibration data
* @return Success : 0, Fail : 1
* *********************************************************************/
extern "C" MATHLIBRARY_API BOOL hal_bt_calib_get(HANDLE hSerial, char* calib_data, INT32* data_len);

/*********************************************************************
 * @fn     hal_bt_calib_set
 * @brief  Request BT calibration data to be saved in DUT
 * @param1 hSerial: Handle of the serial port
 * @return Success:0, Fail:1
 **********************************************************************/
extern "C" MATHLIBRARY_API BOOL hal_bt_calib_set(HANDLE hSerial);
