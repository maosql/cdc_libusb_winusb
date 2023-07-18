#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <direct.h>
#include <stdint.h>

#include "hal_zip.h"
#include "hal_serial.h"
#include "hal_download.h"
#include "hal_htc_mp.h"
#include "hal_libusb.h"

#ifdef HAL_SEND_WPK  /*defined in the wq zip.h file*/
#ifdef HAL_USB_CDC
    const char* cdc_wpk_fw = "./bin_list/fw_cdc_dnld.wpk";
#else
    const char* libusb_wpk_fw = "./bin_list/fw_usb_dnld.wpk";
#endif
#else
#ifdef HAL_USB_CDC
    const char* cdc_wpk_fw = "./bin_list/fw_cdc_dnld.zip";
#else
    const char* libusb_wpk_fw = "./bin_list/fw_usb_dnld.zip";
#endif
#endif

void MP_Test_Initialization(HANDLE hSerial)
{
    hal_chip_check_proc(hSerial, 1);
    hal_wifi_init(hSerial, 0);

    INT32 temp = 0;
    hal_internal_temp_read(hSerial, &temp);
}

void WiFi_Calibration(HANDLE hSerial)
{
    // hal_wifi_set_band(hSerial, 1);
    hal_wifi_set_channel(hSerial, 36, 1, 0);
    hal_wifi_set_mode_rate(hSerial, 5, 1000, 20, 0);
    // hal_wifi_set_tx_power(hSerial, 3);
    // hal_wifi_set_tx_pattern(hSerial, 2, 4);
    hal_wifi_set_target_tx_pwr(hSerial, 1, 1, 3);
    // hal_wifi_always_tx(hSerial, 1);
    hal_wifi_tx_mpdu(hSerial, 1000, 1000);
    // hal_wifi_calc_tssi_value(hSerial, 1, 3, 5, 7);
    float pwr_tssi = 0.0f;
    hal_wifi_get_tssi_value(hSerial, &pwr_tssi);

    // INT32 pwr_diff_qdb = 0, tssi_diff_qdb = 0;
    // hal_wifi_get_tssi_value(hSerial, 1, 3, 5, &pwr_diff_qdb, &tssi_diff_qdb);

    INT32 temp = 0;
    hal_internal_temp_read(hSerial, &temp);

    float ppm_set = 1.3f;
    hal_crystal_set_freq_ppm(hSerial, ppm_set);
    hal_wifi_set_rx_gain(hSerial, 0, 1, 1);
    // hal_wifi_always_rx(hSerial, 1);
    // hal_wifi_start_rx_pkt(hSerial);

    float rssi_value = 0.0f;
    hal_wifi_read_rssi_value(hSerial, &rssi_value);
    printf("rssi_value:%f\n", rssi_value);

    INT32 pkt_num = 0;
    hal_wifi_get_rx_pkt_num(hSerial, &pkt_num);
}

void WiFi_performance_verification(HANDLE hSerial)
{
    // hal_wifi_set_band(hSerial, 1);
    hal_wifi_set_channel(hSerial, 37, 1, 0);
    hal_wifi_set_mode_rate(hSerial, 6, 500, 20 , 0);
    // hal_wifi_set_tx_power(hSerial, 3);
    // hal_wifi_set_tx_pattern(hSerial, 1,3);
    hal_wifi_set_target_tx_pwr(hSerial, 1, 1, 3);
    // hal_wifi_always_tx(hSerial, 1);
    hal_wifi_tx_mpdu(hSerial, 1000, 1000);
    // hal_wifi_always_rx(hSerial, 1);
    // hal_wifi_start_rx_pkt(hSerial);

    INT32 pkt_num = 0;
    hal_wifi_get_rx_pkt_num(hSerial, &pkt_num);
}

void Bluetooth_Setup(HANDLE hSerial)
{
    hal_bt_init(hSerial);
}

void Bluetooth_Calibration(HANDLE hSerial)
{
    hal_bt_set_freq(hSerial, 1000);
    hal_bt_set_tx_power(hSerial, 3);
    hal_bt_set_data_rate(hSerial, 1);
    hal_bt_set_tx_pkt(hSerial, 1, 1);
    hal_bt_always_tx(hSerial, 1);
    hal_bt_always_rx(hSerial, 1);
    hal_bt_start_rx_pkt(hSerial);

    INT32 pkt_num = 0;
    hal_bt_get_rx_pkt_num(hSerial, &pkt_num);
}

