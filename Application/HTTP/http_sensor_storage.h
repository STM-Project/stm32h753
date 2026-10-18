#ifndef HTTP_SENSOR_STORAGE_H_
#define HTTP_SENSOR_STORAGE_H_

/* Configuration lives on the SD card; this header needs no STM32 headers. */
typedef enum {
    HTTP_SENSOR_STORAGE_NOT_INITIALIZED = 0,
    HTTP_SENSOR_STORAGE_READY = 1,
    HTTP_SENSOR_STORAGE_DEFAULTS = 2,
    HTTP_SENSOR_STORAGE_RECOVERED_BACKUP = 3,
    HTTP_SENSOR_STORAGE_CORRUPT = -1,
    HTTP_SENSOR_STORAGE_IO_ERROR = -2,
    HTTP_SENSOR_STORAGE_LOCK_TIMEOUT = -3
} HttpSensorStorageStatus;

/* Call after FatFs and the SD-card mutex have been initialized.
 * Returns 1 for loaded settings, 0 for a new configuration, -1 on failure.
 * Corrupt files are retained and are not overwritten by a settings save.
 */
int HTTP_SensorStorageInit(void);
HttpSensorStorageStatus HTTP_SensorStorageGetStatus(void);
/* Last FatFs FRESULT, as an int so the public header stays hardware-neutral. */
int HTTP_SensorStorageGetFatFsError(void);

#endif /* HTTP_SENSOR_STORAGE_H_ */
