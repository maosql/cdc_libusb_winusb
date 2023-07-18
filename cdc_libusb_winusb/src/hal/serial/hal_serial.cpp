#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <setupapi.h>
#include <devguid.h>

#include "hal_serial.h"
#include "hal_htc_mp.h"

#pragma comment(lib, "setupapi.lib")

#define VENDOR_ID 0x0FFE  // 替换为您的USB设备的供应商ID
#define PRODUCT_ID 0x0002 // 替换为您的USB设备的产品ID

DEFINE_GUID(GUID_DEVINTERFACE_COMPORT, 0x86e0d1e0L, 0x8089, 0x11d0, 0x9c, 0xe4, 0x08, 0x00, 0x3e, 0x30, 0x1f, 0x73);

/*
 * param com:       //eg:COM1，COM2 , >COM9 use \\\\.\\COMx
 * param BaudRate:  //Common value：CBR_9600、CBR_19200、CBR_38400、CBR_115200、CBR_230400、CBR_460800
 */
HANDLE hal_open_serial_port(const char* com, int BaudRate)
{
    HANDLE hSerial;
    DCB dcbSerialParams = { 0 };
    COMMTIMEOUTS timeouts = { 0 };
    DWORD dwEventMask = 0;

    // open the serial port
    hSerial = CreateFile(com, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hSerial == INVALID_HANDLE_VALUE) {
        printf("Error opening serial port,0x%08x.\n", GetLastError());
        return 0;
    }

    // Set the read/write cache size
    if (!SetupComm(hSerial, BUF_SIZE, BUF_SIZE)) {
        printf("SetupComm fail\r\n");
    }

    // get the serial port parameters
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        printf("Error getting serial port state.\n");
        CloseHandle(hSerial);
        return 0;
    }
    dcbSerialParams.BaudRate = BaudRate;
    dcbSerialParams.Parity = NOPARITY;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;

    // set the serial port parameters
    if (!SetCommState(hSerial, &dcbSerialParams)) {
        printf("Error setting serial port state.\n");
        CloseHandle(hSerial);
        return 0;
    }

    dwEventMask |= EV_RXCHAR;  // 监听接收缓冲区中有数据事件
    dwEventMask |= EV_ERR;     // 监听错误事件

    SetCommMask(hSerial, dwEventMask);

    // set the timeouts
    GetCommTimeouts(hSerial, &timeouts);
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = 10;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 1;
    timeouts.WriteTotalTimeoutMultiplier = 1;
    SetCommTimeouts(hSerial, &timeouts);

    return hSerial;
}

UINT8 hal_close_serial_port(HANDLE hSerial)
{
    if (hSerial != INVALID_HANDLE_VALUE) {
        // Stop serial communication.
        CancelIo(hSerial);
        PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);

        // close the serial port
        if (hSerial != INVALID_HANDLE_VALUE) {
            if (CloseHandle(hSerial)) {
                printf("\nSerial port closed successfully.\n");
                return 0;
            }
            else {
                DWORD errorCode = GetLastError();
                printf("\nSerial port closed filed, error code :%d.\n", errorCode);
            }
            hSerial = INVALID_HANDLE_VALUE;
        } else {
            printf("\nThe device has been turned off.\n");
        }
    }

    return 1;
}

void hal_clear_rx_Serial_Port(HANDLE hSerial) {
    if (hSerial == INVALID_HANDLE_VALUE) {
        printf("Serial port is not open.\n");
        return;
    }
    PurgeComm(hSerial, PURGE_RXCLEAR); // Clear the read serial port buffer
    hal_printf("Serial port buffer cleared.\n");
}

void hal_clear_tx_Serial_Port(HANDLE hSerial) {
    if (hSerial == INVALID_HANDLE_VALUE) {
        printf("Serial port is not open.\n");
        return;
    }
    PurgeComm(hSerial, PURGE_TXCLEAR); // Clear the write serial port buffer
    hal_printf("Serial port buffer cleared.\n");
}

