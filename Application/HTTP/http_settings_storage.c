#include "http_settings_storage.h"
#include "http_settings_model.h"
#include "timer.h"
#include "ff.h"
#include <string.h>
#include <stdio.h>

/* Large snapshots belong in initialized external SDRAM, not the HTTP task stack
 * or internal SRAM. Host fault tests use the regular data segment. */
#if defined(__arm__) || defined(__thumb__)
#define SETTINGS_EXTERNAL_BUFFER __attribute__((section(".sdram"), aligned(32)))
#else
#define SETTINGS_EXTERNAL_BUFFER __attribute__((aligned(32)))
#endif

/* Short names also work when long filename support is disabled. */
#define SETTINGS_ACTIVE_FILE "0:/h7conf.bin"
#define SETTINGS_BACKUP_FILE "0:/h7conf.bak"
#define SETTINGS_STAGING_FILE "0:/h7conf.tmp"
#define SETTINGS_STORAGE_LOCK_TICKS 1000U
#define SETTINGS_ALIGNED_BLOB_SIZE ((HTTP_SETTINGS_BLOB_SIZE + 31U) & ~31U)

/* FatFs can DMA directly into its FIL sector buffer and the data buffer.
 * Full cache-line alignment keeps cache maintenance away from other objects.
 * Both objects are used only while Semphr_cardSD is held.
 */
static FIL settingsFile __attribute__((aligned(32)));
static unsigned char settingsBytes[SETTINGS_ALIGNED_BLOB_SIZE] SETTINGS_EXTERNAL_BUFFER;
_Static_assert(HTTP_SETTINGS_BLOB_SIZE <= (UINT)~(UINT)0,
               "FatFs UINT cannot represent the complete settings blob");
static HttpSettingsStorageStatus storageStatus = HTTP_SETTINGS_STORAGE_NOT_INITIALIZED;
static FRESULT storageFatFsError = FR_OK;
static int settingsLoaded;

enum {
    SETTINGS_FILE_MISSING = 0,
    SETTINGS_FILE_VALID = 1,
    SETTINGS_FILE_CORRUPT = -1,
    SETTINGS_FILE_IO_ERROR = -2
};

static int SettingsReadFile(const char *path, size_t length)
{
    UINT received = 0;
    FRESULT result = f_open(&settingsFile, path, FA_READ);
    FRESULT closeResult;
    int fileState;

    if (result == FR_NO_FILE) return SETTINGS_FILE_MISSING;
    if (result != FR_OK) {
        storageFatFsError = result;
        return SETTINGS_FILE_IO_ERROR;
    }

    if (f_size(&settingsFile) != length) {
        fileState = SETTINGS_FILE_CORRUPT;
    } else {
        result = f_read(&settingsFile, settingsBytes, (UINT)length, &received);
        if (result != FR_OK) {
            storageFatFsError = result;
            fileState = SETTINGS_FILE_IO_ERROR;
        } else if (received != length ||
                   !HTTP_SettingsModelValidateBlob(settingsBytes, length)) {
            fileState = SETTINGS_FILE_CORRUPT;
        } else {
            fileState = SETTINGS_FILE_VALID;
        }
    }
    closeResult = f_close(&settingsFile);
    if (closeResult != FR_OK) {
        storageFatFsError = closeResult;
        fileState = SETTINGS_FILE_IO_ERROR;
    }
    return fileState;
}

