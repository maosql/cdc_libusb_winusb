#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "hal_serial.h"
#include "hal_htc_mp_interior.h"
#include "hal_htc_mp.h"
#include "hal_libusb.h"

#define CACHE_BUFF_MAX 256
#define WIFI_NAME_MAX_BUFF 64
#define WIFI_PARAM_MAX_BUFF 16

int g_libusb_get_flag = 0;
UINT8 cache_buffer[CACHE_BUFF_MAX];
UINT8 cache_buffer_len = 0;

/* host to target cmd test */
static struct htc_msg_desc* hostmsg_desc_gen(UINT8 msg_type, void* data)
{
	htc_msg_desc_t* msg_desc = NULL;
	int datalen = sizeof(mp_info_t);

	msg_desc = (struct htc_msg_desc*)malloc(sizeof(htc_msg_desc_t) + datalen);
	if (msg_desc == NULL) {
		return NULL;
	}

	memset(msg_desc, 0, sizeof(htc_msg_desc_t) + datalen);

	msg_desc->msg_idx = msg_type;

	memcpy((UINT8*)msg_desc + sizeof(htc_msg_desc_t), data, datalen);

	return msg_desc;
}

static BOOL htc_mp_set_notify(HANDLE hSerial, desc_msg_type hostmsg_type, UINT8* param, UINT8 param_len)
{
	struct htc_msg_desc* desc = NULL;
	mp_info_t mp;
	BOOL ret = 1;

	if (param_len > MAX_MP_SET_PARAM_LEN) {
		printf("htc_mp_set_notify error param_len:%d > %d\n", param_len, MAX_MP_SET_PARAM_LEN);
		return 1;
	}

	memset(&mp, 0, sizeof(mp));
	mp.param_len = param_len;
	mp.Frame_tail = 0x200;

	if (param_len) {
		memcpy(mp.param, param, param_len);
	}

	desc = hostmsg_desc_gen(hostmsg_type, (void*)&mp);
	if (!desc) {
		printf("htc_mp_set_notify hostmsg_desc_gen error\n");
		return 1;
	}

#ifdef HAL_USB_CDC
	ret = hal_serial_port_write(hSerial, (char*)desc, sizeof(mp_info_t) + sizeof(htc_msg_desc_t));
#else
	hal_wifi_libusb_t* device = (hal_wifi_libusb_t*)hSerial;
	hal_wifi_libusb_bulk_transfer(device, HAL_LIBUSB_EP9_OUT, (UINT8*)desc, sizeof(mp_info_t) + sizeof(htc_msg_desc_t));
#endif
	free(desc);
	return ret;
}

static void htc_mp_dtop_set_notify(HANDLE hSerial, UINT8* param, UINT8 param_len)
{
	htc_mp_set_notify(hSerial, MSG_TYPE_DTOP_NOTIFY, param, param_len);
	Sleep(50);
}

static void htc_mp_wifi_set_notify(HANDLE hSerial, UINT8* param, UINT8 param_len)
{
	htc_mp_set_notify(hSerial, MSG_TYPE_WIFI_NOTIFY, param, param_len);
	Sleep(50);
}

static void htc_mp_bt_set_notify(HANDLE hSerial, UINT8* param, UINT8 param_len)
{
	htc_mp_set_notify(hSerial, MSG_TYPE_BT_NOTIFY, param, param_len);
	Sleep(50);
}