UINT8 hal_serial_port_read_ascii(HANDLE hSerial)
{
    if (hSerial == INVALID_HANDLE_VALUE) {
        printf("Serial port is not open.\n");
        return 0 ;
    }

    DWORD bytes_read = 0;
    UINT8 incoming_data[1024] = { 0 }; // buffer to store incoming data
    int buffer_size = 0;

    memset(incoming_data, 0, sizeof(incoming_data));

    // read incoming data
    if (!ReadFile(hSerial, incoming_data + buffer_size, sizeof(incoming_data) - buffer_size - 1, &bytes_read, NULL)) {
        printf("Error reading from serial port.\n");
        CloseHandle(hSerial);
        return 0;
    }

    buffer_size += bytes_read;

    // 过滤无用数据0x00
    for (int i = 0; i < buffer_size; i++) {
        if (incoming_data[i] == 0x00) {
            memmove(incoming_data + i, incoming_data + i + 1, buffer_size - i - 1);
            buffer_size--;
            i--;
        }
    }

    if (!buffer_size) {
        printf("Recv -> NULL\n");
        return 0;
    }

    hal_printf("Received %d bytes:\n", buffer_size);
    printf("Recv -> %s\n\n", incoming_data);

    return 1;
}

UINT8 hal_serial_port_read_hex(HANDLE hSerial)
{
    if (hSerial == INVALID_HANDLE_VALUE) {
        printf("Serial port is not open.\n");
        return 0;
    }

    DWORD bytes_read = 0;
    UINT8 incoming_data[1024] = { 0 }; // buffer to store incoming data
    int buffer_size = 0;
    UINT8 download_status = 1;

    memset(incoming_data, 0, sizeof(incoming_data));

    // read incoming data
    if (!ReadFile(hSerial, incoming_data + buffer_size, sizeof(incoming_data) - buffer_size - 1, &bytes_read, NULL)) {
        printf("Error reading from serial port.\n");
        CloseHandle(hSerial);
        return 0;
    }
    
    buffer_size += bytes_read;

    // filter out useless data:0x00
    for (int i = 0; i < buffer_size; i++) {
        if (incoming_data[i] == 0x00) {
            memmove(incoming_data + i, incoming_data + i + 1, buffer_size - i - 1);
            buffer_size--;
            i--;
        }
    }

    if (!buffer_size) {
        printf("Recv -> NULL\n");
        return 0;
    }

    hal_printf("Received %d bytes:\n", buffer_size);
    printf("Recv -> ");
    for (int j = 0; j < buffer_size; j++) {
        printf("0x:%02x  ", incoming_data[j]);

        if (incoming_data[j] == 0x4) {
            download_status = 0x04;
        }
    }
    printf("\n\n");

    return download_status;
}

UINT8 hal_serial_port_read(HANDLE hSerial, UINT8* rx_buffer, DWORD* bytes_read)
{
    if (hSerial == INVALID_HANDLE_VALUE) {
        printf("Serial port is not open.\n");
        return 0;
    }

    // read incoming data
    if (!ReadFile(hSerial, rx_buffer, 512, bytes_read, NULL)) {
        printf("Error reading from serial port.\n");
        CloseHandle(hSerial);
        return 0;
    }

    if (!bytes_read) {
        return 0;
    }

    return 1;
}

UINT8 hal_serial_port_write(HANDLE hSerial, char* buffer, DWORD count)
{
    if (hSerial == INVALID_HANDLE_VALUE) {
        printf("Serial port is not open.\n");
        return 0;
    }

    DWORD bytes_written;
    hal_clear_rx_Serial_Port(hSerial);
    hal_clear_tx_Serial_Port(hSerial);

    // send data
    if (!WriteFile(hSerial, buffer, count, &bytes_written, NULL)) {
        printf("Error writing to serial port.\n");
        return 0;
    }
    hal_printf("Sent %d bytes.\n", bytes_written);

    return 1;
}