void Bluetooth_Verification(HANDLE hSerial)
{
    hal_bt_set_freq(hSerial, 500);
    hal_bt_set_tx_power(hSerial, 4);
    hal_bt_set_data_rate(hSerial, 2);
    hal_bt_set_tx_pkt(hSerial, 2, 2);
    hal_bt_always_tx(hSerial, 2);
    hal_bt_always_rx(hSerial, 2);
    hal_bt_start_rx_pkt(hSerial);

    INT32 pkt_num = 0;
    hal_bt_get_rx_pkt_num(hSerial, &pkt_num);
}

void Calibration_Data_Burning(HANDLE hSerial)
{
    UINT8 wifi_calib_get_data[10] = { 0 };
    INT32 wifi_calib_get_len = 0;
    hal_wifi_calib_get(hSerial, wifi_calib_get_data, &wifi_calib_get_len);

    hal_wifi_calib_set(hSerial);

    char bt_calib_get_data[10] = { 0 };
    INT32 bt_calib_get_len = 0;
    hal_bt_calib_get(hSerial, bt_calib_get_data, &bt_calib_get_len);

    hal_bt_calib_set(hSerial);

    INT32 efuse_write_addr_offset = 100;
    char efuse_write_data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    hal_efuse_write_data(hSerial, efuse_write_addr_offset, efuse_write_data, 10);

    INT32 efuse_read_addr_offset = 200;
    char efuse_read_data[10] = { 0 };
    INT32 length = 6;
    hal_efuse_read_data(hSerial, efuse_read_addr_offset, efuse_read_data, length);
}

static void all_api_test(HANDLE hSerial)
{
    //===============dtop
    hal_chip_check_proc(hSerial, 2);

    INT32 r_temp = 0;
    hal_internal_temp_read(hSerial, &r_temp);

    INT32 w_temp = 21;
    hal_ambient_temp_write(hSerial, 0, w_temp);

    INT32 efuse_write_addr_offset = 0x9912345f;
    char efuse_write_data[10] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa};
    hal_efuse_write_data(hSerial, efuse_write_addr_offset, efuse_write_data, 10);

    INT32 efuse_read_addr_offset = 0x9912345f;
    char efuse_read_data[10] = { 0 };
    INT32 length = 6;
    hal_efuse_read_data(hSerial, efuse_read_addr_offset, efuse_read_data, length);

    //===============wifi
    hal_wifi_init(hSerial, 0);
    printf("wifi init done!\n");

    // hal_wifi_set_band(hSerial, 1);

    hal_wifi_set_channel(hSerial, 36, 1, 0);

    hal_wifi_set_mode_rate(hSerial, 5, 1000, 20, 0);

    // hal_wifi_set_tx_power(hSerial, 3);
    hal_wifi_set_target_tx_pwr(hSerial, 1, 1, 3);

    // hal_wifi_calc_tssi_value(hSerial, 1, 3, 5, 7);
    float pwr_tssi = 0.0f;
    hal_wifi_get_tssi_value(hSerial, &pwr_tssi);

    // INT32 pwr_diff_qdb = 0, tssi_diff_qdb = 0;
    // hal_wifi_get_tssi_value(hSerial, 1, 3, 5, &pwr_diff_qdb, &tssi_diff_qdb);

    // hal_wifi_set_tx_pattern(hSerial, 2, 4);

    // hal_crystal_set_cap_code(hSerial, 50);
    float ppm_set = 1.2f;
    hal_crystal_set_freq_ppm(hSerial, ppm_set);

    // hal_wifi_always_tx(hSerial, 1);

    // hal_wifi_always_rx(hSerial, 1);

    hal_wifi_set_rx_gain(hSerial, 0, 0, 1);

    // hal_wifi_start_rx_pkt(hSerial);

    INT32 wifi_pkt_num = 0;
    hal_wifi_get_rx_pkt_num(hSerial, &wifi_pkt_num);
    printf("wifi_pkt_num:%d\n", wifi_pkt_num);

    float rssi_value = 0.0f;
    hal_wifi_read_rssi_value(hSerial, &rssi_value);
    printf("rssi_value:%f\n", rssi_value);

    UINT8 wifi_calib_get_data[10] = { 0 };
    INT32 wifi_calib_get_len = 0;
    hal_wifi_calib_get(hSerial, wifi_calib_get_data, &wifi_calib_get_len);

    hal_wifi_calib_set(hSerial);

    //===============bt
    hal_bt_init(hSerial);

    hal_bt_set_freq(hSerial, 0x12345678);

    hal_bt_set_tx_power(hSerial, 3);

    hal_bt_set_data_rate(hSerial, 1);

    hal_bt_set_tx_pkt(hSerial, 1, 1);

    hal_bt_always_tx(hSerial, 1);

    hal_bt_always_rx(hSerial, 1);

    hal_bt_start_rx_pkt(hSerial);

    INT32 bt_pkt_num = 0;
    hal_bt_get_rx_pkt_num(hSerial, &bt_pkt_num);
    printf("bt_pkt_num:%d\n", bt_pkt_num);

    // char bt_calib_get_data[10] = { 0 };
    // INT32 bt_calib_get_len = 0;
    // hal_bt_calib_get(hSerial, bt_calib_get_data, &bt_calib_get_len);

    // hal_bt_calib_set(hSerial);
    hal_wifi_deinit(hSerial);
    printf("wifi deinit done!\n");
}

