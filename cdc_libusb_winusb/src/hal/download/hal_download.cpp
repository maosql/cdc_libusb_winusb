#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>

#include "hal_htc_mp.h"
#include "hal_serial.h"
#include "hal_libusb.h"
#include "hal_download.h"
#include "unzip.h"
#include "hal_zip.h"

const char* libusb_sbr_bin      = "sbr_core0.bin";
const char* sbr_source_bin      = "sbr_cdc_core0.bin";
const char* iomap_source_bin    = "iomap_config.bin";
const char* dtop_source_bin     = "fw_dtop_mp_test_core0.bin";
const char* wifi_source_bin     = "fw_wifi_mp_test_core1.bin";
const char* bt_source_bin       = "fw_bt_1_1_mp_test_core3.bin";

UINT32 bin_checksum = 0;
UINT32 libusb_bin_checksum = 0;

static void hal_cdc_begin(HANDLE hSerial)
{
    char wqkl_cdc[] = "WQKL-CDC";
    printf("===================WQKL-CDC===================\n");
    hal_serial_port_write(hSerial, wqkl_cdc, sizeof(wqkl_cdc));
    printf("Send -> %s\n", wqkl_cdc);
}

static void hal_cmd_pack(UINT8 cmd, UINT32 para, UINT8* cmd_pkg)
{
    cmd_pkg[0] = cmd;
    cmd_pkg[1] = 0;
    cmd_pkg[2] = 0;
    cmd_pkg[3] = 0;

    if (para == 0) {
        cmd_pkg[4] = 0;
        cmd_pkg[5] = 0;
        cmd_pkg[6] = 0;
        cmd_pkg[7] = 0;
    }
    else {
        cmd_pkg[4] = (para & 0xff) >> 0;
        cmd_pkg[5] = (para & 0xff00) >> 8;
        cmd_pkg[6] = (para & 0xff0000) >> 16;
        cmd_pkg[7] = (para & 0xff000000) >> 24;
    }

    cmd_pkg[8] = 0;
    cmd_pkg[9] = 0;
    cmd_pkg[10] = 0;
    cmd_pkg[11] = 0;
}

void hal_cdc_send_cmd(HANDLE hSerial, UINT8 para_cnt, ...)
{
    bool cmd_flag = 0;
    UINT8 count = para_cnt - 2;
    UINT8 cmd_pkg[12] = { 0 };
    va_list ps;

    va_start(ps, para_cnt);
    UINT8 cmd = va_arg(ps, int);

    switch (cmd) {
    case USB_REQ_TYPE_DEVICE_FW_DTOP_DL_ADDR:
        cmd_flag = 1;
        break;
    case USB_REQ_TYPE_DEVICE_FW_DTOP_STARTPC:
        cmd_flag = 1;
        break;
    case USB_REQ_TYPE_DEVICE_FW_WIFI_DL_ADDR:
        cmd_flag = 1;
        break;
    case USB_REQ_TYPE_DEVICE_FW_WIFI_STARTPC:
        cmd_flag = 1;
        break;
    case USB_REQ_TYPE_DEVICE_FW_BT_DL_ADDR:
        cmd_flag = 1;
        break;
    case USB_REQ_TYPE_DEVICE_FW_BT_STARTPC:
        cmd_flag = 1;
        break;
    case USB_REQ_TYPE_DEVICE_FW_DTOP_DL:
        cmd_flag = 1;
        break;
    case USB_REQ_TYPE_DEVICE_FW_DTOP_DL_COMP:
        cmd_flag = 1;
        break;
    case USB_REQ_TYPE_DEVICE_FW_WIFI_DL:
        cmd_flag = 1;
        break;
    case USB_REQ_TYPE_DEVICE_FW_WIFI_DL_COMP:
        cmd_flag = 1;
        break;
    case USB_REQ_TYPE_DEVICE_FW_BT_DL:
        cmd_flag = 1;
        break;
    case USB_REQ_TYPE_DEVICE_FW_BT_DL_COMP:
        cmd_flag = 1;
        break;
    case USB_REQ_TYPE_DEVICE_FW_DTOP_REMOVE:
        cmd_flag = 1;
        break;
    case USB_REQ_TYPE_DEVICE_FW_WIFI_REMOVE:
        cmd_flag = 1;
        break;
    case USB_REQ_TYPE_DEVICE_FW_BT_REMOVE:
        cmd_flag = 1;
        break;
    case USB_REQ_TYPE_DEVICE_GET_DTOP_DSTATE:
        cmd_flag = 1;
        break;
    case USB_REQ_TYPE_DEVICE_GET_WIFI_DSTATE:
        cmd_flag = 1;
        break;
    case USB_REQ_TYPE_DEVICE_GET_BT_DSTATE:
        cmd_flag = 1;
        break;
    case USB_REQ_TYPE_DEVICE_GET_RUNSYS:
        cmd_flag = 1;
        break;
    case USB_REQ_TYPE_DEVICE_GET_ROM_VERSION:
        cmd_flag = 1;
        break;
    case USB_REQ_TYPE_DEVICE_GET_ROM_ERRNO:
        cmd_flag = 1;
        break;
    case USB_REQ_TYPE_DEVICE_SET_SOC_RESET:
        cmd_flag = 1;
        break;
    default:
        printf("Without this cmd\n");
    }

    if (cmd_flag == 1) {
        cmd_flag = 0;

        if (count == 0) {
            hal_cmd_pack(cmd, 0, cmd_pkg);
        }
        else if (count > 1) {
            printf("cmd parameter error");
            exit(0);
        }
        else {
            UINT32 date = va_arg(ps, UINT32);
            hal_cmd_pack(cmd, date, cmd_pkg);
            printf("Send -> cmd:0x%x, date:0x%x\n", cmd, date);

            for (UINT8 i = 0; i < sizeof(cmd_pkg); i++) {
                printf("0x%02x ", cmd_pkg[i]);
            }
            printf("\n");
        }

        hal_serial_port_write(hSerial, (char*)cmd_pkg, sizeof(cmd_pkg));
        memset(cmd_pkg, 0, sizeof(cmd_pkg));

        bin_checksum = 0;
    }

    Sleep(20);
}

