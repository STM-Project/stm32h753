#ifndef HTTP_SETTINGS_STORAGE_H_
#define HTTP_SETTINGS_STORAGE_H_

/* Configuration lives on the SD card; this header needs no STM32 headers. */
typedef enum {
    HTTP_SETTINGS_STORAGE_NOT_INITIALIZED = 0,
    HTTP_SETTINGS_STORAGE_READY = 1,
    HTTP_SETTINGS_STORAGE_DEFAULTS = 2,
    HTTP_SETTINGS_STORAGE_RECOVERED_BACKUP = 3,
    HTTP_SETTINGS_STORAGE_CORRUPT = -1,
    HTTP_SETTINGS_STORAGE_IO_ERROR = -2,
    HTTP_SETTINGS_STORAGE_LOCK_TIMEOUT = -3
} HttpSettingsStorageStatus;

/* Call after SDRAM, FatFs and the SD-card mutex have been initialized.
 * Persists the complete settings model independently of h7sense sensor files.
 * Returns 1 for loaded settings, 0 for a new configuration, -1 on failure.
 * Corrupt files are retained and are not overwritten by a settings save.
 */
int HTTP_SettingsStorageInit(void);
HttpSettingsStorageStatus HTTP_SettingsStorageGetStatus(void);
/* Last FatFs FRESULT, as an int so the public header stays hardware-neutral. */
int HTTP_SettingsStorageGetFatFsError(void);

#endif /* HTTP_SETTINGS_STORAGE_H_ */
