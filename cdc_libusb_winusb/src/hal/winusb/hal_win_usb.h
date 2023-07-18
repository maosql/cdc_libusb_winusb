#pragma once
#include <windows.h>
#include <winusb.h>

#include "hal_htc_mp.h"

#define USE_HAL_WIN_USB

typedef struct {
	HANDLE deviceHandle;
	WINUSB_INTERFACE_HANDLE interfaceHandle;
	OVERLAPPED control_overlap;
	OVERLAPPED bulk_out_overlap;
	OVERLAPPED bulk_in_overlap;
	UINT32 cmd_seq;
	char path[MAX_PATH];
	char err_msg[MAX_PATH];
	UINT8 rcv_buffer[MAX_PATH];
} hal_wifi_device_t;

/*
 * USB directions
 *
 * This bit flag is used in endpoint descriptors' bEndpointAddress field.
 * It's also one of three fields in control requests bRequestType.
 */
#define USB_DIR_OUT 0	/* to device */
#define USB_DIR_IN 0x80 /* to host */

 /*
  * USB types, the second of three bRequestType fields
  */
#define USB_TYPE_MASK (0x03 << 5)
#define USB_TYPE_STANDARD (0x00 << 5)
#define USB_TYPE_CLASS (0x01 << 5)
#define USB_TYPE_VENDOR (0x02 << 5)
#define USB_TYPE_RESERVED (0x03 << 5)

  /*
   * USB recipients, the third of three bRequestType fields
   */
#define USB_RECIP_MASK 0x1f
#define USB_RECIP_DEVICE 0x00
#define USB_RECIP_INTERFACE 0x01
#define USB_RECIP_ENDPOINT 0x02
#define USB_RECIP_OTHER 0x03
   /* From Wireless USB 1.0 */
#define USB_RECIP_PORT 0x04
#define USB_RECIP_RPIPE 0x05

// Vendor Command Id
#define HAL_WINUSB_ID_FW_DL 0xAA		// usb vendor request id for firmware download
#define HAL_WINUSB_ID_FW_DL_COMP 0xAB	// usb vendor request id for firmware download complete
#define HAL_WINUSB_ID_GET_DSTATE 0xBA	// usb vendor request id for device state check
#define HAL_WINUSB_ID_GET_ROM_VER 0xCA // usb vendor request id for get rom version
#define HAL_WINUSB_USB_REQ_TYPE_DEVICE_SET_SOC_RESET       0xDA
#define HAL_WINUSB_MAGIC_USB_RESET_SOC_VALUE       0xABCD
#define HAL_WINUSB_MAGIC_USB_RESET_SOC_INDEX       0x1234

LPCWSTR ConvertToLPCWSTR(const char* str);
BOOL hal_wifi_enum(char path_list[][MAX_PATH], int max_count, int* found_count);
//BOOL hal_wifi_enum(char path_list[][MAX_PATH], UINT8 max_count, int* found_count);
BOOL OpenWinUSBDevice(const char* DevicePath, hal_wifi_dev_t* phDevice);

BOOL hal_wifi_get_rom_ver(hal_wifi_dev_t pdevice, hal_wifi_rom_ver_t* ver);
const char* hal_wifi_get_err_msg(hal_wifi_dev_t pdevice);