DWORD hal_read_bin(const char* source_file, FILE** fp)
{
    DWORD file_size;
    errno_t err;

    // open binary file
    err = fopen_s(fp, source_file, "rb, ccs=UTF-8");
    if (err) {
        printf("Error opening file %s\n", source_file);
        return 0;
    }

    // get file size
    fseek(*fp, 0, SEEK_END);
    file_size = ftell(*fp);
    printf("file_size:%ld\n", file_size);
    fseek(*fp, 0, SEEK_SET);

    return file_size;
}

static void read_bin_date(char* buffer, long file_size)
{
    long i = 0;
    for (i = 0; i < file_size; i++)
    {
        printf("%02x ", (UINT8)*buffer);
        buffer++;
        if ((i + 1) % (BUF_SIZE - 8) == 0) {
            printf("\n");
        }
    }

    if (i % (BUF_SIZE - 8) != 0) { // Handles cases where the last row contains less than 16 data.
        printf("\n");
    }

    hal_printf("\nfile info done\n");
}

static void print_bin_date(char* buffer, long file_size)
{
    long i = 0;

    for (i = 0; i < file_size; i++)
    {
        printf("%02x ", (UINT8)*buffer);
        buffer++;
    }

    printf("\n");
}

static void hal_bin_pack(char* buffer_start, char* buffer_end, UINT32 id, UINT8* bin_pkg)
{
    UINT16 sum = 0;

    char* p = buffer_start;

    for (p = buffer_start; p < buffer_end; p++) {
        sum += (UINT8)*p & 0xff;
    }

    hal_printf("sum:0x%04x\n", sum);

    bin_checksum += sum;
    bin_checksum = (bin_checksum & 0xffff);

    bin_pkg[0] = id & 0xff;
    bin_pkg[1] = (id & 0xff00) >> 8;

    bin_pkg[2] = sum & 0xff;
    bin_pkg[3] = (sum & 0xff00) >> 8;

    bin_pkg[4] = 0;
    bin_pkg[5] = 0;
    bin_pkg[6] = 0;
    bin_pkg[7] = 1;
}

static void hal_bin_unpack(HANDLE hSerial, char* buffer, DWORD buffer_len, UINT16 distance)
{
    UINT32 id = 0;
    char* tx_buffer;
    DWORD buffer_node = 0;
    UINT32 buffer_rema = 0;
    UINT32 buffer_divisor = 0;
    UINT8 bin_pkg[8] = { 0 };

    buffer_rema = buffer_len % distance;
    buffer_divisor = buffer_len / distance;

    hal_printf("buffer_len:%d\n", buffer_len);
    hal_printf("distance:%d\n", distance);
    hal_printf("buffer_rema:%d\n", buffer_rema);
    hal_printf("buffer_divisor:%d\n", buffer_divisor);

    tx_buffer = (char*)malloc((distance + 8));
    if (!tx_buffer) {
        printf("No space was requested\n");
        exit(0);
    }
    memset(tx_buffer, 0, (distance + 8));

    for (id = 0; id <= buffer_divisor; id++)
    {
        buffer_node = distance * id;

        if (id == buffer_divisor) {
            memcpy(tx_buffer, (buffer + buffer_node), buffer_rema);
            hal_bin_pack((buffer + buffer_node), (buffer + buffer_node + buffer_rema), id, bin_pkg);
            memcpy((tx_buffer + buffer_rema), (char*)bin_pkg, 8);

            //print_bin_date(tx_buffer, (buffer_rema + 8));
            hal_serial_port_write(hSerial, tx_buffer, (buffer_rema + 8));
            memset(tx_buffer, 0, (distance + 8));
            memset(bin_pkg, 0, sizeof(bin_pkg));
        }
        else {

            memcpy(tx_buffer, (buffer + buffer_node), distance);
            hal_bin_pack((buffer + buffer_node), (buffer + buffer_node + distance), id, bin_pkg);
            memcpy((tx_buffer + distance), (char*)bin_pkg, 8);

            //print_bin_date(tx_buffer, (distance + 8));
            hal_serial_port_write(hSerial, tx_buffer, (distance + 8));
            memset(tx_buffer, 0, (distance + 8));
            memset(bin_pkg, 0, sizeof(bin_pkg));
        }
    }

    free(tx_buffer);
}

