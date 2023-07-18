#include <stdio.h>
#include <stdlib.h>

#include "libusb.h"
#include "hal_libusb.h"

#define VENDOR_ID       0x0ffe  // 替换为你的设备的 Vendor ID
#define PRODUCT_ID      0x0002  // 替换为你的设备的 Product ID

UINT8 hal_wifi_libusb_init(hal_wifi_dev_t* phDevice)
{
    libusb_context* context = NULL;
    libusb_device_handle* deviceHandle = NULL;

    hal_wifi_libusb_t* device = (hal_wifi_libusb_t*)malloc(sizeof(hal_wifi_libusb_t));
    if (device == NULL) {
        printf("Opening libusb device: malloc failed\n");
        return FALSE;
    }

    // 初始化 libusb
    if (libusb_init(&context) != LIBUSB_SUCCESS) {
        printf("Failed to initialize libusb\n");
        return FALSE;
    }

    // 打开设备
    deviceHandle = libusb_open_device_with_vid_pid(context, VENDOR_ID, PRODUCT_ID);
    if (deviceHandle == NULL) {
        printf("Failed to open device\n");
        libusb_exit(context);
        return FALSE;
    }

    // 初始化设备接口
    if (libusb_claim_interface(deviceHandle, HAL_LIBUSB_INTERFACE0) != LIBUSB_SUCCESS) {
        printf("Failed to claim interface\n");
        libusb_close(deviceHandle);
        libusb_exit(context);
        return FALSE;
    }

    if (libusb_claim_interface(deviceHandle, HAL_LIBUSB_INTERFACE1) != LIBUSB_SUCCESS) {
        printf("Failed to claim interface\n");
        libusb_close(deviceHandle);
        libusb_exit(context);
        return FALSE;
    }

    device->context = context;
    device->deviceHandle = deviceHandle;

    *phDevice = device;

    return TRUE;
}

BOOL hal_wifi_libusb_close(hal_wifi_dev_t phDevice)
{
    hal_wifi_libusb_t* device = (hal_wifi_libusb_t*)phDevice;

    UINT8 ret = libusb_release_interface(device->deviceHandle, HAL_LIBUSB_INTERFACE0);
    if (ret) {
        if (ret == LIBUSB_ERROR_INVALID_PARAM) {
            printf("The device handle is invalid or not open,interface0\n");
        } else if (ret == LIBUSB_ERROR_NOT_FOUND) {
            printf("Invalid interface number,interface0\n");
        } else {
            printf("Other errors occurred,interface0\n");
        }
        ret = 1;
    } else {
        ret = 0;
    }

    UINT8 ret1 = libusb_release_interface(device->deviceHandle, HAL_LIBUSB_INTERFACE1);
    if (ret1) {
        if (ret1 == LIBUSB_ERROR_INVALID_PARAM) {
            printf("The device handle is invalid or not open,interface1\n");
        } else if (ret1 == LIBUSB_ERROR_NOT_FOUND) {
            printf("Invalid interface number,interface1\n");
        } else {
            printf("Other errors occurred,interface1\n");
        }
        ret1 = 1;
    } else {
        ret1 = 0;
    }

    // 关闭设备
    libusb_close(device->deviceHandle);

    // 关闭 libusb
    libusb_exit(device->context);

    return ret && ret1;
}

int hal_wifi_libusb_bulk_transfer(hal_wifi_libusb_t* phDevice, UINT8 endpoint, UINT8* data, int data_len)
{
    int transferred = 0;
    int result = libusb_bulk_transfer(phDevice->deviceHandle, endpoint, data, data_len, &transferred, 1000);  // 通过 bulk transfer 发送数据
    if (result != LIBUSB_SUCCESS) {
        printf("Failed to send data: %s\n", libusb_strerror(result));
        return FALSE;
    }
    else {
        //printf("Data sent successfully: %d bytes\n", transferred);
        return transferred;
    }
    return FALSE;
}

int hal_wifi_libusb_control_read(hal_wifi_libusb_t* device, UINT8 Request, PUCHAR data, int data_len)
{
    DWORD bytes = 0;
    int result = 0;

    result = libusb_control_transfer(device->deviceHandle, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        Request, 0, 0, data, data_len, 1000);
    if (result < 0) {
        printf("Failed to libusb control read: %d\n", result);
    }

    return result;
}

int hal_wifi_libusb_control_write(hal_wifi_libusb_t* device, UINT8 Request, USHORT Value, USHORT Index)
{
    DWORD bytes = 0;
    int result = 0;

    result = libusb_control_transfer(device->deviceHandle, LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        Request, Value, Index, NULL, 0, 1000);
    if (result < 0) {
        printf("Failed to libusb control write: %d\n", result);
    }

    return result;
}

