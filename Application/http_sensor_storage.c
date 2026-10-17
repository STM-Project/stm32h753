#include "http_sensor_storage.h"
#include "http_sensor_settings.h"
#include "timer.h"
#include "ff.h"
#include <string.h>

/* Short names also work when long filename support is disabled. */
#define SENSOR_ACTIVE_FILE "0:/h7sense.bin"
#define SENSOR_BACKUP_FILE "0:/h7sense.bak"
#define SENSOR_STAGING_FILE "0:/h7sense.tmp"
#define SENSOR_STORAGE_LOCK_TICKS 1000U
#define SENSOR_ALIGNED_BLOB_SIZE ((HTTP_SENSOR_SETTINGS_BLOB_SIZE + 31U) & ~31U)

/* FatFs can DMA directly into its FIL sector buffer and the data buffer.
 * Full cache-line alignment keeps cache maintenance away from other objects.
 * Both objects are used only while Semphr_cardSD is held.
 */
static FIL sensorFile __attribute__((aligned(32)));
static unsigned char sensorBytes[SENSOR_ALIGNED_BLOB_SIZE] __attribute__((aligned(32)));
static HttpSensorStorageStatus storageStatus = HTTP_SENSOR_STORAGE_NOT_INITIALIZED;
static FRESULT storageFatFsError = FR_OK;

enum {
    SENSOR_FILE_MISSING = 0,
    SENSOR_FILE_VALID = 1,
    SENSOR_FILE_CORRUPT = -1,
    SENSOR_FILE_IO_ERROR = -2
};

static int SensorReadFile(const char *path, size_t length)
{
    UINT received = 0;
    FRESULT result = f_open(&sensorFile, path, FA_READ);
    FRESULT closeResult;
    int fileState;

    if (result == FR_NO_FILE) return SENSOR_FILE_MISSING;
    if (result != FR_OK) {
        storageFatFsError = result;
        return SENSOR_FILE_IO_ERROR;
    }

    if (f_size(&sensorFile) != length) {
        fileState = SENSOR_FILE_CORRUPT;
    } else {
        result = f_read(&sensorFile, sensorBytes, (UINT)length, &received);
        if (result != FR_OK) {
            storageFatFsError = result;
            fileState = SENSOR_FILE_IO_ERROR;
        } else if (received != length ||
                   !HTTP_SensorSettingsValidateBlob(sensorBytes, length)) {
            fileState = SENSOR_FILE_CORRUPT;
        } else {
            fileState = SENSOR_FILE_VALID;
        }
    }
    closeResult = f_close(&sensorFile);
    if (closeResult != FR_OK) {
        storageFatFsError = closeResult;
        fileState = SENSOR_FILE_IO_ERROR;
    }
    return fileState;
}

static int SensorLoad(void *buffer, size_t length, void *context)
{
    int activeState, backupState, result = -1;
    (void)context;
    if (length != HTTP_SENSOR_SETTINGS_BLOB_SIZE || !buffer) {
        storageStatus = HTTP_SENSOR_STORAGE_CORRUPT;
        return -1;
    }
    if (!TakeMutex(Semphr_cardSD, SENSOR_STORAGE_LOCK_TICKS)) {
        storageStatus = HTTP_SENSOR_STORAGE_LOCK_TIMEOUT;
        return -1;
    }
    storageFatFsError = FR_OK;
    activeState = SensorReadFile(SENSOR_ACTIVE_FILE, length);
    if (activeState == SENSOR_FILE_VALID) {
        memcpy(buffer, sensorBytes, length);
        storageStatus = HTTP_SENSOR_STORAGE_READY;
        result = 1;
    } else {
        backupState = SensorReadFile(SENSOR_BACKUP_FILE, length);
        if (backupState == SENSOR_FILE_VALID) {
            memcpy(buffer, sensorBytes, length);
            storageStatus = HTTP_SENSOR_STORAGE_RECOVERED_BACKUP;
            result = 1;
        } else if (activeState == SENSOR_FILE_MISSING &&
                   backupState == SENSOR_FILE_MISSING) {
            storageStatus = HTTP_SENSOR_STORAGE_DEFAULTS;
            result = 0;
        } else if (activeState == SENSOR_FILE_CORRUPT ||
                   backupState == SENSOR_FILE_CORRUPT) {
            storageStatus = HTTP_SENSOR_STORAGE_CORRUPT;
        } else {
            storageStatus = HTTP_SENSOR_STORAGE_IO_ERROR;
        }
    }
    GiveMutex(Semphr_cardSD);
    return result;
}