int hal_cdc_send_bin(HANDLE hSerial, FILE* fp, DWORD file_size)
{
    char* buffer = NULL;
    size_t count;

    // allocate memory
    buffer = (char*)malloc(file_size);
    if (!buffer) {
        printf("No space was requested\n");
        return 1;
    }
    memset(buffer, 0, file_size);

    // read file contents into memory
    count = fread_s(buffer, file_size, file_size, 1, fp);

    // close file
    fclose(fp);

    //#ifdef HAL_DEBUG
    //    read_bin_date(buffer, file_size);
    //#endif

    // process file contents
    if (count) {
        hal_bin_unpack(hSerial, buffer, file_size, (BUF_SIZE - 16));
    }

    // free the memory
    free(buffer);

    return 0;
}

static BOOL hal_cdc_send_sbr(HANDLE hSerial, unzFile zipFile, const char* source_bin)
{
    UINT8 __rom__ = 0x1;
    UINT32 sbr_core0_lma = 0x80022000;
    UINT32 sbr_core0_bin_len = 0;
    UINT32 sbr_core0_vma = 0x80022000;
    UINT8 core = 0xff;
    static FILE* fp = NULL;
    UINT8 download_status = 0;

    if (hal_is_source_bin_file(zipFile, source_bin)) {
        return FALSE;
    }

    sbr_core0_bin_len = hal_get_zip_size(zipFile, source_bin);
    if (!sbr_core0_bin_len) {
        return FALSE;
    }

    printf("\n\n======================= SBR ========================\n");
    hal_cdc_send_cmd(hSerial, 3, USB_REQ_TYPE_DEVICE_GET_ROM_VERSION, __rom__);
    hal_serial_port_read_hex(hSerial);
    hal_cdc_send_cmd(hSerial, 3, USB_REQ_TYPE_DEVICE_FW_DTOP_DL_ADDR, sbr_core0_lma);
    hal_serial_port_read_hex(hSerial);
    hal_cdc_send_cmd(hSerial, 3, USB_REQ_TYPE_DEVICE_FW_DTOP_DL, sbr_core0_bin_len);
    hal_serial_port_read_hex(hSerial);
    hal_cdc_send_cmd(hSerial, 3, USB_REQ_TYPE_DEVICE_FW_DTOP_STARTPC, sbr_core0_vma);
    hal_serial_port_read_hex(hSerial);

    printf("\nDownload sbr_cdc_core0.bin...\n");
    printf("Download sbr_cdc_core0.bin...\n");
    printf("Download sbr_cdc_core0.bin...\n\n");
    hal_send_zip(hSerial, zipFile, source_bin);

    hal_cdc_send_cmd(hSerial, 3, USB_REQ_TYPE_DEVICE_FW_DTOP_DL_COMP, bin_checksum);
    download_status = hal_serial_port_read_hex(hSerial);

    if (download_status == 0x04) {
        printf("============== download sbr success!!! ============\n\n\n");
    }
    else {
        printf("============== download sbr failed!!! ==============\n\n\n");
        return FALSE;
    }

    Sleep(10);
    return TRUE;
}

static BOOL hal_cdc_send_iomap(HANDLE hSerial, unzFile zipFile, const char* source_bin)
{
    UINT8 __rom__ = 0x1;
    UINT32 iomap_config_lma = 0x8001ed80;
    UINT32 iomap_config_bin_len = 0;
    UINT32 iomap_config_vma = 0x8001ed80;
    UINT8 core = 0xff;
    static FILE* fp = NULL;
    UINT8 download_status = 0;

    if (hal_is_source_bin_file(zipFile, source_bin)) {
        return FALSE;
    }

    iomap_config_bin_len = hal_get_zip_size(zipFile, source_bin);
    if (!iomap_config_bin_len) {
        return FALSE;
    }

    printf("\n\n======================= IOMAP ========================\n");
    hal_cdc_send_cmd(hSerial, 3, USB_REQ_TYPE_DEVICE_GET_ROM_VERSION, __rom__);
    hal_serial_port_read_hex(hSerial);
    hal_cdc_send_cmd(hSerial, 3, USB_REQ_TYPE_DEVICE_FW_DTOP_DL_ADDR, iomap_config_lma);
    hal_serial_port_read_hex(hSerial);
    hal_cdc_send_cmd(hSerial, 3, USB_REQ_TYPE_DEVICE_FW_DTOP_DL, iomap_config_bin_len);
    hal_serial_port_read_hex(hSerial);
    hal_cdc_send_cmd(hSerial, 3, USB_REQ_TYPE_DEVICE_FW_DTOP_STARTPC, iomap_config_vma);
    hal_serial_port_read_hex(hSerial);

    printf("\nDownload iomap_cdc_core0.bin...\n");
    printf("Download iomap_cdc_core0.bin...\n");
    printf("Download iomap_cdc_core0.bin...\n\n");
    hal_send_zip(hSerial, zipFile, source_bin);

    hal_cdc_send_cmd(hSerial, 3, USB_REQ_TYPE_DEVICE_FW_DTOP_DL_COMP, bin_checksum);
    download_status = hal_serial_port_read_hex(hSerial);

    if (download_status == 0x04) {
        printf("============== download iomap success!!! ============\n\n\n");
    }
    else {
        printf("============== download iomap failed!!! ==============\n\n\n");
        return FALSE;
    }

    Sleep(10);
    return TRUE;
}