static void ReadSerialPort(HANDLE hSerial, UINT8* buf)
{
	OVERLAPPED overlapped = { 0 };
	DWORD bytesRead = 0;
	DWORD dwEventMask = 0;
	UINT8 rsp_buf[MAX_MP_GET_PARAM_LEN] = { 0 };
	DWORD bytes_read = 0;

	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (hEvent == NULL) {
		printf("Failed to create an event object. Procedure\n");
		return;
	}

	// 将事件对象关联到 overlapped 结构体中
	overlapped.hEvent = hEvent;

	// 使用 WaitCommEvent 函数等待串口数据到来
	if (!WaitCommEvent(hSerial, &dwEventMask, &overlapped)) {
		if (GetLastError() != ERROR_IO_PENDING) {
			printf("Failed to create an event object. Failed to wait for a serial port event\n");
			CloseHandle(hEvent);
			return;
		}
	}

	// 等待事件对象的触发
	DWORD dwWaitResult = WaitForSingleObject(hEvent, 1000);
	if (dwWaitResult == WAIT_OBJECT_0) {
		// 事件对象触发，表示串口数据到来
		if (!GetOverlappedResult(hSerial, &overlapped, &bytesRead, TRUE)) {
			// 获取读取结果失败，处理错误
			printf("get read ver failed\n");
			CloseHandle(hEvent);
			return;
		} else {
			// read incoming data
			if (!ReadFile(hSerial, rsp_buf, MAX_MP_GET_PARAM_LEN, &bytes_read, &overlapped)) {
				printf("Error reading from serial port.\n");
				CloseHandle(hSerial);
				return;
			}
			GetOverlappedResult(hSerial, &overlapped, &bytesRead, TRUE);
			memcpy(buf, rsp_buf, bytes_read);

			printf("Received data %d bytes: ", bytes_read);
			for (DWORD i = 0; i < bytes_read; i++)
			{
				printf("%02X ", rsp_buf[i]);
			}
			printf("\n");

			// 关闭事件句柄
			CloseHandle(overlapped.hEvent);
		}
	} else if (dwWaitResult == WAIT_TIMEOUT) {
		// 超时，没有数据到来
		printf("Timeout, no data arrived\n");
		CloseHandle(hEvent);
		return;
	} else {
		// 等待事件对象失败，处理错误
		printf("Failed to wait for event object.\n");
		CloseHandle(hEvent);
		return;
	}
}

// 传输完成事件的回调函数
void LIBUSB_CALL transfer_callback(struct libusb_transfer* transfer)
{
	if (transfer->status == LIBUSB_TRANSFER_COMPLETED) {
		g_libusb_get_flag = 1;
		cache_buffer_len = transfer->actual_length;

		memset(cache_buffer, 0, cache_buffer_len);
		memcpy(cache_buffer, transfer->buffer, cache_buffer_len);
	} else {
		// 数据接收失败或被取消，处理错误
		g_libusb_get_flag--;

		if (g_libusb_get_flag == -4) {
			g_libusb_get_flag = 2;
			return;
		}
		// 重新提交传输请求，以便继续接收数据
		libusb_submit_transfer(transfer);
	}
}

static BOOL htc_mp_get_notify(HANDLE hSerial, desc_msg_type hostmsg_type, UINT8* param, UINT16 param_len, UINT8* buf)
{
	struct htc_msg_desc* desc = NULL;
	mp_info_t mp;
	BOOL ret = 1;

	if (param_len > MAX_MP_GET_PARAM_LEN) {
		printf("htc_mp_set_notify error param_len:%d > %d\n", param_len, MAX_MP_GET_PARAM_LEN);
		return 1;
	}

	memset(&mp, 0, sizeof(mp));
	mp.param_len = param_len;
	mp.Frame_tail = 0x200;

	if (param_len) {
		memcpy(mp.param, param, param_len);
	}

	desc = hostmsg_desc_gen(hostmsg_type, (void*)&mp);
	if (!desc) {
		printf("htc_mp_get_notify hostmsg_desc_gen error\n");
		return 1;
	}

#ifdef HAL_USB_CDC
	ret = hal_serial_port_write(hSerial, (char*)desc, sizeof(mp_info_t) + sizeof(htc_msg_desc_t));
	ReadSerialPort(hSerial, buf);
#else
	DWORD reat_byte = 0;
	hal_wifi_libusb_t* device = (hal_wifi_libusb_t*)hSerial;

	hal_wifi_libusb_bulk_transfer(device, HAL_LIBUSB_EP9_OUT, (UINT8*)desc, sizeof(mp_info_t) + sizeof(htc_msg_desc_t));

	struct libusb_transfer* transfer = libusb_alloc_transfer(0);
	UINT8 buffer[CACHE_BUFF_MAX];

	libusb_fill_interrupt_transfer(transfer, device->deviceHandle, HAL_LIBUSB_EP9_IN, buffer, sizeof(buffer),
		transfer_callback, NULL, 1000);

	// 提交传输请求
	int submitResult = libusb_submit_transfer(transfer);
	if (submitResult != LIBUSB_SUCCESS) {
		return 1;
	}

	// 进入接收循环
	while (1) {
		// 处理USB事件
		libusb_handle_events(NULL);
		if (g_libusb_get_flag == 1) {
			g_libusb_get_flag = 0;
			memcpy(buf, cache_buffer, cache_buffer_len);

			break;
		} else if (g_libusb_get_flag == 2) {
			g_libusb_get_flag = 0;
			printf("libusb get date null\n");

			break;
		} else {
		}
	}

	//memcpy(buf, cache_buffer, cache_buffer_len);

	// 取消传输请求
	libusb_cancel_transfer(transfer);

	// 释放资源
	libusb_free_transfer(transfer);
#endif

	free(desc);
	return ret;
}
static void htc_mp_dtop_get_notify(HANDLE hSerial, UINT8* param, UINT16 param_len, UINT8* buf)
{
	Sleep(100);
	htc_mp_get_notify(hSerial, MSG_TYPE_DTOP_NOTIFY, param, param_len, buf);
}