static int SensorWriteStaging(const void *buffer, size_t length)
{
    UINT written = 0;
    FRESULT result, closeResult;
    memcpy(sensorBytes, buffer, length);
    result = f_open(&sensorFile, SENSOR_STAGING_FILE, FA_CREATE_ALWAYS | FA_WRITE);
    if (result != FR_OK) {
        storageFatFsError = result;
        return 0;
    }
    result = f_write(&sensorFile, sensorBytes, (UINT)length, &written);
    if (result == FR_OK && written != length) result = FR_DISK_ERR;
    if (result == FR_OK) result = f_sync(&sensorFile);
    closeResult = f_close(&sensorFile);
    if (result == FR_OK) result = closeResult;
    if (result != FR_OK) {
        storageFatFsError = result;
        return 0;
    }
    return 1;
}

static int SensorSave(const void *buffer, size_t length, void *context)
{
    int activeState, backupState = SENSOR_FILE_MISSING, result = -1;
    FRESULT fileResult;
    (void)context;
    if (length != HTTP_SENSOR_SETTINGS_BLOB_SIZE || !buffer ||
        !HTTP_SensorSettingsValidateBlob(buffer, length)) {
        storageStatus = HTTP_SENSOR_STORAGE_CORRUPT;
        return -1;
    }
    if (!TakeMutex(Semphr_cardSD, SENSOR_STORAGE_LOCK_TICKS)) {
        storageStatus = HTTP_SENSOR_STORAGE_LOCK_TIMEOUT;
        return -1;
    }
    storageFatFsError = FR_OK;
    /* Check the files again: card removal or corruption after startup must
     * not replace the only usable backup with an unreadable active file.
     */
    activeState = SensorReadFile(SENSOR_ACTIVE_FILE, length);
    if (activeState == SENSOR_FILE_IO_ERROR) goto io_error;
    if (activeState != SENSOR_FILE_VALID) {
        backupState = SensorReadFile(SENSOR_BACKUP_FILE, length);
        if (backupState == SENSOR_FILE_IO_ERROR) goto io_error;
        if (backupState != SENSOR_FILE_VALID &&
            (activeState == SENSOR_FILE_CORRUPT || backupState == SENSOR_FILE_CORRUPT)) {
            storageStatus = HTTP_SENSOR_STORAGE_CORRUPT;
            goto done;
        }
    }
    if (!SensorWriteStaging(buffer, length)) goto io_error;

    if (activeState == SENSOR_FILE_VALID) {
        fileResult = f_unlink(SENSOR_BACKUP_FILE);
        if (fileResult != FR_OK && fileResult != FR_NO_FILE) {
            storageFatFsError = fileResult;
            goto io_error;
        }
        fileResult = f_rename(SENSOR_ACTIVE_FILE, SENSOR_BACKUP_FILE);
        if (fileResult != FR_OK) {
            storageFatFsError = fileResult;
            goto io_error;
        }
        fileResult = f_rename(SENSOR_STAGING_FILE, SENSOR_ACTIVE_FILE);
        if (fileResult != FR_OK) {
            /* The backup stays valid even if rollback itself fails. The
             * loader checks it on the next boot; staging is never loaded.
             */
            FRESULT rollbackResult = f_rename(SENSOR_BACKUP_FILE, SENSOR_ACTIVE_FILE);
            storageFatFsError = rollbackResult == FR_OK ? fileResult : rollbackResult;
            goto io_error;
        }
    } else {
        if (activeState == SENSOR_FILE_CORRUPT) {
            /* Reached only when a separately validated backup is retained. */
            fileResult = f_unlink(SENSOR_ACTIVE_FILE);
            if (fileResult != FR_OK) {
                storageFatFsError = fileResult;
                goto io_error;
            }
        }
        fileResult = f_rename(SENSOR_STAGING_FILE, SENSOR_ACTIVE_FILE);
        if (fileResult != FR_OK) {
            storageFatFsError = fileResult;
            goto io_error;
        }
    }
    storageStatus = HTTP_SENSOR_STORAGE_READY;
    result = 1;
    goto done;

io_error:
    storageStatus = HTTP_SENSOR_STORAGE_IO_ERROR;
done:
    GiveMutex(Semphr_cardSD);
    return result;
}

int HTTP_SensorStorageInit(void)
{
    return HTTP_SensorSettingsInit(SensorLoad, SensorSave, NULL);
}

HttpSensorStorageStatus HTTP_SensorStorageGetStatus(void)
{
    return storageStatus;
}

int HTTP_SensorStorageGetFatFsError(void)
{
    return (int)storageFatFsError;
}
