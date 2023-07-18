#pragma once
#include <windows.h>

#include "libusb.h"
#include "hal_htc_mp.h"

#define HAL_LIBUSB_INTERFACE0	0       // 设备的接口编号
#define HAL_LIBUSB_INTERFACE1	1       // 设备的接口编号
#define HAL_LIBUSB_INTERFACE2	2       // 设备的接口编号

#define HAL_LIBUSB_EP1_FW_DL		1							// endpoint
#define HAL_LIBUSB_EP7_FW_DL		7							// endpoint
#define HAL_LIBUSB_EP9_IN		9 | LIBUSB_ENDPOINT_IN		// endpoint
#define HAL_LIBUSB_EP9_OUT		9 | LIBUSB_ENDPOINT_OUT		// endpoint

// Firmware download
#define HAL_USB_FW_DL_EPID 1
#define HAL_USB_FW_DL_MTU 10240 // size of each firmware download transfer
							   // fw dl mtu +  sizeof(struct hal_usb_fw_dl_tag) must be
							   // 4 byte alignment. (usb dma address rule)

// Vendor Command Id
#define HAL_LIBUSB_ID_FW_DTOP_DL_ADDR      0xA4
#define HAL_LIBUSB_ID_DTOP_STARTPC		  0xA5
#define HAL_LIBUSB_ID_FW_WIFI_DL_ADDR      0xA6
#define HAL_LIBUSB_ID_FW_WIFI_STARTPC      0xA7
#define HAL_LIBUSB_ID_FW_BT_DL_ADDR        0xA8
#define HAL_LIBUSB_ID_FW_BT_STARTPC        0xA9
#define HAL_LIBUSB_ID_FW_DTOP_DL           0xAA
#define HAL_LIBUSB_ID_FW_DTOP_DL_COMP      0xAB
#define HAL_LIBUSB_ID_FW_WIFI_DL           0xAC
#define HAL_LIBUSB_ID_FW_WIFI_DL_COMP      0xAD
#define HAL_LIBUSB_ID_FW_BT_DL             0xAE
#define HAL_LIBUSB_ID_FW_BT_DL_COMP        0xAF
#define HAL_LIBUSB_ID_FW_DTOP_REMOVE       0xB0
#define HAL_LIBUSB_ID_FW_WIFI_REMOVE       0xB1
#define HAL_LIBUSB_ID_FW_BT_REMOVE         0xB2
#define HAL_LIBUSB_ID_FW_BT_DL_SKIP        0xB3
#define HAL_LIBUSB_ID_FW_WIFI_DL_SKIP      0xB4
#define HAL_LIBUSB_ID_FW_DTOP_DL_DATA      0xB7
#define HAL_LIBUSB_ID_FW_WIFI_DL_DATA      0xB8
#define HAL_LIBUSB_ID_FW_BT_DL_DATA        0xB6
#define HAL_LIBUSB_ID_GET_DTOP_DSTATE      0xBA
#define HAL_LIBUSB_ID_GET_WIFI_DSTATE      0xBB
#define HAL_LIBUSB_ID_GET_BT_DSTATE        0xBC
#define HAL_LIBUSB_ID_GET_RUNSYS           0xBD
#define HAL_LIBUSB_ID_GET_BOOTROM_DSTATE   0xBE
#define HAL_LIBUSB_ID_GET_ROM_VERSION      0xCA
#define HAL_LIBUSB_ID_GET_ROM_ERRNO        0xCB
#define HAL_LIBUSB_ID_GET_BT_FW_INFO       0xCC
#define HAL_LIBUSB_ID_GET_WIFI_FW_INFO     0xCD
#define HAL_LIBUSB_ID_SET_SOC_RESET        0xDA
#define HAL_LIBUSB_ID_CONFIG_DBGLOG        0xDB
#define HAL_LIBUSB_ID_CONFIG_LOGGER        0xDC
#define HAL_LIBUSB_ID_FW_BOOT_CFG_DL_ADDR  0xE0
#define HAL_LIBUSB_ID_FW_BOOT_CFG_STARTPC  0xE1
#define HAL_LIBUSB_ID_FW_BOOT_CFG_DL       0xE2
#define HAL_LIBUSB_ID_FW_BOOT_CFG_DL_COMP  0xE3
#define HAL_LIBUSB_ID_FW_BOOT_CFG_DL_DATA  0xE4
#define HAL_LIBUSB_MAGIC_RESET_SOC_VALUE   0xABCD
#define HAL_LIBUSB_MAGIC_RESET_SOC_INDEX   0x1234

typedef enum {
	HAL_LIBUSB_FW_DL_READY,
	HAL_LIBUSB__FW_DL_GOING,
	HAL_LIBUSB__FW_DL_REJECT,
} HAL_LIBUSB_FW_DL_REPLY_e;

typedef struct hal_wifi_libusb {
	libusb_context* context;
	libusb_device_handle* deviceHandle;
} hal_wifi_libusb_t;

typedef struct hal_libusb_fw_dl_tag
{
	UINT16 id;
	UINT16 checksum;
} hal_libusb_fw_dl_tag_t;

int hal_wifi_libusb_bulk_transfer(hal_wifi_libusb_t* phDevice, UINT8 endpoint, UINT8* data, int data_len);
int hal_wifi_libusb_control_write(hal_wifi_libusb_t* device, UINT8 Request, USHORT Value, USHORT Index);
int hal_wifi_libusb_control_read(hal_wifi_libusb_t* device, UINT8 Request, PUCHAR data, int data_len);
int hal_wifi_libusb_control_transfer(hal_wifi_libusb_t* device, UINT8 Request, USHORT Value, USHORT Index, PUCHAR data, int data_len);
UINT8 hal_wifi_libusb_init(hal_wifi_dev_t* phDevice);
BOOL hal_wifi_libusb_close(hal_wifi_dev_t phDevice);
int hal_wifi_libusb_get_rom_ver(hal_wifi_dev_t pdevice, hal_wifi_rom_ver_t* ver);
int hal_libusb_soc_reset(hal_wifi_dev_t phDevice);

int hal_libusb_runtime(hal_wifi_libusb_t* device, UINT8 Request, UINT32 addr, UINT8* runtime_data);
int hal_libusb_download_addr(hal_wifi_libusb_t* device, UINT8 Request, UINT32 addr);
int hal_libusb_download_start(hal_wifi_libusb_t* device, UINT8 Request, UINT32 fw_len, UINT8* fw_dl_state = NULL);
int hal_libusb_set_startpc(hal_wifi_libusb_t* device, UINT8 Request, UINT32 startpc);
int hal_libusb_download_finish(hal_wifi_libusb_t* device, UINT8 Request, UINT32 checksum);
int hal_wifi_libusb_get_state(hal_wifi_dev_t phDevice, UINT8 Request, hal_wifi_state_t* state);