static BOOL hal_cdc_send_dtop(HANDLE hSerial, unzFile zipFile, const char* source_bin)
{
    UINT8 __rom__ = 0x1;
    UINT32 dtop_core0_lma = 0x80000000;
    UINT32 dtop_core0_bin_len = 0;
    UINT32 dtop_core0_vma = 0x80000000;
    UINT8 core = 0xff;
    static FILE* fp = NULL;
    UINT8 download_status = 0;

    if (hal_is_source_bin_file(zipFile, source_bin)) {
        return FALSE;
    }

    dtop_core0_bin_len = hal_get_zip_size(zipFile, source_bin);
    if (!dtop_core0_bin_len) {
        return FALSE;
    }

    printf("======================= DTOP =======================\n");
    hal_cdc_send_cmd(hSerial, 3, USB_REQ_TYPE_DEVICE_GET_ROM_VERSION, __rom__);
    hal_serial_port_read_hex(hSerial);
    hal_cdc_send_cmd(hSerial, 3, USB_REQ_TYPE_DEVICE_FW_DTOP_DL_ADDR, dtop_core0_lma);
    hal_serial_port_read_hex(hSerial);
    hal_cdc_send_cmd(hSerial, 3, USB_REQ_TYPE_DEVICE_FW_DTOP_DL, dtop_core0_bin_len);
    hal_serial_port_read_hex(hSerial);
    hal_cdc_send_cmd(hSerial, 3, USB_REQ_TYPE_DEVICE_FW_DTOP_STARTPC, dtop_core0_vma);
    hal_serial_port_read_hex(hSerial);

    printf("\nDownload fw_dtop_core0.bin...\n");
    printf("Download fw_dtop_core0.bin...\n");
    printf("Download fw_dtop_core0.bin...\n\n");
    hal_send_zip(hSerial, zipFile, source_bin);

    hal_cdc_send_cmd(hSerial, 3, USB_REQ_TYPE_DEVICE_FW_DTOP_DL_COMP, bin_checksum);
    Sleep(2);
    download_status = hal_serial_port_read_hex(hSerial);

    if (download_status == 0x04) {
        printf("============= download dtop success!!! =============\n\n\n");
    }
    else {
        printf("============= download dtop failed!!! ==============\n\n\n");
        return FALSE;
    }

    Sleep(10);
    return TRUE;
}

static BOOL hal_cdc_send_wifi(HANDLE hSerial, unzFile zipFile, const char* source_bin)
{
    UINT8 __rom__ = 0x1;
    UINT32 dtop_core0_lma = 0x7FF00000;
    UINT32 dtop_core0_bin_len = 0;
    UINT32 dtop_core0_vma = 0x7FF00000;
    UINT8 core = 0xff;
    static FILE* fp = NULL;
    UINT8 download_status = 0;

    if (hal_is_source_bin_file(zipFile, source_bin)) {
        return FALSE;
    }

    dtop_core0_bin_len = hal_get_zip_size(zipFile, source_bin);
    if (!dtop_core0_bin_len) {
        return FALSE;
    }

    printf("======================= WIFI =======================\n");
    hal_cdc_send_cmd(hSerial, 3, USB_REQ_TYPE_DEVICE_GET_ROM_VERSION, __rom__);
    hal_serial_port_read_hex(hSerial);
    hal_cdc_send_cmd(hSerial, 3, USB_REQ_TYPE_DEVICE_FW_DTOP_DL_ADDR, dtop_core0_lma);
    hal_serial_port_read_hex(hSerial);
    hal_cdc_send_cmd(hSerial, 3, USB_REQ_TYPE_DEVICE_FW_DTOP_DL, dtop_core0_bin_len);
    hal_serial_port_read_hex(hSerial);
    hal_cdc_send_cmd(hSerial, 3, USB_REQ_TYPE_DEVICE_FW_DTOP_STARTPC, dtop_core0_vma);
    hal_serial_port_read_hex(hSerial);

    printf("\nDownload fw_wifi_mp_test_core1.bin...\n");
    printf("Download fw_wifi_mp_test_core1.bin...\n");
    printf("Download fw_wifi_mp_test_core1.bin...\n\n");
    hal_send_zip(hSerial, zipFile, source_bin);

    hal_cdc_send_cmd(hSerial, 3, USB_REQ_TYPE_DEVICE_FW_DTOP_DL_COMP, bin_checksum);
    Sleep(2);
    download_status = hal_serial_port_read_hex(hSerial);

    if (download_status == 0x04) {
        printf("============== download wifi success!!! =============\n\n\n");
    }
    else {
        printf("============== download wifi failed!!! ==============\n\n\n");
        return FALSE;
    }

    Sleep(10);
    return TRUE;
}

