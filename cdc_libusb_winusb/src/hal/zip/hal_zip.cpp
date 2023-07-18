#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <direct.h>

#include "unzip.h"
#include "hal_zip.h"
#include "hal_serial.h"
#include "hal_htc_mp.h"
#include "hal_libusb.h"

extern UINT32 bin_checksum;
extern UINT32 libusb_bin_checksum;

void hal_zip_list(unzFile zipFile)
{
    unzGoToFirstFile(zipFile);
    unz_file_info fileInfo;

    while (unzGetCurrentFileInfo(zipFile, &fileInfo, NULL, 0, NULL, 0, NULL, 0) == UNZ_OK) {
        char fileName[62];
        unzGetCurrentFileInfo(zipFile, &fileInfo, fileName, sizeof(fileName) - 1, NULL, 0, NULL, 0);

        printf("File Name: %s\n", fileName);

        // Move to the next file
        if (unzGoToNextFile(zipFile) != UNZ_OK) {
            break;
        }
    }
}

UINT8 hal_is_bin_file(const char* filename)
{
    // Gets the file extension
    const char* fileExtension = strrchr(filename, '.');

    if (fileExtension == NULL) {
        printf("File extension not found.\n");
        return 1;
    }

    // Check if the file extension is ".bin"
    if (strcmp(fileExtension, ".bin") != 0) {
        printf("File is not a bin file.\n");
        return 1;
    }

    return 0;
}

UINT8 hal_is_zip_file(const char* filename)
{
    // Gets the file extension
    const char* fileExtension = strrchr(filename, '.');

    if (fileExtension == NULL) {
        printf("File extension not found.\n");
        return 1;
    }

#ifdef HAL_SEND_WPK
    // Check if the file extension is ".wpk"
    if (strcmp(fileExtension, ".wpk") != 0) {
        printf("File is not a wpk file.\n");
        return 1;
    }
#else
    if (strcmp(fileExtension, ".zip") != 0) {
        printf("File is not a zip file.\n");
        return 1;
    }
#endif
    return 0;
}

UINT8 hal_is_source_bin_file(unzFile zipFile, const char* file_to_extract)
{
    unzGoToFirstFile(zipFile);
    unz_file_info fileInfo;

    while (unzGetCurrentFileInfo(zipFile, &fileInfo, NULL, 0, NULL, 0, NULL, 0) == UNZ_OK) {
        char fileName[62];
        unzGetCurrentFileInfo(zipFile, &fileInfo, fileName, sizeof(fileName) - 1, NULL, 0, NULL, 0);

        //printf("fn:%s\n", fileName);

        if (!strcmp(fileName, file_to_extract)) {
            return 0;
        }

        // Move to the next file
        if (unzGoToNextFile(zipFile) != UNZ_OK) {
            break;
        }
    }
    return 1;
}

static UINT8 hal_unzip(unzFile zipFile, const char* wpk_file)
{
    char dest_file[100] = { 0 };
    char entryName[MAX_PATH];

    size_t len = strlen(wpk_file);
    if (len > 4) {
        strncpy(dest_file, wpk_file, (len - 4));
        dest_file[(len - 3)] = '\0';
    }
    else {
        strcpy(dest_file, wpk_file);
    }

    if (_mkdir(dest_file)) {
        printf("Warn: mkdir folder,folder already exists.\n");
        return 0;
    }
    else {
        printf("mkdir dest_file-->%s\n", dest_file);
    }

    if (unzGoToFirstFile(zipFile) != UNZ_OK)
    {
        printf("Failed to go to first file in ZIP\n");
        unzClose(zipFile);
        return 1;
    }

    do
    {
        if (unzGetCurrentFileInfo(zipFile, NULL, entryName, sizeof(entryName), NULL, 0, NULL, 0) != UNZ_OK)
        {
            printf("Failed to get current file info in ZIP\n");
            unzClose(zipFile);
            return 1;
        }

        // ¼ÆËãÊä³öÂ·¾¶
        char outputPath[MAX_PATH];
        snprintf(outputPath, sizeof(outputPath), "%s/%s", dest_file, entryName);

        if (unzOpenCurrentFile(zipFile) != UNZ_OK)
        {
            printf("Failed to open current file in ZIP\n");
            unzClose(zipFile);
            return 1;
        }

        FILE* outputFile = fopen(outputPath, "wb");
        if (outputFile == NULL)
        {
            printf("Failed to create output file: %s\n", outputPath);
            unzCloseCurrentFile(zipFile);
            unzClose(zipFile);
            return 1;
        }

        char buffer[4096];
        int bytesRead;
        while ((bytesRead = unzReadCurrentFile(zipFile, buffer, sizeof(buffer))) > 0)
        {
            fwrite(buffer, 1, bytesRead, outputFile);
        }

        fclose(outputFile);

        if (unzCloseCurrentFile(zipFile) != UNZ_OK)
        {
            printf("Failed to close current file in ZIP\n");
            unzClose(zipFile);
            return 1;
        }

        printf("File extracted: %s\n", outputPath);
    } while (unzGoToNextFile(zipFile) == UNZ_OK);

    return 0;
}

