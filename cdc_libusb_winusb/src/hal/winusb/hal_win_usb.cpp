#include <windows.h>
#include <winusb.h>
#include <iostream>
#include <stdio.h>
#include <setupapi.h>
#include <initguid.h>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "winusb.lib")

#include "hal_win_usb.h"

#define MAX_USB_DEVICES 10

#define HAL_WIFI_DEVICE_IDENTIFIER "vid_0ffe&pid_0002"

// 将 char* 转换为 LPCWSTR
LPCWSTR ConvertToLPCWSTR(const char* str)
{
    int length = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    LPWSTR wideStr = (LPWSTR)malloc(length * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, str, -1, wideStr, length);
    return wideStr;
}

static const GUID GUID_DEVINTERFACE_USB_DEVICE =
{ 0xA5DCBF10L, 0x6530, 0x11D2, {0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED} };

BOOL hal_wifi_enum(char path_list[][MAX_PATH], int max_count, int* found_count)
{
	HDEVINFO hDevInfo;
	SP_DEVICE_INTERFACE_DATA DevIntfData;
	PSP_DEVICE_INTERFACE_DETAIL_DATA DevIntfDetailData;
	SP_DEVINFO_DATA DevData;

	DWORD dwSize;
	DWORD dwMemberIdx;
	int count = 0;

	BOOL ret = FALSE;

	hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_USB_DEVICE, NULL, 0, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		printf("SetupDiGetClassDevs failed\n");
		return FALSE;
	}

	DevIntfData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	dwMemberIdx = 0;

	// Prepare to enumerate all device interfaces for the device information
	// set that we retrieved with SetupDiGetClassDevs(..)
	DevIntfData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	dwMemberIdx = 0;

	// Next, we will keep calling this SetupDiEnumDeviceInterfaces(..) until this
	// function causes GetLastError() to return  ERROR_NO_MORE_ITEMS. With each
	// call the dwMemberIdx value needs to be incremented to retrieve the next
	// device interface information.

	SetupDiEnumDeviceInterfaces(hDevInfo, NULL, (LPGUID)&GUID_DEVINTERFACE_USB_DEVICE,
		dwMemberIdx, &DevIntfData);

	while (GetLastError() != ERROR_NO_MORE_ITEMS)
	{

		// As a last step we will need to get some more details for each
		// of device interface information we are able to retrieve. This
		// device interface detail gives us the information we need to identify
		// the device (VID/PID), and decide if it's useful to us. It will also
		// provide a DEVINFO_DATA structure which we can use to know the serial
		// port name for a virtual com port.

		DevData.cbSize = sizeof(DevData);

		// Get the required buffer size. Call SetupDiGetDeviceInterfaceDetail with
		// a NULL DevIntfDetailData pointer, a DevIntfDetailDataSize
		// of zero, and a valid RequiredSize variable. In response to such a call,
		// this function returns the required buffer size at dwSize.

		SetupDiGetDeviceInterfaceDetail(
			hDevInfo, &DevIntfData, NULL, 0, &dwSize, NULL);

		// Allocate memory for the DeviceInterfaceDetail struct. Don't forget to
		// deallocate it later!
		DevIntfDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize);
		if (DevIntfDetailData == NULL)
		{
			return false;
		}

		DevIntfDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

		if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevIntfData,
			DevIntfDetailData, dwSize, &dwSize, &DevData))
		{
			// Finally we can start checking if we've found a useable device,
			// by inspecting the DevIntfDetailData->DevicePath variable.
			// The DevicePath looks something like this:
			//
			// \\?\usb#vid_04d8&pid_0033#5&19f2438f&0&2#{a5dcbf10-6530-11d2-901f-00c04fb951ed}
			//
			// printf("%s\n", DevIntfDetailData->DevicePath);
			// check for hard-coded vendor/product ids

			if ((count < max_count) && (strstr(DevIntfDetailData->DevicePath, HAL_WIFI_DEVICE_IDENTIFIER) != NULL))
			{
				memset(path_list[count], 0, MAX_PATH);
				memcpy_s(path_list[count], MAX_PATH, DevIntfDetailData->DevicePath, strlen(DevIntfDetailData->DevicePath));
				count++;
				ret = TRUE;
			}
		}
		HeapFree(GetProcessHeap(), 0, DevIntfDetailData);

		// Continue looping
		SetupDiEnumDeviceInterfaces(
			hDevInfo, NULL, &GUID_DEVINTERFACE_USB_DEVICE, ++dwMemberIdx, &DevIntfData);
	}

	SetupDiDestroyDeviceInfoList(hDevInfo);

	if (found_count) {
		*found_count = count;
	}

	return ret;
}