static void htc_mp_wifi_get_notify(HANDLE hSerial, UINT8* param, UINT16 param_len, UINT8* buf)
{
	Sleep(100);
	htc_mp_get_notify(hSerial, MSG_TYPE_WIFI_NOTIFY, param, param_len, buf);
}

static void htc_mp_bt_get_notify(HANDLE hSerial, UINT8* param, UINT16 param_len, UINT8* buf)
{
	Sleep(100);
	htc_mp_get_notify(hSerial, MSG_TYPE_BT_NOTIFY, param, param_len, buf);
}

char separator[] = " ";
size_t len_separator = strlen(separator);

static UINT8 hal_strcat(char* str1, char* str2)
{
	size_t len1 = strlen(str1);
	size_t len2 = strlen(str2);

	if ((len1 + len2 + len_separator) < WIFI_NAME_MAX_BUFF) {
		strcat(str1, separator);
		strcat(str1, str2);
	}
	else {
		printf("Error: Insufficient buffer space.\n");
		return 1;
	}
	return 0;
}

//=========================DTOP API=========================
UINT8 hal_chip_check_proc(HANDLE hSerial, INT32 Testmode)
{
	INT32 param = 0;
	param = Testmode;

	htc_mp_dtop_set_notify(hSerial, (UINT8*)&param, sizeof(INT32));

	return 0;
}

UINT8 hal_internal_temp_read(HANDLE hSerial, INT32* temp)
{
	char wifi_buf[WIFI_NAME_MAX_BUFF] = "mp_get_chip_temp";

	*temp = 0;
	htc_mp_wifi_get_notify(hSerial, (UINT8*)wifi_buf, (UINT8)strlen(wifi_buf), (uint8_t*)temp);

	return 0;
}

UINT8 hal_ambient_temp_write(HANDLE hSerial, INT32 rf_path, INT32 temp)
{
	char wifi_buf[WIFI_NAME_MAX_BUFF] = "mp_set_amb_temp";
	char wifi_param[WIFI_PARAM_MAX_BUFF] = { 0 };

	sprintf(wifi_param, "%d", rf_path);
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	sprintf(wifi_param, "%d", temp);
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	htc_mp_wifi_set_notify(hSerial, (UINT8*)wifi_buf, (UINT8)strlen(wifi_buf));

	return 0;
}

UINT8 hal_efuse_write_data(HANDLE hSerial, INT32 addr_offset, char *data, INT32 data_len)
{
	mp_efuse_write_t param;

	param.addr_offset = addr_offset;
	param.data = *data;
	htc_mp_dtop_set_notify(hSerial, (UINT8*)&param, sizeof(param));

	return 0;
}

UINT8 hal_efuse_read_data(HANDLE hSerial, INT32 addr_offset, char* efuse_data, INT32 length)
{
	mp_efuse_read_t param;

	param.addr_offset = addr_offset;
	param.length = length;

	htc_mp_dtop_get_notify(hSerial, (UINT8*)&param, sizeof(param), (UINT8*)&efuse_data);

	return 0;
}