static int SettingsLoad(void *buffer, size_t length, void *context)
{
    int activeState, backupState, result = -1;
    (void)context;
    if (length != HTTP_SETTINGS_BLOB_SIZE || !buffer) {
        storageStatus = HTTP_SETTINGS_STORAGE_CORRUPT;
        return -1;
    }
    if (!TakeMutex(Semphr_cardSD, SETTINGS_STORAGE_LOCK_TICKS)) {
        storageStatus = HTTP_SETTINGS_STORAGE_LOCK_TIMEOUT;
        return -1;
    }
    storageFatFsError = FR_OK;
    activeState = SettingsReadFile(SETTINGS_ACTIVE_FILE, length);
    if (activeState == SETTINGS_FILE_VALID) {
        memcpy(buffer, settingsBytes, length);
        storageStatus = HTTP_SETTINGS_STORAGE_READY;
        result = 1;
    } else {
        backupState = SettingsReadFile(SETTINGS_BACKUP_FILE, length);
        if (backupState == SETTINGS_FILE_VALID) {
            memcpy(buffer, settingsBytes, length);
            storageStatus = HTTP_SETTINGS_STORAGE_RECOVERED_BACKUP;
            result = 1;
        } else if (activeState == SETTINGS_FILE_MISSING &&
                   backupState == SETTINGS_FILE_MISSING) {
            storageStatus = HTTP_SETTINGS_STORAGE_DEFAULTS;
            result = 0;
        } else if (activeState == SETTINGS_FILE_CORRUPT ||
                   backupState == SETTINGS_FILE_CORRUPT) {
            storageStatus = HTTP_SETTINGS_STORAGE_CORRUPT;
        } else {
            storageStatus = HTTP_SETTINGS_STORAGE_IO_ERROR;
        }
    }
    GiveMutex(Semphr_cardSD);
    return result;
}

static int SettingsWriteStaging(const void *buffer, size_t length)
{
    UINT written = 0;
    FRESULT result, closeResult;
    memcpy(settingsBytes, buffer, length);
    result = f_open(&settingsFile, SETTINGS_STAGING_FILE, FA_CREATE_ALWAYS | FA_WRITE);
    if (result != FR_OK) {
        storageFatFsError = result;
        return 0;
    }
    result = f_write(&settingsFile, settingsBytes, (UINT)length, &written);
    if (result == FR_OK && written != length) result = FR_DISK_ERR;
    if (result == FR_OK) result = f_sync(&settingsFile);
    closeResult = f_close(&settingsFile);
    if (result == FR_OK) result = closeResult;
    if (result != FR_OK) {
        storageFatFsError = result;
        return 0;
    }
    return 1;
}

static int SettingsSave(const void *buffer, size_t length, void *context)
{
    int activeState, backupState = SETTINGS_FILE_MISSING, result = -1;
    FRESULT fileResult;
    (void)context;
    if (length != HTTP_SETTINGS_BLOB_SIZE || !buffer ||
        !HTTP_SettingsModelValidateBlob(buffer, length)) {
        storageStatus = HTTP_SETTINGS_STORAGE_CORRUPT;
        return -1;
    }
    if (!TakeMutex(Semphr_cardSD, SETTINGS_STORAGE_LOCK_TICKS)) {
        storageStatus = HTTP_SETTINGS_STORAGE_LOCK_TIMEOUT;
        return -1;
    }
    storageFatFsError = FR_OK;
    /* Check the files again: card removal or corruption after startup must
     * not replace the only usable backup with an unreadable active file.
     */
    activeState = SettingsReadFile(SETTINGS_ACTIVE_FILE, length);
    if (activeState == SETTINGS_FILE_IO_ERROR) goto io_error;
    if (activeState != SETTINGS_FILE_VALID) {
        backupState = SettingsReadFile(SETTINGS_BACKUP_FILE, length);
        if (backupState == SETTINGS_FILE_IO_ERROR) goto io_error;
        if (backupState != SETTINGS_FILE_VALID &&
            (activeState == SETTINGS_FILE_CORRUPT || backupState == SETTINGS_FILE_CORRUPT)) {
            storageStatus = HTTP_SETTINGS_STORAGE_CORRUPT;
            goto done;
        }
    }
    if (!SettingsWriteStaging(buffer, length)) goto io_error;
    /* Do not rotate the known-good configuration until the flushed, closed
     * replacement can be read back with its CRC/schema intact. Comparing to
     * the submitted snapshot also detects a valid but stale card write.
     */
    if (SettingsReadFile(SETTINGS_STAGING_FILE, length) != SETTINGS_FILE_VALID ||
        memcmp(settingsBytes, buffer, length) != 0) {
        if (storageFatFsError == FR_OK) storageFatFsError = FR_DISK_ERR;
        goto io_error;
    }

    if (activeState == SETTINGS_FILE_VALID) {
        fileResult = f_unlink(SETTINGS_BACKUP_FILE);
        if (fileResult != FR_OK && fileResult != FR_NO_FILE) {
            storageFatFsError = fileResult;
            goto io_error;
        }
        fileResult = f_rename(SETTINGS_ACTIVE_FILE, SETTINGS_BACKUP_FILE);
        if (fileResult != FR_OK) {
            storageFatFsError = fileResult;
            goto io_error;
        }
        fileResult = f_rename(SETTINGS_STAGING_FILE, SETTINGS_ACTIVE_FILE);
        if (fileResult != FR_OK) {
            /* The backup stays valid even if rollback itself fails. The
             * loader checks it on the next boot; staging is never loaded.
             */
            FRESULT rollbackResult = f_rename(SETTINGS_BACKUP_FILE, SETTINGS_ACTIVE_FILE);
            storageFatFsError = rollbackResult == FR_OK ? fileResult : rollbackResult;
            goto io_error;
        }
    } else {
        if (activeState == SETTINGS_FILE_CORRUPT) {
            /* Reached only when a separately validated backup is retained. */
            fileResult = f_unlink(SETTINGS_ACTIVE_FILE);
            if (fileResult != FR_OK) {
                storageFatFsError = fileResult;
                goto io_error;
            }
        }
        fileResult = f_rename(SETTINGS_STAGING_FILE, SETTINGS_ACTIVE_FILE);
        if (fileResult != FR_OK) {
            storageFatFsError = fileResult;
            goto io_error;
        }
    }
    storageStatus = HTTP_SETTINGS_STORAGE_READY;
    result = 1;
    goto done;

io_error:
    storageStatus = HTTP_SETTINGS_STORAGE_IO_ERROR;
done:
    GiveMutex(Semphr_cardSD);
    return result;
}

