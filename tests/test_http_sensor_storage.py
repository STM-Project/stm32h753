#!/usr/bin/env python3
"""Exercise SD settings transactions against a fault-injecting FatFs adapter.

Run: python3 tests/test_http_sensor_storage.py
The settings serializer has its own tests; this harness supplies a strict fake
blob validator to isolate the storage adapter and each filesystem failure path.
"""
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]

TIMER_STUB = """
enum { Semphr_cardSD = 2 };
unsigned TakeMutex(int, unsigned);
void GiveMutex(int);
"""

FATFS_STUB = """
#include <stddef.h>
typedef unsigned UINT;
typedef enum {
 FR_OK, FR_DISK_ERR, FR_INT_ERR, FR_NOT_READY, FR_NO_FILE, FR_NO_PATH,
 FR_INVALID_NAME, FR_DENIED, FR_EXIST, FR_INVALID_OBJECT, FR_WRITE_PROTECTED,
 FR_INVALID_DRIVE, FR_NOT_ENABLED, FR_NO_FILESYSTEM, FR_MKFS_ABORTED, FR_TIMEOUT
} FRESULT;
typedef struct { int id; size_t size; } FIL;
#define FA_READ 1
#define FA_WRITE 2
#define FA_CREATE_ALWAYS 8
#define f_size(f) ((f)->size)
FRESULT f_open(FIL *, const char *, unsigned);
FRESULT f_close(FIL *);
FRESULT f_read(FIL *, void *, UINT, UINT *);
FRESULT f_write(FIL *, const void *, UINT, UINT *);
FRESULT f_sync(FIL *);
FRESULT f_unlink(const char *);
FRESULT f_rename(const char *, const char *);
"""


def main():
    with tempfile.TemporaryDirectory(prefix="h7-sensor-storage-") as directory:
        directory = Path(directory)
        (directory / "timer.h").write_text(TIMER_STUB)
        (directory / "ff.h").write_text(FATFS_STUB)
        executable = directory / "storage-test"
        subprocess.run([
            "gcc", "-std=c11", "-Wall", "-Wextra", "-Werror",
            "-fsanitize=address,undefined", "-I" + str(directory),
            "-I" + str(ROOT / "Application"), "-I" + str(ROOT / "Application/HTTP"),
            str(ROOT / "Application/HTTP/http_sensor_storage.c"),
            str(ROOT / "tests/fixtures/http_sensor_storage_harness.c"),
            "-o", str(executable),
        ], check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