static void read_zip_bin_date(char* buffer, long file_size)
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

static void print_zip_bin_date(char* buffer, long file_size)
{
    long i = 0;

    for (i = 0; i < file_size; i++)
    {
        printf("%02x ", (UINT8)*buffer);
        buffer++;
    }

    printf("\n");
}

static void hal_zip_bin_pack(char* buffer_start, char* buffer_end, UINT32 id, UINT8* bin_pkg)
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

static int hal_send_zip_bin(HANDLE hSerial, char* buffer, UINT32 len, UINT32 id)
{
    UINT8 bin_pkg[8] = { 0 };

    hal_zip_bin_pack(buffer, (buffer + len), id, bin_pkg);
    memcpy((buffer + len), (char*)bin_pkg, 8);

    //print_zip_bin_date(buffer, (len + 8));
    hal_serial_port_write(hSerial, buffer, (len + 8));
    memset(buffer, 0, (len + 16));
    memset(bin_pkg, 0, sizeof(bin_pkg));

    return 0;
}

UINT32 hal_get_zip_size(unzFile zip_file, const char* file_to_extract)
{
    // Locate the file of the size you want to get
    if (unzLocateFile(zip_file, file_to_extract, 0) != UNZ_OK) {
        printf("Failed to locate file.\n");
        unzClose(zip_file);
        return 1;
    }

    // get file info
    unz_file_info file_info;
    if (unzGetCurrentFileInfo(zip_file, &file_info, NULL, 0, NULL, 0, NULL, 0) != UNZ_OK) {
        printf("Failed to get file info.\n");
        unzClose(zip_file);
        return 1;
    }

    return (UINT32)file_info.uncompressed_size;
}

int hal_send_zip(HANDLE hSerial, unzFile zip_file, const char* file_to_extract)
{
    char buf[BUF_SIZE] = { 0 };

    //Locate the file you want to extract
    int ret = unzLocateFile(zip_file, file_to_extract, 1);
    if (ret != UNZ_OK) {
        printf("Failed to locate file: %s\n", file_to_extract);
        unzClose(zip_file);
        return 1;
    }

    unz_file_info64  file_info;
    ret = unzGetCurrentFileInfo64(zip_file, &file_info, NULL, 0, NULL, 0, NULL, 0); // get file info
    if (ret != UNZ_OK) {
        printf("Failed to get file info: %s\n", file_to_extract);
        unzClose(zip_file);
        return 1;
    }

    ret = unzOpenCurrentFile(zip_file); // Open the file you want to extract
    if (ret != UNZ_OK) {
        printf("Failed to open file: %s\n", file_to_extract);
        unzClose(zip_file);
        return 1;
    }

    int bytesRead = 0;
    UINT32 id = 0;
    do {
        bytesRead = unzReadCurrentFile(zip_file, buf, (sizeof(buf) - 16));
        if (bytesRead > 0) {
            hal_send_zip_bin(hSerial, buf, bytesRead, id);
            id++;
        }
        memset(buf, 0, BUF_SIZE);
    } while (bytesRead > 0);

    unzCloseCurrentFile(zip_file); // Close the files in the ZIP file

    return 1;
}

static void hal_libusb_bin_pack(UINT8* buffer_start, UINT8* buffer_end, UINT32 id, hal_libusb_fw_dl_tag_t* fwdl_tag)
{
    UINT16 sum = 0;
    UINT8* p = buffer_start;

    for (p = buffer_start; p < buffer_end; p++) {
        sum += (UINT8)*p & 0xff;
    }

    libusb_bin_checksum += sum;
    libusb_bin_checksum = (libusb_bin_checksum & 0xffff);

    fwdl_tag->id = id;
    fwdl_tag->checksum = (UINT16)(sum & 0xffff);
}

static int hal_libusb_send_bin(hal_wifi_libusb_t* device, UINT8 endpoint, UINT8* buffer, UINT32 len, UINT32 id)
{
    hal_libusb_fw_dl_tag_t fwdl_tag = { 0 };

    hal_libusb_bin_pack(buffer, (buffer + len), id, &fwdl_tag);
    memcpy((buffer + len), (UINT8*)&fwdl_tag, sizeof(hal_libusb_fw_dl_tag_t));
    hal_wifi_libusb_bulk_transfer(device, endpoint, buffer, (len + sizeof(hal_libusb_fw_dl_tag_t)));
    memset(buffer, 0, HAL_USB_FW_DL_MTU);
    memset(&fwdl_tag, 0, sizeof(hal_libusb_fw_dl_tag_t));

    return 0;
}