static BOOL hal_cdc_send_bt(HANDLE hSerial, unzFile zipFile, const char* source_bin)
{
    UINT8 __rom__ = 0x1;
    UINT32 dtop_core0_lma = 0x102079a8;
    UINT32 dtop_core0_bin_len = 0;
    UINT32 dtop_core0_vma = 0x102079a8;
    UINT8 core = 0xff;
    static FILE* fp = NULL;
    UINT8 download_status = 0;

    if (hal_is_source_bin_file(zipFile, source_bin)) {
        return FALSE;
    }

    dtop_core0_bin_len = hal_get_zip_size(zipFile, source_bin);
    if (!dtop_core0_bin_len) {
        return FALSE;
    }

    printf("========================= BT =========================\n");
    hal_cdc_send_cmd(hSerial, 3, USB_REQ_TYPE_DEVICE_GET_ROM_VERSION, __rom__);
    hal_serial_port_read_hex(hSerial);
    hal_cdc_send_cmd(hSerial, 3, USB_REQ_TYPE_DEVICE_FW_DTOP_DL_ADDR, dtop_core0_lma);
    hal_serial_port_read_hex(hSerial);
    hal_cdc_send_cmd(hSerial, 3, USB_REQ_TYPE_DEVICE_FW_DTOP_DL, dtop_core0_bin_len);
    hal_serial_port_read_hex(hSerial);
    hal_cdc_send_cmd(hSerial, 3, USB_REQ_TYPE_DEVICE_FW_DTOP_STARTPC, dtop_core0_vma);
    hal_serial_port_read_hex(hSerial);

    printf("\nDownload fw_bt_1_1_mp_test_core3.bin...\n");
    printf("Download fw_bt_1_1_mp_test_core3.bin...\n");
    printf("Download fw_bt_1_1_mp_test_core3.bin...\n\n");
    hal_send_zip(hSerial, zipFile, source_bin);

    hal_cdc_send_cmd(hSerial, 3, USB_REQ_TYPE_DEVICE_FW_DTOP_DL_COMP, bin_checksum);
    Sleep(2);
    download_status = hal_serial_port_read_hex(hSerial);

    if (download_status == 0x04) {
        printf("================ download bt success!!! ==============\n\n\n");
    }
    else {
        printf("================ download bt failed!!! ===============\n\n\n");
        return FALSE;
    }

    Sleep(10);
    return TRUE;
}

static UINT8 hal_cdc_send_wpk(HANDLE hSerial, const char* wpk_fw)
{
    int ret = 0;
    UINT8 is_zip_file = 1;
    is_zip_file = hal_is_zip_file(wpk_fw);

    if (is_zip_file == 1) {
        return 1;
    }

    unzFile zipFile = unzOpen(wpk_fw);
    if (zipFile == NULL)
    {
        printf("Failed to open ZIP file: %s\n", wpk_fw);
        return 1;
    }

    //send WQKL-CDC
    hal_cdc_begin(hSerial);
    hal_serial_port_read_ascii(hSerial);

    //download bin file
    ret = hal_cdc_send_sbr(hSerial, zipFile, sbr_source_bin);
    if (!ret) {
        printf("Failed to download %s file.\n", sbr_source_bin);
        return FALSE;
    }

    ret = hal_cdc_send_iomap(hSerial, zipFile, iomap_source_bin);
    if (!ret) {
        printf("no %s file.\n", iomap_source_bin);
    }

    ret = hal_cdc_send_dtop(hSerial, zipFile, dtop_source_bin);
    if (!ret) {
        printf("Failed to download %s file.\n", dtop_source_bin);
        return FALSE;
    }

    ret = hal_cdc_send_wifi(hSerial, zipFile, wifi_source_bin);
    if (!ret) {
        printf("Failed to download %s file.\n", wifi_source_bin);
        return FALSE;
    }

    ret = hal_cdc_send_bt(hSerial, zipFile, bt_source_bin);
    if (!ret) {
        printf("Failed to download %s file.\n", bt_source_bin);
        return FALSE;
    }

    unzClose(zipFile);

    return TRUE;
}

UINT8 DUT_usbcdc_init(HANDLE* hSerial, const char* wpk_fw)
{
    UINT8 ret = 0;
    char com[20] = { 0 };

    hal_pid_vid_get_virtual_serial_port(com);
    if (strlen(com) > 0) {
        printf("Virtual serial port found at: %s\n", com);
    }
    else {
        printf("No virtual serial port found.\n");
        return 1;
    }

    //open serial
    *hSerial = hal_open_serial_port(com, 2000000);
    if (!*hSerial) {
        printf("open hSerial is fail\n");
        return 1;
    }

    //download all bin and get status
    ret = hal_cdc_send_wpk(*hSerial, wpk_fw);
    if (!ret) {
        exit(-1);
    }

    Sleep(100);

    return 0;
}

UINT8 DUT_usbcdc_close(HANDLE hSerial)
{
    printf("\n======================== DONE =======================\n");
    //close serial
    return hal_close_serial_port(hSerial);
}


//========================libusb=========================