BOOL InitializeWinUSB(HANDLE hDevice, UCHAR interfaceNumber, PWINUSB_INTERFACE_HANDLE pWinUSBHandle)
{
    // 打开设备接口
    WINUSB_INTERFACE_HANDLE winusbHandle;
    BOOL success = WinUsb_Initialize(hDevice, &winusbHandle);
    if (!success) {
        printf("Failed to initialize WinUSB\n");
        return FALSE;
    }

    // 获取接口描述符
    USB_INTERFACE_DESCRIPTOR interfaceDescriptor;
    success = WinUsb_QueryInterfaceSettings(winusbHandle, 0, &interfaceDescriptor);
    if (!success) {
        printf("Failed to get interface descriptor\n");
        WinUsb_Free(winusbHandle);
        return FALSE;
    }

    // 根据接口号选择要使用的接口
    for (UCHAR i = 0; i < interfaceDescriptor.bNumEndpoints; i++) {
        WINUSB_PIPE_INFORMATION pipeInformation;
        success = WinUsb_QueryPipe(winusbHandle, 0, i, &pipeInformation);
        if (!success) {
            printf("Failed to get endpoint descriptor\n");
            WinUsb_Free(winusbHandle);
            return FALSE;
        }

        // 根据不同的接口号进行操作
        if (interfaceDescriptor.bInterfaceNumber == interfaceNumber) {
            // 这里可以执行与指定接口相关的操作
            // ...

            *pWinUSBHandle = winusbHandle;  // 返回 WinUSB 句柄
            return TRUE;
        }
    }

    // 如果没有找到指定的接口号
    printf("Interface not found\n");
    WinUsb_Free(winusbHandle);
    return FALSE;
}

BOOL OpenWinUSBDevice(const char* DevicePath, hal_wifi_dev_t* phDevice)
{
    HANDLE deviceHandle;

    hal_wifi_device_t* device = (hal_wifi_device_t*)malloc(sizeof(hal_wifi_device_t));
    if (device == NULL)
    {
        printf("Opening USB device: malloc failed\n");
        return FALSE;
    }

    ZeroMemory(device, sizeof(hal_wifi_device_t));

    // open driver
    deviceHandle = CreateFile(DevicePath,
        GENERIC_WRITE | GENERIC_READ,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
        NULL);

    if (deviceHandle == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        printf("Failed to open device. Error code: %lu\n", error);
        free(device);
        return FALSE;
    }

    WINUSB_INTERFACE_HANDLE winusbHandle;
    InitializeWinUSB(deviceHandle, 1, &winusbHandle);

    return TRUE;
    // init WinUSB
    //WINUSB_INTERFACE_HANDLE interfaceHandle;
    //BOOL result = WinUsb_Initialize(deviceHandle, &interfaceHandle);
    //if (!result) {
    //    DWORD error = GetLastError();
    //    printf("Failed to initialize WinUSB. Error code: %lu\n", error);
    //    free(device);
    //    CloseHandle(deviceHandle);
    //    return FALSE;
    //}

    //device->deviceHandle = deviceHandle;
    //device->interfaceHandle = interfaceHandle;
    //device->control_overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    //device->bulk_out_overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    //device->bulk_in_overlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    //snprintf(device->path, MAX_PATH, "%s", DevicePath);

    //*phDevice = device;

    //return result;
}

BOOL hal_wifi_get_rom_ver(hal_wifi_dev_t pdevice, hal_wifi_rom_ver_t* ver)
{
    // Start trasnsfer
    WINUSB_SETUP_PACKET setup_packet;
    DWORD bytes = 0;
    BOOL result;
    hal_wifi_device_t* device = (hal_wifi_device_t*)pdevice;

    memset(&setup_packet, 0, sizeof(setup_packet));
    setup_packet.RequestType = USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE;
    setup_packet.Request = HAL_WINUSB_ID_GET_ROM_VER;
    setup_packet.Length = sizeof(hal_wifi_rom_ver_t);

    result = WinUsb_ControlTransfer(device->interfaceHandle, setup_packet, (PUCHAR)ver, sizeof(hal_wifi_rom_ver_t), NULL, &device->control_overlap);
    if (!result)
    {
        if (GetLastError() != ERROR_IO_PENDING)
        {
            printf("get_rom_version error: %ld\n", GetLastError());
            snprintf(device->err_msg, MAX_PATH, "get_rom_version error: %ld", GetLastError());
        }
        else
        {
            result = WinUsb_GetOverlappedResult(device->interfaceHandle, &device->control_overlap, &bytes, TRUE);
            if (!result)
            {
                printf("get_rom_version, wait err:%ld\n", GetLastError());
                snprintf(device->err_msg, MAX_PATH, "get_rom_version, wait err:%ld", GetLastError());
            }
            else
            {
                printf("get_rom_version succeed, bytes:%d ver:%d.%d build_time:%d:%d\n", bytes, ver->major, ver->minor, ver->build_hr, ver->build_min);
            }
        }
    }

    return result;
}

const char* hal_wifi_get_err_msg(hal_wifi_dev_t pdevice)
{
    hal_wifi_device_t* device = (hal_wifi_device_t*)pdevice;

    return device->err_msg;
}