int hal_wifi_libusb_control_transfer(hal_wifi_libusb_t* device, UINT8 Request, USHORT Value, USHORT Index, PUCHAR data, int data_len)
{
    DWORD bytes = 0;
    int result = 0;

    result = libusb_control_transfer(device->deviceHandle, LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
        Request, Value, Index, data, data_len, 2000);
    if (result < 0) {
        printf("Failed to libusb control transfer: %d\n", result);
    }

    return result;
}

int hal_libusb_soc_reset(hal_wifi_dev_t phDevice)
{
    int result = 0;
    hal_wifi_libusb_t* device = (hal_wifi_libusb_t*)phDevice;

    result = hal_wifi_libusb_control_write(device, HAL_LIBUSB_ID_SET_SOC_RESET, HAL_LIBUSB_MAGIC_RESET_SOC_VALUE, HAL_LIBUSB_MAGIC_RESET_SOC_INDEX);
    if (result < 0) {
        printf("Failed to soc reset: %d\n", result);
    }

    return result;
}

int hal_wifi_libusb_get_rom_ver(hal_wifi_dev_t phDevice, hal_wifi_rom_ver_t* ver)
{
    DWORD bytes = 0;
    int result = 0;
    hal_wifi_libusb_t* device = (hal_wifi_libusb_t*)phDevice;

    result = hal_wifi_libusb_control_read(device, HAL_LIBUSB_ID_GET_ROM_VERSION, (PUCHAR)ver, sizeof(hal_wifi_rom_ver_t));
    if (result < 0) {
        printf("Failed to get rom ver: %d\n", result);
    } else {
        printf("build_time:%02X:%02X,rom_ver:%02X-%02X\n", ver->build_hr, ver->build_min, ver->major, ver->minor);
    }

    return result;
}

int hal_libusb_runtime(hal_wifi_libusb_t* device, UINT8 Request, UINT32 addr, UINT8* runtime_data)
{
    int result = 0;
    USHORT Value = (((addr) >> 16) & 0xffff);
    USHORT Index = ((addr) & 0xffff);

    result = hal_wifi_libusb_control_transfer(device, Request, Value, Index, runtime_data, sizeof(UINT8));
    if (result < 0) {
        printf("Failed to run time: result:%d, runtime_data:%d\n", result, *runtime_data);
    }

    return result;
}

int hal_libusb_download_addr(hal_wifi_libusb_t* device, UINT8 Request, UINT32 addr)
{
    int result = 0;
    USHORT Value = (((addr) >> 16) & 0xffff);
    USHORT Index = ((addr) & 0xffff);

    result = hal_wifi_libusb_control_write(device, Request, Value, Index);
    if (result < 0) {
        printf("Failed to dlwnload addr: %d\n", result);
    }

    return result;
}

int hal_libusb_download_start(hal_wifi_libusb_t* device, UINT8 Request, UINT32 fw_len, UINT8* fw_dl_state)
{
    int result = 0;
    USHORT Value = (((fw_len) >> 16) & 0xffff);
    USHORT Index = ((fw_len) & 0xffff);

    if (fw_dl_state == NULL) {
        return -1;
    }

    result = hal_wifi_libusb_control_transfer(device, Request, Value, Index, fw_dl_state, sizeof(UINT8));
    if ((result < 0) || (*fw_dl_state != HAL_LIBUSB_FW_DL_READY)) {
        printf("Failed to dlwnload start: result:%d, fw_dl_state:%d\n", result, *fw_dl_state);
    }

    return result;
}

int hal_libusb_set_startpc(hal_wifi_libusb_t* device, UINT8 Request, UINT32 startpc)
{
    int result = 0;
    USHORT Value = (((startpc) >> 16) & 0xffff);
    USHORT Index = ((startpc) & 0xffff);

    result = hal_wifi_libusb_control_write(device, Request, Value, Index);
    if (result < 0) {
        printf("Failed to set startpc: %d\n", result);
    }

    return result;
}

int hal_libusb_download_finish(hal_wifi_libusb_t* device, UINT8 Request, UINT32 checksum)
{
    int result = 0;
    USHORT Value = (UINT16)(checksum & 0xffff);

    result = hal_wifi_libusb_control_write(device, Request, Value, 0);
    if (result < 0) {
        printf("Failed to download finish: %d\n", result);
    }

    return result;
}

int hal_wifi_libusb_get_state(hal_wifi_dev_t phDevice, UINT8 Request, hal_wifi_state_t* state)
{
    int result = 0;
    hal_wifi_libusb_t* device = (hal_wifi_libusb_t*)phDevice;

    result = hal_wifi_libusb_control_read(device, Request, (PUCHAR)state, sizeof(hal_wifi_state_t));
    if (result < 0) {
        printf("Failed to get state: %d\n", result);
    }

    return result;
}