static BOOL hal_libusb_send_sbr(hal_wifi_libusb_t* device, unzFile zipFile, const char* source_bin)
{
    int result = false;
    UINT32 libusb_sbr_bin_len = 0;
    hal_wifi_state_t state = { 0 };
    UINT8 fw_dl_state[] = { 0 };

    if (hal_is_source_bin_file(zipFile, source_bin)) {
        return FALSE;
    }

    libusb_sbr_bin_len = hal_get_zip_size(zipFile, source_bin);
    if (!libusb_sbr_bin_len) {
        return FALSE;
    }

    result = hal_libusb_download_addr(device, HAL_LIBUSB_ID_FW_DTOP_DL_ADDR, 0x80022000);
    if (result < 0) {
        return FALSE;
    }

    result = hal_libusb_set_startpc(device, HAL_LIBUSB_ID_DTOP_STARTPC, 0x80022000);
    if (result < 0) {
        return FALSE;
    }

    result = hal_libusb_download_start(device, HAL_LIBUSB_ID_FW_DTOP_DL, libusb_sbr_bin_len, fw_dl_state);
    if ((result < 0) || (fw_dl_state[0] != HAL_LIBUSB_FW_DL_READY)) {
        return FALSE;
    }

    //printf("\nlibusb Download sbr_core0.bin...\n");
    //printf("libusb Download sbr_core0.bin...\n");
    //printf("libusb Download sbr_core0.bin...\n\n");
    result = hal_libusb_send_zip(device, HAL_LIBUSB_EP1_FW_DL, zipFile, source_bin);
    if (!result) {
        return FALSE;
    }

    result = hal_libusb_download_finish(device, HAL_LIBUSB_ID_FW_DTOP_DL_COMP, libusb_bin_checksum);
    libusb_bin_checksum = 0;
    if (result < 0) {
        return FALSE;
    }

    Sleep(200);
    for (int i = 0; i < 20; i++)
    {
        result = hal_wifi_libusb_get_state(device, HAL_LIBUSB_ID_GET_BOOTROM_DSTATE, &state);
        if (result >= 0) {
            if (state.state == 1)
            {
                //printf("get_state succeed, bytes_read:%d state:%d\n", result, state.state);
                break;
            }
            else {
                //printf("get_state succeed, bytes_read:%d state:%d\n", result, state.state);
            }

            if (i == 19) {
                return FALSE;
            }

            Sleep(200);
            continue;
        }
    }
    Sleep(500);

    return TRUE;
}

static BOOL hal_libusb_send_iomap(hal_wifi_libusb_t* device, unzFile zipFile, const char* source_bin)
{
    int result = false;
    UINT32 libusb_iomap_bin_len = 0;
    hal_wifi_state_t state = { 0 };
    UINT8 fw_dl_state[] = { 0 };

    if (hal_is_source_bin_file(zipFile, source_bin)) {
        return FALSE;
    }

    printf("\n======================fw_iomap======================\n");

    libusb_iomap_bin_len = hal_get_zip_size(zipFile, source_bin);
    if (!libusb_iomap_bin_len) {
        return FALSE;
    }

    result = hal_libusb_download_addr(device, HAL_LIBUSB_ID_FW_DTOP_DL_ADDR, 0x8001ed80);
    if (result < 0) {
        return FALSE;
    }

    result = hal_libusb_set_startpc(device, HAL_LIBUSB_ID_DTOP_STARTPC, 0x8001ed80);
    if (result < 0) {
        return FALSE;
    }

    result = hal_libusb_download_start(device, HAL_LIBUSB_ID_FW_DTOP_DL, libusb_iomap_bin_len, fw_dl_state);
    if ((result < 0) || (fw_dl_state[0] != HAL_LIBUSB_FW_DL_READY)) {
        return FALSE;
    }

    result = hal_libusb_send_zip(device, HAL_LIBUSB_EP1_FW_DL, zipFile, source_bin);
    if (!result) {
        return FALSE;
    }

    printf("\nlibusb Download iomap_config.bin...\n");
    printf("libusb Download iomap_config.bin...\n");
    printf("libusb Download iomap_config.bin...\n\n");
    result = hal_libusb_download_finish(device, HAL_LIBUSB_ID_FW_DTOP_DL_COMP, libusb_bin_checksum);
    libusb_bin_checksum = 0;
    if (result < 0) {
        return FALSE;
    }

    Sleep(200);
    for (int i = 0; i < 20; i++)
    {
        result = hal_wifi_libusb_get_state(device, HAL_LIBUSB_ID_GET_DTOP_DSTATE, &state);
        if (result >= 0) {
            if (state.state == 1)
            {
                printf("get_state succeed, bytes_read:%d state:%d\n", result, state.state);
                break;
            }
            else {
                printf("get_state succeed, bytes_read:%d state:%d\n", result, state.state);
            }

            if (i == 19) {
                return FALSE;
            }

            Sleep(200);
            continue;
        }
    }
    Sleep(500);

    printf("======================fw_iomap done======================\n");

    return TRUE;
}