BOOL hal_libusb_send_zip(hal_wifi_libusb_t* device, UINT8 endpoint, unzFile zip_file, const char* file_to_extract)
{
    UINT8 buf[HAL_USB_FW_DL_MTU] = { 0 };

    //Locate the file you want to extract
    int ret = unzLocateFile(zip_file, file_to_extract, 1);
    if (ret != UNZ_OK) {
        printf("Failed to locate file: %s\n", file_to_extract);
        unzClose(zip_file);
        return FALSE;
    }

    unz_file_info64  file_info;
    ret = unzGetCurrentFileInfo64(zip_file, &file_info, NULL, 0, NULL, 0, NULL, 0); // get file info
    if (ret != UNZ_OK) {
        printf("Failed to get file info: %s\n", file_to_extract);
        unzClose(zip_file);
        return FALSE;
    }

    ret = unzOpenCurrentFile(zip_file); // Open the file you want to extract
    if (ret != UNZ_OK) {
        printf("Failed to open file: %s\n", file_to_extract);
        unzClose(zip_file);
        return FALSE;
    }

    int bytesRead = 0;
    UINT32 id = 0;
    do {
        bytesRead = unzReadCurrentFile(zip_file, buf, (sizeof(buf) - 16));
        if (bytesRead > 0) {
            hal_libusb_send_bin(device, endpoint, buf, bytesRead, id);
            id++;
        }
        memset(buf, 0, BUF_SIZE);
    } while (bytesRead > 0);

    unzCloseCurrentFile(zip_file); // Close the files in the ZIP file

    return TRUE;
}

static int hal_while_read_zip(unzFile zip_file)
{
    // Walk through each file in the compressed folder
    unz_global_info64 gi;
    int err = unzGetGlobalInfo64(zip_file, &gi);
    if (err != UNZ_OK) {
        printf("Failed to get global info of zip file.\n");
        unzClose(zip_file);
        return 1;
    }

    for (uLong i = 0; i < gi.number_entry; ++i) {
        unz_file_info64 file_info;
        char filename[256];
        err = unzGetCurrentFileInfo64(zip_file, &file_info, filename, sizeof(filename), NULL, 0, NULL, 0);
        if (err != UNZ_OK) {
            printf("Failed to get file info of entry %lu.\n", i);
            unzClose(zip_file);
            return 1;
        }

        printf("File: %s\n", filename);
        printf("Size: %llu bytes\n", file_info.uncompressed_size);

        // Open current file
        err = unzOpenCurrentFile(zip_file);
        if (err != UNZ_OK) {
            printf("Failed to open current file.\n");
            unzClose(zip_file);
            return 1;
        }

        //if (strcmp(filename, sbr_source_fw) == 0) {
        //    printf("===========sbr===========\n");
        //}

        //if (strcmp(filename, dtop_source_fw) == 0) {
        //    printf("===========dtop===========\n");
        //}

        //if (strcmp(filename, wifi_source_fw) == 0) {
        //    printf("===========wifi===========\n");
        //}

        //if (strcmp(filename, bt_source_fw) == 0) {
        //    printf("===========bt===========\n");
        //}

        // reads the contents of the current file
        char buffer[1024];
        int bytesRead = 0;
        do {
            bytesRead = unzReadCurrentFile(zip_file, buffer, sizeof(buffer));
            if (bytesRead > 0) {
                printf("bytesRead:%d\n", bytesRead);
                for (int i = 0; i < bytesRead; i++) {
                    printf("0x%x ", (UINT8)buffer[i]);
                }
            }
        } while (bytesRead > 0);

        // Close current file
        unzCloseCurrentFile(zip_file);

        // move to the next file
        if (i < gi.number_entry - 1) {
            err = unzGoToNextFile(zip_file);
            if (err != UNZ_OK) {
                printf("Failed to go to next file.\n");
                unzClose(zip_file);
                return 1;
            }
        }

        printf("\n");
    }

    return 0;
}

int hal_read_wpk(const char* wpk_file)
{
    UINT8 is_zip_file = 1;
    is_zip_file = hal_is_zip_file(wpk_file);

    if (is_zip_file == 1) {
        return 1;
    }

    unzFile zipFile = unzOpen(wpk_file);
    if (zipFile == NULL)
    {
        printf("Failed to open ZIP file: %s\n", wpk_file);
        return 1;
    }

    //hal_unzip(zipFile, wpk_file);
    //hal_zip_list(zipFile);
    //hal_while_read_zip(zipFile);

    unzClose(zipFile);
    return 0;
}