UINT8 hal_xtal_cap_code_read(HANDLE hSerial, INT32* cap_code)
{
	char wifi_buf[WIFI_NAME_MAX_BUFF] = "mp_get_cap_code";

	*cap_code = 0;
	htc_mp_wifi_get_notify(hSerial, (UINT8*)wifi_buf, (UINT8)strlen(wifi_buf), (uint8_t*)cap_code);

	return 0;
}

UINT8 hal_crystal_set_freq_ppm(HANDLE hSerial, float ppm)
{
	char wifi_buf[WIFI_NAME_MAX_BUFF] = "mp_set_ppm";
	char wifi_param[WIFI_PARAM_MAX_BUFF] = { 0 };

	sprintf(wifi_param, "%d", (INT32)(ppm * 100));
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	htc_mp_wifi_set_notify(hSerial, (UINT8*)wifi_buf, (UINT8)strlen(wifi_buf));

	return 0;
}

UINT8 hal_mac_addr_write(HANDLE hSerial, INT32 mac_type, UINT8* macaddr)
{
	char wifi_buf[WIFI_NAME_MAX_BUFF] = "mp_mac_write";
	char wifi_param[WIFI_PARAM_MAX_BUFF] = { 0 };

	sprintf(wifi_param, "%d", mac_type);
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	uint8_t mac_str[13] = { 0 };
	for (uint8_t i = 0; i < 6; i++) {
		for (uint8_t k = 0; k < 2; k++) {
			uint8_t data = 0;
			if (0 == k) {
				data = macaddr[i] & 0x0F;
			}
			else {
				data = (macaddr[i] >> 4) & 0x0F;
			}
			if (data <= 9) {
				mac_str[2 * (6 - i) - 1 - k] = data + 48;
			}
			else {
				mac_str[2 * (6 - i) - 1 - k] = data - 10 + 65;
			}
		}
	}

	if (hal_strcat(wifi_buf, (char*)mac_str)) {
		return 1;
	}

	htc_mp_wifi_set_notify(hSerial, (UINT8*)wifi_buf, (UINT8)strlen(wifi_buf));

	return 0;
}

UINT8 hal_mac_addr_read(HANDLE hSerial, UINT8* macaddr, INT32* macaddr_len)
{
	char wifi_buf[WIFI_NAME_MAX_BUFF] = "mp_mac_read";

	htc_mp_wifi_get_notify(hSerial, (UINT8*)wifi_buf, (UINT8)strlen(wifi_buf), (UINT8*)macaddr);

	return 0;
}


//=========================WIFI API=========================
UINT8 hal_wifi_init(HANDLE hSerial, INT32 work_mode)
{
	char wifi_buf[WIFI_NAME_MAX_BUFF] = "mp_wifi_init";
	char wifi_param[WIFI_PARAM_MAX_BUFF] = { 0 };

	sprintf(wifi_param, "%d", work_mode);
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	htc_mp_wifi_set_notify(hSerial, (UINT8*)wifi_buf, (UINT8)strlen(wifi_buf));

	return 0;
}

UINT8 hal_wifi_tx_mpdu(HANDLE hSerial, INT32 pkt_num, INT32 pkt_len)
{
	char wifi_buf[WIFI_NAME_MAX_BUFF] = "mp_tx_mpdu";
	char wifi_param[WIFI_PARAM_MAX_BUFF] = { 0 };

	sprintf(wifi_param, "%d", pkt_num);
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	sprintf(wifi_param, "%d", pkt_len);
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	htc_mp_wifi_set_notify(hSerial, (UINT8*)wifi_buf, (UINT8)strlen(wifi_buf));

	return 0;
}

UINT8 hal_wifi_set_channel(HANDLE hSerial, INT32 channel, INT32 bw, INT32 prim_position)
{
	char wifi_buf[WIFI_NAME_MAX_BUFF] = "mp_set_ch";
	char wifi_param[WIFI_PARAM_MAX_BUFF] = { 0 };

	sprintf(wifi_param, "%d", channel);
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	sprintf(wifi_param, "%d", bw);
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	sprintf(wifi_param, "%d", prim_position);
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	htc_mp_wifi_set_notify(hSerial, (UINT8*)wifi_buf, (UINT8)strlen(wifi_buf));

	return 0;
}

