#pragma once
#ifndef __SERIAL_H__
#define __SERIAL_H__
#include <windows.h>

//#define HAL_DEBUG
#ifdef HAL_DEBUG
#define hal_printf(...) printf(__VA_ARGS__)
#else
#define hal_printf(...)
#endif

#define BUF_SIZE    1024
#define RX_BUFFER_LEN 8
typedef enum {
    USB_REQ_TYPE_DEVICE_FW_DTOP_DL_ADDR = 0xA4,
    USB_REQ_TYPE_DEVICE_FW_DTOP_STARTPC = 0xA5,
    USB_REQ_TYPE_DEVICE_FW_WIFI_DL_ADDR = 0xA6,
    USB_REQ_TYPE_DEVICE_FW_WIFI_STARTPC = 0xA7,
    USB_REQ_TYPE_DEVICE_FW_BT_DL_ADDR   = 0xA8,
    USB_REQ_TYPE_DEVICE_FW_BT_STARTPC   = 0xA9,
    USB_REQ_TYPE_DEVICE_FW_DTOP_DL      = 0xAA,
    USB_REQ_TYPE_DEVICE_FW_DTOP_DL_COMP = 0xAB,
    USB_REQ_TYPE_DEVICE_FW_WIFI_DL      = 0xAC,
    USB_REQ_TYPE_DEVICE_FW_WIFI_DL_COMP = 0xAD,
    USB_REQ_TYPE_DEVICE_FW_BT_DL        = 0xAE,
    USB_REQ_TYPE_DEVICE_FW_BT_DL_COMP   = 0xAF,
    USB_REQ_TYPE_DEVICE_FW_DTOP_REMOVE  = 0xB0,
    USB_REQ_TYPE_DEVICE_FW_WIFI_REMOVE  = 0xB1,
    USB_REQ_TYPE_DEVICE_FW_BT_REMOVE    = 0xB2,
    USB_REQ_TYPE_DEVICE_GET_DTOP_DSTATE = 0xBA,
    USB_REQ_TYPE_DEVICE_GET_WIFI_DSTATE = 0xBB,
    USB_REQ_TYPE_DEVICE_GET_BT_DSTATE   = 0xBC,
    USB_REQ_TYPE_DEVICE_GET_RUNSYS      = 0xBD,
    USB_REQ_TYPE_DEVICE_GET_ROM_VERSION = 0xCA,
    USB_REQ_TYPE_DEVICE_GET_ROM_ERRNO   = 0xCB,
    USB_REQ_TYPE_DEVICE_SET_SOC_RESET   = 0xDA,
} HAL_USB_CDC_CMD;

//#define HAL_CMD_STATE_TAB    \
//        X_MACROS(USB_REQ_TYPE_DEVICE_FW_DTOP_DL_ADDR,   0xA4)\
//        X_MACROS(USB_REQ_TYPE_DEVICE_FW_DTOP_STARTPC,   0xA5)\
//        X_MACROS(USB_REQ_TYPE_DEVICE_FW_WIFI_DL_ADDR,   0xA6)\
//        X_MACROS(USB_REQ_TYPE_DEVICE_FW_WIFI_STARTPC,   0xA7)\
//        X_MACROS(USB_REQ_TYPE_DEVICE_FW_BT_DL_ADDR,     0xA8)\
//        X_MACROS(USB_REQ_TYPE_DEVICE_FW_BT_STARTPC,     0xA9)\
//        X_MACROS(USB_REQ_TYPE_DEVICE_FW_DTOP_DL,        0xAA)\
//        X_MACROS(USB_REQ_TYPE_DEVICE_FW_DTOP_DL_COMP,   0xAB)\
//        X_MACROS(USB_REQ_TYPE_DEVICE_FW_WIFI_DL,        0xAC)\
//        X_MACROS(USB_REQ_TYPE_DEVICE_FW_WIFI_DL_COMP,   0xAD)\
//        X_MACROS(USB_REQ_TYPE_DEVICE_FW_BT_DL,          0xAE)\
//        X_MACROS(USB_REQ_TYPE_DEVICE_FW_BT_DL_COMP,     0xAF)\
//        X_MACROS(USB_REQ_TYPE_DEVICE_FW_DTOP_REMOVE,    0xB0)\
//        X_MACROS(USB_REQ_TYPE_DEVICE_FW_WIFI_REMOVE,    0xB1)\
//        X_MACROS(USB_REQ_TYPE_DEVICE_FW_BT_REMOVE,      0xB2)\
//        X_MACROS(USB_REQ_TYPE_DEVICE_GET_DTOP_DSTATE,   0xBA)\
//        X_MACROS(USB_REQ_TYPE_DEVICE_GET_WIFI_DSTATE,   0xBB)\
//        X_MACROS(USB_REQ_TYPE_DEVICE_GET_BT_DSTATE,     0xBC)\
//        X_MACROS(USB_REQ_TYPE_DEVICE_GET_RUNSYS,        0xBD)\
//        X_MACROS(USB_REQ_TYPE_DEVICE_GET_ROM_VERSION,   0xCA)\
//        X_MACROS(USB_REQ_TYPE_DEVICE_GET_ROM_ERRNO,     0xCB)\
//        X_MACROS(USB_REQ_TYPE_DEVICE_SET_SOC_RESET,     0xDA)

//typedef enum {
//#define X_MACROS(a, b) a = b,
//    HAL_CMD_STATE_TAB
//#undef X_MACROS
//}HAL_USB_CDC_CMD_TAB;

//HANDLE hal_open_serial_port(const char* com, int BaudRate);
//void hal_close_serial_port(HANDLE hSerial);
void hal_clear_rx_Serial_Port(HANDLE hSerial);
void hal_clear_tx_Serial_Port(HANDLE hSerial);
UINT8 hal_serial_port_write(HANDLE hSerial, char* buffer, DWORD count);
UINT8 hal_serial_port_read(HANDLE hSerial, UINT8* rx_buffer, DWORD* bytes_read);
UINT8 hal_serial_port_read_ascii(HANDLE hSerial);
UINT8 hal_serial_port_read_hex(HANDLE hSerial);
void hal_device_get_virtual_serial_port(char* extended_port_name);
void hal_pid_vid_get_virtual_serial_port(char* extended_port_name);

#endif