static void wifi_test(HANDLE hSerial)
{
    hal_wifi_init(hSerial, 0);
    printf("wifi init done!\n");

    hal_wifi_set_channel(hSerial, 7, 1, 0);

    hal_wifi_set_mode_rate(hSerial, 4, 700, 20, 0);

    //tx on
    // hal_wifi_always_tx(hSerial, 1);

    //tx off
    // hal_wifi_always_tx(hSerial, 0);

    INT32 wifi_pkt_num = 0;
    hal_wifi_get_rx_pkt_num(hSerial, &wifi_pkt_num);
    printf("wifi_pkt_num:%d\n", wifi_pkt_num);

    hal_wifi_deinit(hSerial);
    printf("wifi deinit done!\n");
}

static void bt_test(HANDLE hSerial)
{
    hci_en_tx_pkt_test_cmd_t param = { 0 };
    UINT8 rx_data[256] = { 0 };

    param.en = 0x1;
    param.tx_mode = 0x2;

    hal_bt_dtest_start(hSerial, param, rx_data);
    printf("bt_rx_data ->> ");
    for (UINT8 i = 0; i < 7; i++) {
        printf("%02x ", rx_data[i]);
    }
    printf("\r\n");
}

#ifdef HAL_USB_CDC
int hal_kiwi_cdc_test(void)
{
    HANDLE hSerial = NULL;
    UINT8 err = 0;
    err = DUT_usbcdc_init(&hSerial, cdc_wpk_fw);
    if (err) {
        return 1;
    }

    printf("====================== test API ======================\n");
    
    // all_api_test(hSerial);

    wifi_test(hSerial);
    bt_test(hSerial);

    Sleep(100);
    if (DUT_usbcdc_close(hSerial)) {
        printf("DUT close filed\n");
    } else {
        printf("DUT close sucess\n");
    }

    return 0;
}
#else
int hal_kiwi_libusb_test(void)
{
    hal_wifi_dev_t device = NULL;
    int ret = 0;

    ret = DUT_libusb_init(&device, libusb_wpk_fw);
    if (ret) {
        if (DUT_libusb_close(device)) {
            printf("DUT close filed\n");
        }
        else {
            printf("DUT close sucess\n");
        }
    }

    printf("====================== test API ======================\n");

    //all_api_test();

    wifi_test(device);
    bt_test(device);

    Sleep(100);
    if (DUT_libusb_close(device)) {
        printf("DUT close filed\n");
    }
    else {
        printf("DUT close sucess\n");
    }

    return 0;
}
#endif

int main(void)
{
#ifdef HAL_USB_CDC
    printf("cdc test\n");
    hal_kiwi_cdc_test();
#else
    printf("libusb test\n");
    hal_kiwi_libusb_test();
#endif

    printf("The program is finished, please press any key to continue. . .");
    int ch = getchar();
    if (ch == EOF) {
        printf("Reached end of file or error occurred.\n");
    }
    else {
        printf("\n");
    }

    return 0;
}