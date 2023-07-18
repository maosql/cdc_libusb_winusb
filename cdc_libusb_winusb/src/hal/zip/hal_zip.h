#pragma once
#ifndef __HAL_ZIP_H__
#define __HAL_ZIP_H__

#include "unzip.h"
#include "hal_libusb.h"

#define HAL_SEND_WPK

void hal_zip_list(unzFile zipFile);

UINT8 hal_is_bin_file(const char* filename);

UINT8 hal_is_zip_file(const char* filename);

UINT8 hal_is_source_bin_file(unzFile zipFile, const char* file_to_extract);

UINT32 hal_get_zip_size(unzFile zip_file, const char* file_to_extract);

int hal_send_zip(HANDLE hSerial, unzFile zip_file, const char* file_to_extract);

int hal_read_wpk(const char* wpk_file);

BOOL hal_libusb_send_zip(hal_wifi_libusb_t* device, UINT8 endpoint, unzFile zip_file, const char* file_to_extract);

#endif