UINT8 hal_wifi_set_mode_rate(HANDLE hSerial, INT32 mode, INT32 rate, INT32 bw, INT32 gi_or_preamble)
{
	char wifi_buf[WIFI_NAME_MAX_BUFF] = "mp_set_mode_rate";
	char wifi_param[WIFI_PARAM_MAX_BUFF] = { 0 };

	sprintf(wifi_param, "%d", mode);
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	sprintf(wifi_param, "%d", rate);
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	sprintf(wifi_param, "%d", bw);
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	sprintf(wifi_param, "%d", gi_or_preamble);
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	htc_mp_wifi_set_notify(hSerial, (UINT8*)wifi_buf, (UINT8)strlen(wifi_buf));

	return 0;
}

UINT8 hal_wifi_set_target_tx_pwr(HANDLE hSerial, INT32 calib_mode, INT32 rf_path, float power_dbm)
{
	char wifi_buf[WIFI_NAME_MAX_BUFF] = "mp_set_trgt_tx_pwr";
	char wifi_param[WIFI_PARAM_MAX_BUFF] = { 0 };

	sprintf(wifi_param, "%d", calib_mode);
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	sprintf(wifi_param, "%d", rf_path);
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	sprintf(wifi_param, "%d", (INT32)(power_dbm * 100));
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	htc_mp_wifi_set_notify(hSerial, (UINT8*)wifi_buf, (UINT8)strlen(wifi_buf));

	return 0;
}

UINT8 hal_wifi_stop_tx(HANDLE hSerial)
{
	char wifi_buf[WIFI_NAME_MAX_BUFF] = "mp_stop_tx";

	htc_mp_wifi_set_notify(hSerial, (UINT8*)wifi_buf, (UINT8)strlen(wifi_buf));

	return 0;
}

UINT8 hal_wifi_test_tssi(HANDLE hSerial)
{
	char wifi_buf[WIFI_NAME_MAX_BUFF] = "mp_test_tssi";

	htc_mp_wifi_set_notify(hSerial, (UINT8*)wifi_buf, (UINT8)strlen(wifi_buf));

	return 0;
}

UINT8 hal_wifi_set_real_tx_pwr(HANDLE hSerial, INT32 rf_path, float real_pwr_dbm)
{
	char wifi_buf[WIFI_NAME_MAX_BUFF] = "mp_set_real_tx_pwr";
	char wifi_param[WIFI_PARAM_MAX_BUFF] = { 0 };

	sprintf(wifi_param, "%d", rf_path);
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	sprintf(wifi_param, "%d", (INT32)(real_pwr_dbm * 100));
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	htc_mp_wifi_set_notify(hSerial, (UINT8*)wifi_buf, (UINT8)strlen(wifi_buf));

	return 0;
}

UINT8 hal_wifi_get_tssi_value(HANDLE hSerial, float* tssi_pwr)
{
	char wifi_buf[WIFI_NAME_MAX_BUFF] = "mp_get_tssi_val";

	INT32 tmp_tssi_pwr = 0;
	htc_mp_wifi_get_notify(hSerial, (UINT8*)wifi_buf, (UINT8)strlen(wifi_buf), (uint8_t*)&tmp_tssi_pwr);
	*tssi_pwr = (float)(tmp_tssi_pwr / 100.0);

	return 0;
}

UINT8 hal_wifi_set_rx_gain(HANDLE hSerial, INT32 calib_mode, INT32 rf_path, INT32 gain_idx)
{
	char wifi_buf[WIFI_NAME_MAX_BUFF] = "mp_set_rx_gain";
	char wifi_param[WIFI_PARAM_MAX_BUFF] = { 0 };

	sprintf(wifi_param, "%d", calib_mode);
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	sprintf(wifi_param, "%d", rf_path);
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	sprintf(wifi_param, "%d", gain_idx);
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	htc_mp_wifi_set_notify(hSerial, (UINT8*)wifi_buf, (UINT8)strlen(wifi_buf));

	return 0;
}