int HTTP_SettingsStorageInit(void)
{
    int result = HTTP_SettingsModelInit(SettingsLoad, SettingsSave, NULL);
    settingsLoaded = result >= 0;
    return result;
}

int HTTP_SettingsStorageRetryLoad(void)
{
    return !settingsLoaded && HTTP_SettingsStorageInit() >= 0;
}

HttpSettingsStorageStatus HTTP_SettingsStorageGetStatus(void)
{
    return storageStatus;
}

int HTTP_SettingsStorageGetFatFsError(void)
{
    return (int)storageFatFsError;
}

const char *HTTP_SettingsStorageGetErrorMessage(void)
{
    static char message[224];
    const char *reason;
    if (storageStatus == HTTP_SETTINGS_STORAGE_CORRUPT)
        return "Nie zapisano ustawien: plik h7conf.bin lub h7conf.bak jest uszkodzony albo niezgodny z firmware. Zachowaj kopie plikow konfiguracji.";
    if (storageStatus == HTTP_SETTINGS_STORAGE_LOCK_TIMEOUT)
        return "Nie zapisano ustawien: karta SD jest zajeta. Sprobuj zapisac ponownie.";
    if (storageStatus == HTTP_SETTINGS_STORAGE_NOT_INITIALIZED)
        return "Nie zapisano ustawien: nie uruchomiono obslugi konfiguracji na karcie SD.";
    switch (storageFatFsError) {
    case FR_NOT_READY: reason = "karta SD jest niedostepna (FR_NOT_READY)"; break;
    case FR_NO_FILESYSTEM: reason = "brak obslugiwanego systemu plikow na karcie (FR_NO_FILESYSTEM)"; break;
    case FR_WRITE_PROTECTED: reason = "karta SD jest zabezpieczona przed zapisem (FR_WRITE_PROTECTED)"; break;
    case FR_DENIED: reason = "brak miejsca lub odmowa zapisu na karcie (FR_DENIED)"; break;
    case FR_NOT_ENABLED: reason = "system plikow karty SD nie jest zamontowany (FR_NOT_ENABLED)"; break;
    case FR_DISK_ERR: reason = "blad odczytu lub zapisu karty SD (FR_DISK_ERR)"; break;
    case FR_TIMEOUT: reason = "przekroczony czas dostepu do karty SD (FR_TIMEOUT)"; break;
    default: reason = "blad dostepu do konfiguracji na karcie SD"; break;
    }
    snprintf(message, sizeof message, "Nie zapisano ustawien: %s. Kod FatFs: %d.", reason, (int)storageFatFsError);
    return message;
}