void hal_pid_vid_get_virtual_serial_port(char* extended_port_name)
{
    HDEVINFO device_info_set;
    SP_DEVINFO_DATA device_info_data;
    DWORD index = 0;
    char port_name[MAX_PATH] = { 0 };

    // 初始化设备信息集合
    device_info_set = SetupDiGetClassDevs(&GUID_DEVINTERFACE_COMPORT, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (device_info_set == INVALID_HANDLE_VALUE)
    {
        printf("Failed to get device information set.\n");
        return;
    }

    // 遍历设备信息集合
    device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);
    while (SetupDiEnumDeviceInfo(device_info_set, index++, &device_info_data))
    {
        DWORD property_type;
        TCHAR property_buffer[MAX_PATH];
        DWORD required_size;

        // 获取设备VID和PID
        DWORD vid, pid;
        if (!SetupDiGetDeviceRegistryProperty(device_info_set, &device_info_data, SPDRP_HARDWAREID, &property_type, (BYTE*)property_buffer, sizeof(property_buffer), &required_size))
        {
            printf("Failed to retrieve hardware ID.\n");
            continue;
        }

        // 解析VID和PID
        if (sscanf(property_buffer, "USB\\VID_%4X&PID_%4X", &vid, &pid) != 2) {
            continue;
        }
        //printf("vid:0x%x\n", vid);
        //printf("pid:0x%x\n\n", pid);

        // 判断设备的VID和PID是否匹配
        if (vid == VENDOR_ID && pid == PRODUCT_ID) {
            // 获取设备端口名
            if (!SetupDiGetDeviceRegistryProperty(device_info_set, &device_info_data, SPDRP_FRIENDLYNAME, &property_type, \
                (BYTE*)property_buffer, sizeof(property_buffer), &required_size)) {
                printf("Failed to retrieve friendly name.\n");
                continue;
            }

            // 提取端口号
            char* port = strstr(property_buffer, "(COM");
            //printf("port:%s\n", port);
            if (port != NULL) {
                if (sscanf(port, "(%[^)])", port_name) != 1) {
                    printf("Invalid input string\n");
                    break;
                }

                sprintf(extended_port_name, "\\\\.\\%s", port_name);
                //printf("Port: %s\n", port);
                break;
            }
        }
    }

    SetupDiDestroyDeviceInfoList(device_info_set);
}

void hal_device_get_virtual_serial_port(char* extended_port_name)
{
    char port_name[MAX_PATH] = { 0 };
    HDEVINFO device_info_set;
    SP_DEVINFO_DATA device_info_data;
    DWORD index = 0;

    // 初始化设备信息集合
    device_info_set = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, NULL, NULL, DIGCF_PRESENT);
    if (device_info_set == INVALID_HANDLE_VALUE)
    {
        strcpy(port_name, ""); // 未找到虚拟串口时，返回空字符串
    }

    // 遍历设备信息集合
    device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);
    while (SetupDiEnumDeviceInfo(device_info_set, index++, &device_info_data))
    {
        char port_name_value[MAX_PATH];
        DWORD port_name_size = sizeof(port_name_value);

        // 获取设备描述信息
        char device_description[MAX_PATH];
        if (SetupDiGetDeviceRegistryProperty(device_info_set, &device_info_data, SPDRP_DEVICEDESC, NULL, (PBYTE)device_description, sizeof(device_description), NULL))
        {
            //printf("device_description:%s\n", device_description);
            // 判断是否为虚拟串口
            if (strstr(device_description, "USB 串行设备") != NULL)
            {
                // 获取端口名称
                if (SetupDiGetDeviceRegistryProperty(device_info_set, &device_info_data, SPDRP_FRIENDLYNAME, NULL, (PBYTE)port_name_value, sizeof(port_name_value), NULL))
                {
                    // 提取端口号
                    //printf("port_name_value:%s\n", port_name_value);
                    char* port_number_start = strchr(port_name_value, '(');
                    char* port_number_end = strchr(port_name_value, ')');
                    if (port_number_start != NULL && port_number_end != NULL && port_number_start < port_number_end)
                    {
                        strncpy(port_name, port_number_start + 1, port_number_end - port_number_start - 1);
                        port_name[port_number_end - port_number_start - 1] = '\0';
                        sprintf(extended_port_name, "\\\\.\\%s", port_name);
                        SetupDiDestroyDeviceInfoList(device_info_set);
                    }
                }
            }
        }
    }

    SetupDiDestroyDeviceInfoList(device_info_set);

    strcpy(port_name, ""); // 未找到虚拟串口时，返回空字符串
}