UINT8 hal_wifi_set_rx_rate(HANDLE hSerial, INT32 rate_mode, INT32 rate)
{
	char wifi_buf[WIFI_NAME_MAX_BUFF] = "mp_set_rx_rate";
	char wifi_param[WIFI_PARAM_MAX_BUFF] = { 0 };

	sprintf(wifi_param, "%d", rate_mode);
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	sprintf(wifi_param, "%d", rate);
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	htc_mp_wifi_set_notify(hSerial, (UINT8*)wifi_buf, (UINT8)strlen(wifi_buf));

	return 0;
}

UINT8 hal_wifi_start_rx_pkt_count(HANDLE hSerial, INT32 rf_path)
{
	char wifi_buf[WIFI_NAME_MAX_BUFF] = "mp_start_rx_pkt";
	char wifi_param[WIFI_PARAM_MAX_BUFF] = { 0 };

	sprintf(wifi_param, "%d", rf_path);
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	htc_mp_wifi_set_notify(hSerial, (UINT8*)wifi_buf, (UINT8)strlen(wifi_buf));

	return 0;
}

UINT8 hal_wifi_get_rx_pkt_num(HANDLE hSerial, INT32* pkt_num)
{
	char wifi_buf[WIFI_NAME_MAX_BUFF] = "mp_get_rx_pkt_num";

	*pkt_num = 0;
	htc_mp_wifi_get_notify(hSerial, (UINT8*)wifi_buf, (UINT8)strlen(wifi_buf), (uint8_t*)pkt_num);

	return 0;
}

UINT8 hal_wifi_set_real_rx_pwr(HANDLE hSerial, INT32 rf_path, float rx_pwr_dbm)
{
	char wifi_buf[WIFI_NAME_MAX_BUFF] = "get_rx_pkt_num";
	char wifi_param[WIFI_PARAM_MAX_BUFF] = { 0 };

	UINT8 rx_buf[256] = { 0 };

	sprintf(wifi_param, "%d", rf_path);
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	sprintf(wifi_param, "%d", (INT32)(rx_pwr_dbm * 100));
	if (hal_strcat(wifi_buf, wifi_param)) {
		return 1;
	}

	htc_mp_wifi_set_notify(hSerial, (UINT8*)wifi_buf, (UINT8)strlen(wifi_buf));

	return 0;
}

UINT8 hal_wifi_read_rssi_value(HANDLE hSerial, float* rssi_pwr)
{
	char wifi_buf[WIFI_NAME_MAX_BUFF] = "mp_read_rssi_val";

	INT32 tmp_rssi_pwr = 0;
	htc_mp_wifi_get_notify(hSerial, (UINT8*)wifi_buf, (UINT8)strlen(wifi_buf), (uint8_t*)&tmp_rssi_pwr);
	*rssi_pwr = (float)(tmp_rssi_pwr / 100.0);

	return 0;
}

UINT8 hal_wifi_calib_set(HANDLE hSerial)
{
	char wifi_buf[WIFI_NAME_MAX_BUFF] = "mp_set_calib";
	htc_mp_wifi_set_notify(hSerial, (UINT8*)wifi_buf, (UINT8)strlen(wifi_buf));

	return 0;
}

UINT8 hal_wifi_calbuf_get(HANDLE hSerial, UINT8* calib_buf, INT32* data_len)
{
	mp_wifi_calib_get_t rx_param = { 0 };
	char wifi_buf[WIFI_NAME_MAX_BUFF] = "mp_get_calbuf";

	htc_mp_wifi_get_notify(hSerial, (UINT8*)wifi_buf, (UINT8)strlen(wifi_buf), (UINT8*)calib_buf);

	//if (calib_data == NULL || data_len == NULL) {
	//	return false;
	//}

	*data_len = 128;
	//for (INT32 i = 0; i < 128; i++) {
	//	calib_data[i] = rx_param.calib_data[i + 4];
	//}
	return 0;
}