static BOOL hal_libusb_send_dtop(hal_wifi_libusb_t* device, unzFile zipFile, const char* source_bin)
{
    int result = false;
    UINT32 libusb_dtop_bin_len = 0;
    hal_wifi_state_t state = { 0 };
    UINT8 fw_dl_state[] = { 0 };

    if (hal_is_source_bin_file(zipFile, source_bin)) {
        return FALSE;
    }

    printf("\n======================fw_dtop======================\n");

    libusb_dtop_bin_len = hal_get_zip_size(zipFile, source_bin);
    if (!libusb_dtop_bin_len) {
        return FALSE;
    }

    result = hal_libusb_download_addr(device, HAL_LIBUSB_ID_FW_DTOP_DL_ADDR, 0x80000000);
    if (result < 0) {
        return FALSE;
    }

    result = hal_libusb_set_startpc(device, HAL_LIBUSB_ID_DTOP_STARTPC, 0x80000000);
    if (result < 0) {
        return FALSE;
    }

    result = hal_libusb_download_start(device, HAL_LIBUSB_ID_FW_DTOP_DL, libusb_dtop_bin_len, fw_dl_state);
    if ((result < 0) || (fw_dl_state[0] != HAL_LIBUSB_FW_DL_READY)) {
        return FALSE;
    }

    result = hal_libusb_send_zip(device, HAL_LIBUSB_EP1_FW_DL, zipFile, source_bin);
    if (!result) {
        return FALSE;
    }

    printf("\nlibusb Download fw_dtop_core0.bin...\n");
    printf("libusb Download fw_dtop_core0.bin...\n");
    printf("libusb Download fw_dtop_core0.bin...\n\n");
    result = hal_libusb_download_finish(device, HAL_LIBUSB_ID_FW_DTOP_DL_COMP, libusb_bin_checksum);
    libusb_bin_checksum = 0;
    if (result < 0) {
        return FALSE;
    }

    Sleep(200);
    for (int i = 0; i < 20; i++)
    {
        result = hal_wifi_libusb_get_state(device, HAL_LIBUSB_ID_GET_DTOP_DSTATE, &state);
        if (result >= 0) {
            if (state.state == 1)
            {
                printf("get_state succeed, bytes_read:%d state:%d\n", result, state.state);
                break;
            }
            else {
                printf("get_state succeed, bytes_read:%d state:%d\n", result, state.state);
            }

            if (i == 19) {
                return FALSE;
            }

            Sleep(200);
            continue;
        }
    }
    Sleep(500);

    printf("======================fw_dtop done======================\n");

    return TRUE;
}

static BOOL hal_libusb_send_wifi(hal_wifi_libusb_t* device, unzFile zipFile, const char* source_bin)
{
    int result = false;
    UINT32 libusb_wifi_bin_len = 0;
    hal_wifi_state_t state = { 0 };
    UINT8 fw_dl_state[] = { 0 };
    UINT8 runtime_data[] = { 0 };

    if (hal_is_source_bin_file(zipFile, source_bin)) {
        return FALSE;
    }

    printf("\n======================fw_wifi======================\n");

    libusb_wifi_bin_len = hal_get_zip_size(zipFile, source_bin);
    if (!libusb_wifi_bin_len) {
        return FALSE;
    }

    result = hal_libusb_runtime(device, HAL_LIBUSB_ID_GET_RUNSYS, 0, runtime_data);
    if (result < 0) {
        return FALSE;
    }
    printf("wifi runtime_data:%d\n", runtime_data[0]);

    result = hal_libusb_download_addr(device, HAL_LIBUSB_ID_FW_WIFI_DL_ADDR, 0x7ff00000);
    if (result < 0) {
        return FALSE;
    }

    result = hal_libusb_set_startpc(device, HAL_LIBUSB_ID_FW_WIFI_STARTPC, 0x7ff00000);
    if (result < 0) {
        return FALSE;
    }

    result = hal_libusb_download_start(device, HAL_LIBUSB_ID_FW_WIFI_DL, libusb_wifi_bin_len, fw_dl_state);
    if ((result < 0) || (fw_dl_state[0] != HAL_LIBUSB_FW_DL_READY)) {
        return FALSE;
    }

    printf("\nlibusb Download fw_wifi_os_1_1_core1.bin...\n");
    printf("libusb Download fw_wifi_os_1_1_core1.bin...\n");
    printf("libusb Download fw_wifi_os_1_1_core1.bin...\n\n");
    result = hal_libusb_send_zip(device, HAL_LIBUSB_EP7_FW_DL, zipFile, source_bin);
    if (!result) {
        return FALSE;
    }

    result = hal_libusb_download_finish(device, HAL_LIBUSB_ID_FW_WIFI_DL_COMP, libusb_bin_checksum);
    libusb_bin_checksum = 0;
    if (result < 0) {
        return FALSE;
    }

    Sleep(200);
    for (int i = 0; i < 20; i++)
    {
        result = hal_wifi_libusb_get_state(device, HAL_LIBUSB_ID_GET_WIFI_DSTATE, &state);
        if (result >= 0) {
            if (state.state == 1)
            {
                printf("get_state succeed, bytes_read:%d state:%d\n", result, state.state);
                break;
            }
            else {
                printf("get_state succeed, bytes_read:%d state:%d\n", result, state.state);
            }

            if (i == 19) {
                return FALSE;
            }

            Sleep(200);
            continue;
        }
    }
    Sleep(500);

    printf("\n======================fw_wifi done======================\n");

    return TRUE;
}

