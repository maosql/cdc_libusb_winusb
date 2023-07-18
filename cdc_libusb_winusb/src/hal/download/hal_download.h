#pragma once
#ifndef __HAL_DOWNLOAD_H__
#define __HAL_DOWNLOAD_H__

#include <windows.h>
#include "unzip.h"

void hal_cdc_send_cmd(HANDLE hSerial, UINT8 para_cnt, ...);
DWORD hal_read_bin(const char* source_file, FILE** fp);
int hal_cdc_send_bin(HANDLE hSerial, FILE* fp, DWORD file_size);

#endif