UINT8 hal_wifi_calib_get(HANDLE hSerial, UINT8* calib_data, INT32* data_len)
{
	mp_wifi_calib_get_t rx_param = { 0 };
	char wifi_buf[WIFI_NAME_MAX_BUFF] = "mp_get_calib";

	htc_mp_wifi_get_notify(hSerial, (UINT8*)wifi_buf, (UINT8)strlen(wifi_buf), (UINT8*)&rx_param);

	if (calib_data == NULL || data_len == NULL) {
		return 1;
	}

	*data_len = 128;
	for (INT32 i = 0; i < 128; i++) {
		calib_data[i] = rx_param.calib_data[i + 4];
	}
	return 0;
}

UINT8 hal_wifi_deinit(HANDLE hSerial)
{
	char wifi_buf[WIFI_NAME_MAX_BUFF] = "mp_deinit";

	htc_mp_wifi_set_notify(hSerial, (UINT8*)wifi_buf, (UINT8)strlen(wifi_buf));

	return 0;
}

//=========================BT API=========================
#define HCI_EN_TX_PKT_TEST_CMD_OPCODE	0xFCB3
#pragma pack(push)
#pragma pack(1)
typedef struct hci_format {
	UINT8 head;
	UINT16 opcode;
	UINT8 param_len;
	UINT8 bt_api;
	UINT32 param1;
	UINT32 param2;
} hci_format_t;

typedef struct rx_hci_format {
	UINT8 event_code;
	UINT8 param_len;
	UINT32 param1;
	UINT32 param2;
} rx_hci_format_t;

typedef struct hci_format_all
{
	uint8_t head;
	uint16_t opcode;
	uint8_t param_len;
	hci_en_tx_pkt_test_cmd_t param;
} hci_format_all_t;

#pragma pack(pop)

uint32_t swapEndian(uint32_t value)
{
	return ((value & 0xFF000000) >> 24) |
		((value & 0x00FF0000) >> 8) |
		((value & 0x0000FF00) << 8) |
		((value & 0x000000FF) << 24);
}

/* pHY colleage use this API */
BOOL hal_bt_dtest_start(HANDLE hSerial, hci_en_tx_pkt_test_cmd_t param, UINT8* rx_data)
{
	hci_format_all_t bt_hci = { 0 };

	bt_hci.head = 0x01;
	bt_hci.opcode = HCI_EN_TX_PKT_TEST_CMD_OPCODE;
	bt_hci.param_len = 19;
	memcpy(&bt_hci.param, &param, sizeof(param));

	htc_mp_bt_get_notify(hSerial, (UINT8*)&bt_hci, sizeof(hci_format_all_t), rx_data);

	return 0;
}

BOOL hal_bt_init(HANDLE hSerial)
{
	hci_format_t bt_hci = { 0 };

	bt_hci.head = 0x01;
	bt_hci.opcode = 0xfe10;
	bt_hci.param_len = 9;
	bt_hci.bt_api = 0;

	htc_mp_bt_set_notify(hSerial, (UINT8*)&bt_hci, sizeof(hci_format_t));

	return 0;
}

BOOL hal_bt_set_freq(HANDLE hSerial, INT32 freq)
{
	hci_format_t bt_hci = { 0 };

	bt_hci.head = 0x01;
	bt_hci.opcode = 0xfe10;
	bt_hci.param_len = 9;
	bt_hci.bt_api = 1;
	bt_hci.param1 = freq;

	htc_mp_bt_set_notify(hSerial, (UINT8*)&bt_hci, sizeof(hci_format_t));

	return 0;
}

BOOL hal_bt_set_tx_power(HANDLE hSerial, INT32 power_qdBm)
{
	hci_format_t bt_hci = { 0 };

	bt_hci.head = 0x01;
	bt_hci.opcode = 0xfe10;
	bt_hci.param_len = 9;
	bt_hci.bt_api = 2;
	bt_hci.param1 = power_qdBm;

	htc_mp_bt_set_notify(hSerial, (UINT8*)&bt_hci, sizeof(hci_format_t));

	return 0;
}