static BOOL hal_libusb_send_bt(hal_wifi_libusb_t* device, unzFile zipFile, const char* source_bin)
{
    int result = false;
    UINT32 libusb_bt_bin_len = 0;
    hal_wifi_state_t state = { 0 };
    UINT8 fw_dl_state[] = { 0 };

    if (hal_is_source_bin_file(zipFile, source_bin)) {
        return FALSE;
    }

    printf("\n======================fw_bt======================\n");

    libusb_bt_bin_len = hal_get_zip_size(zipFile, source_bin);
    if (!libusb_bt_bin_len) {
        return FALSE;
    }

    result = hal_libusb_download_addr(device, HAL_LIBUSB_ID_FW_WIFI_DL_ADDR, 0x102079a8);
    if (result < 0) {
        return FALSE;
    }

    result = hal_libusb_set_startpc(device, HAL_LIBUSB_ID_FW_WIFI_STARTPC, 0x102079a8);
    if (result < 0) {
        return FALSE;
    }

    result = hal_libusb_download_start(device, HAL_LIBUSB_ID_FW_WIFI_DL, libusb_bt_bin_len, fw_dl_state);
    if ((result < 0) || (fw_dl_state[0] != HAL_LIBUSB_FW_DL_READY)) {
        return FALSE;
    }

    printf("\nlibusb Download fw_bt_1_1_core3.bin...\n");
    printf("libusb Download fw_bt_1_1_core3.bin...\n");
    printf("libusb Download fw_bt_1_1_core3.bin...\n\n");
    result = hal_libusb_send_zip(device, HAL_LIBUSB_EP7_FW_DL, zipFile, source_bin);
    if (!result) {
        return FALSE;
    }

    result = hal_libusb_download_addr(device, HAL_LIBUSB_ID_FW_BT_DL_ADDR, 0x102079a8);
    if (result < 0) {
        return FALSE;
    }

    result = hal_libusb_set_startpc(device, HAL_LIBUSB_ID_FW_BT_STARTPC, 0x102079a8);
    if (result < 0) {
        return FALSE;
    }

    result = hal_libusb_download_start(device, HAL_LIBUSB_ID_FW_BT_DL, libusb_bt_bin_len, fw_dl_state);
    if ((result < 0) || (fw_dl_state[0] != HAL_LIBUSB_FW_DL_READY)) {
        return FALSE;
    }

    result = hal_libusb_download_finish(device, HAL_LIBUSB_ID_FW_BT_DL_COMP, 0);
    libusb_bin_checksum = 0;
    if (result < 0) {
        return FALSE;
    }

    Sleep(200);
    for (int i = 0; i < 20; i++)
    {
        result = hal_wifi_libusb_get_state(device, HAL_LIBUSB_ID_GET_BT_DSTATE, &state);
        if (result >= 0) {
            if (state.state == 1)
            {
                printf("get_state succeed, bytes_read:%d state:%d\n", result, state.state);
                break;
            }
            else {
                printf("get_state succeed, bytes_read:%d state:%d\n", result, state.state);
            }

            if (i == 19) {
                return FALSE;
            }

            Sleep(200);
            continue;
        }
    }
    Sleep(500);

    printf("======================fw_bt done======================\n");

    return TRUE;
}

static UINT8 hal_libusb_send_wpk(hal_wifi_dev_t pDevice, const char* wpk_fw)
{
    BOOL ret = 0;
    UINT8 is_zip_file = 1;
    hal_wifi_libusb_t* device = (hal_wifi_libusb_t*)pDevice;

    is_zip_file = hal_is_zip_file(wpk_fw);
    if (is_zip_file == 1) {
        return FALSE;
    }

    unzFile zipFile = unzOpen(wpk_fw);
    if (zipFile == NULL)
    {
        printf("Failed to open ZIP file: %s\n", wpk_fw);
        return FALSE;
    }

    //download bin file
    ret = hal_libusb_send_sbr(device, zipFile, libusb_sbr_bin);
    if (!ret) {
        printf("Failed to download %s file.\n", libusb_sbr_bin);
        return FALSE;
    }

    ret = hal_libusb_send_iomap(device, zipFile, iomap_source_bin);
    if (!ret) {
        printf("no %s file.\n", iomap_source_bin);
    }

    ret = hal_libusb_send_dtop(device, zipFile, dtop_source_bin);
    if (!ret) {
        printf("Failed to download %s file.\n", dtop_source_bin);
        return FALSE;
    }

    ret = hal_libusb_send_bt(device, zipFile, bt_source_bin);
    if (!ret) {
        printf("Failed to download %s file.\n", bt_source_bin);
        return FALSE;
    }

    ret = hal_libusb_send_wifi(device, zipFile, wifi_source_bin);
    if (!ret) {
        printf("Failed to download %s file.\n", wifi_source_bin);
        return FALSE;
    }

    unzClose(zipFile);

    return TRUE;
}

UINT8 DUT_libusb_init(hal_wifi_dev_t* device, const char* wpk_fw)
{
    int ret = 0;
    hal_wifi_rom_ver_t rom_ver;

    ret = hal_wifi_libusb_init(device);
    if (!ret) {
        printf("Failed:init libusb\n");
        return 1;
    }

    ZeroMemory(&rom_ver, sizeof(rom_ver));
    ret = hal_wifi_libusb_get_rom_ver(*device, &rom_ver);
    if (ret < 0) {
        printf("Failed:get rom ver\n");
        return 1;
    }

    ret = hal_libusb_send_wpk(*device, wpk_fw);
    if (!ret) {
        exit(-1);
    }

    return 0;
}

UINT8 DUT_libusb_close(hal_wifi_dev_t device)
{
    printf("\n======================== DONE =======================\n");
    //close libusb
    return hal_wifi_libusb_close(device);
}