BOOL hal_bt_set_data_rate(HANDLE hSerial, INT32 data_rate)
{
	hci_format_t bt_hci = { 0 };

	bt_hci.head = 0x01;
	bt_hci.opcode = 0xfe10;
	bt_hci.param_len = 9;
	bt_hci.bt_api = 3;
	bt_hci.param1 = data_rate;

	htc_mp_bt_set_notify(hSerial, (UINT8*)&bt_hci, sizeof(hci_format_t));

	return 0;
}

BOOL hal_bt_set_tx_pkt(HANDLE hSerial, INT32 pkt_len, INT32 payload)
{
	hci_format_t bt_hci = { 0 };

	bt_hci.head = 0x01;
	bt_hci.opcode = 0xfe10;
	bt_hci.param_len = 9;
	bt_hci.bt_api = 4;
	bt_hci.param1 = pkt_len;
	bt_hci.param2 = payload;

	htc_mp_bt_set_notify(hSerial, (UINT8*)&bt_hci, sizeof(hci_format_t));

	return 0;
}

BOOL hal_bt_always_tx(HANDLE hSerial, INT32 onoff)
{
	hci_format_t bt_hci = { 0 };

	bt_hci.head = 0x01;
	bt_hci.opcode = 0xfe10;
	bt_hci.param_len = 9;
	bt_hci.bt_api = 5;
	bt_hci.param1 = onoff;

	htc_mp_bt_set_notify(hSerial, (UINT8*)&bt_hci, sizeof(hci_format_t));

	return 0;
}

BOOL hal_bt_always_rx(HANDLE hSerial, INT32 onoff)
{
	hci_format_t bt_hci = { 0 };

	bt_hci.head = 0x01;
	bt_hci.opcode = 0xfe10;
	bt_hci.param_len = 9;
	bt_hci.bt_api = 6;
	bt_hci.param1 = onoff;

	htc_mp_bt_set_notify(hSerial, (UINT8*)&bt_hci, sizeof(hci_format_t));

	return 0;
}

BOOL hal_bt_start_rx_pkt(HANDLE hSerial)
{
	hci_format_t bt_hci = { 0 };

	bt_hci.head = 0x01;
	bt_hci.opcode = 0xfe10;
	bt_hci.param_len = 9;
	bt_hci.bt_api = 7;

	htc_mp_bt_set_notify(hSerial, (UINT8*)&bt_hci, sizeof(hci_format_t));

	return 0;
}

BOOL hal_bt_get_rx_pkt_num(HANDLE hSerial, INT32* pkt_num)
{
	hci_format_t bt_hci = { 0 };
	rx_hci_format_t rx_data;

	bt_hci.head = 0x01;
	bt_hci.opcode = 0xfe10;
	bt_hci.param_len = 9;
	bt_hci.bt_api = 8;

	htc_mp_bt_get_notify(hSerial, (UINT8*)&bt_hci, sizeof(hci_format_t), (UINT8*)&rx_data);

	if (pkt_num == NULL) {
		return 1;
	}

	if (rx_data.event_code == 0x04) {
		*pkt_num = rx_data.param1;
	}

	return 0;
}

BOOL hal_bt_calib_get(HANDLE hSerial, char* calib_data, INT32* data_len)
{
	hci_format_t bt_hci = { 0 };
	UINT8 get_cali_info[20] = { 0 };

	bt_hci.head = 0x01;
	bt_hci.opcode = 0xfe10;
	bt_hci.param_len = 9;
	bt_hci.bt_api = 9;

	htc_mp_bt_get_notify(hSerial, (UINT8*)&bt_hci, sizeof(hci_format_t), get_cali_info);
	if (get_cali_info[0] == 0x04) {
		*data_len = (INT32)get_cali_info[1];
		for (INT32 i = 0; i < (*data_len); i++) {
			calib_data[i] = get_cali_info[i + 2];
		}
	}

	return 0;
}

BOOL hal_bt_calib_set(HANDLE hSerial)
{
	hci_format_t bt_hci = { 0 };

	bt_hci.head = 0x01;
	bt_hci.opcode = 0xfe10;
	bt_hci.param_len = 9;
	bt_hci.bt_api = 10;

	htc_mp_bt_set_notify(hSerial, (UINT8*)&bt_hci, sizeof(hci_format_t));

	return 0;
}
