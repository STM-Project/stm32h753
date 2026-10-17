#!/usr/bin/env python3
"""Exercise SD settings transactions against a fault-injecting FatFs adapter.

Run: python3 tests/test_http_settings_storage.py
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

RECOVERY_HARNESS = r'''
static int save_timer(unsigned instance, const char *hour) {
    char path[32], target[4096], name[96]; uint16_t found;
    snprintf(path,sizeof path,"/tim%03u",instance);
    const HTTP_SettingPage *page=HTTP_SettingsCatalogFind(path,&found); assert(page);
    size_t used=(size_t)snprintf(target,sizeof target,"/lpc.cgi/A117?h7_page=%s&h7_form=0",path);
    for(unsigned f=0;f<page->field_count;f++) {
        const HTTP_SettingField *field=&page->fields[f];
        if(field->kind==HTTP_SETTING_CHECKBOX) continue;
        assert(HTTP_SettingsCatalogFieldName(field,found,name,sizeof name));
        const char *value=f==0?hour:field->default_value;
        int n=snprintf(target+used,sizeof target-used,"&%s=%s",name,value);
        assert(n>0 && (size_t)n<sizeof target-used);used+=(size_t)n;
    }
    strcat(target,"&n_109_zapisz=");
    return HTTP_SettingsApplyTarget(target,strlen(target),NULL,NULL);
}
int main(void) {
    assert(HTTP_SettingsStorageInit()==0);
    assert(save_timer(2,"08:30:00")==HTTP_SETTINGS_SAVED);
    /* Simulate a cold boot with a previously saved card but no model/backend. */
    assert(HTTP_SettingsModelInit(NULL,NULL,NULL)==0);
    failOpen=1;assert(HTTP_SettingsStorageInit()==-1);
    assert(strstr(HTTP_SettingsStorageGetErrorMessage(),"FR_NOT_READY"));
    assert(save_timer(1,"12:45:00")==HTTP_SETTINGS_STORAGE_ERROR);
    assert(HTTP_SettingsStorageRetryLoad()==0);
    failOpen=0;assert(HTTP_SettingsStorageRetryLoad()==1);
    assert(save_timer(1,"12:45:00")==HTTP_SETTINGS_SAVED);
    assert(HTTP_SettingsStorageInit()==1);
    uint16_t instance;
    const HTTP_SettingPage *page=HTTP_SettingsCatalogFind("/tim002",&instance);
    assert(!strcmp(HTTP_SettingsGetByName(page,instance,"n_s_Tim_godzON1_002"),"08:30:00"));
    page=HTTP_SettingsCatalogFind("/tim001",&instance);
    assert(!strcmp(HTTP_SettingsGetByName(page,instance,"n_s_Tim_godzON1_001"),"12:45:00"));
    assert(HTTP_SettingsStorageRetryLoad()==0);
    puts("PASS: real model + FatFs recover after failed boot load, preserving another timer and persisting the retried save.");
    return 0;
}
'''


def main():
    with tempfile.TemporaryDirectory(prefix="h7-settings-storage-") as directory:
        directory = Path(directory)
        (directory / "timer.h").write_text(TIMER_STUB)
        (directory / "ff.h").write_text(FATFS_STUB)
        executable = directory / "storage-test"
        subprocess.run([
            "gcc", "-std=c11", "-Wall", "-Wextra", "-Werror",
            "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-no-pie", "-I" + str(directory),
            "-I" + str(ROOT / "Application"),
            str(ROOT / "Application/http_settings_storage.c"),
            str(ROOT / "tests/fixtures/http_settings_storage_harness.c"),
            "-o", str(executable),
        ], check=True)
        subprocess.run([str(executable)], check=True)
        # Reuse the fault-injecting filesystem with the real serializer rather
        # than the isolated adapter test's model stub.
        mock = (ROOT / "tests/fixtures/http_settings_storage_harness.c").read_text()
        mock = mock[:mock.index("int HTTP_SettingsModelValidateBlob(")]
        for declaration in ("static unsigned char loaded[N], candidate[N];\n",
                            "static HTTP_SettingsSave saveCallback;\n",
                            "static void *saveContext;\n"):
            mock = mock.replace(declaration, "")
        integration = directory / "recovery.c"
        integration.write_text(mock + RECOVERY_HARNESS)
        recovery = directory / "recovery"
        subprocess.run(["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                        "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-no-pie",
                        "-I" + str(directory), "-I" + str(ROOT / "Application"),
                        str(integration), str(ROOT / "Application/http_settings_storage.c"),
                        str(ROOT / "Application/http_settings_model.c"),
                        str(ROOT / "Application/http_settings_catalog.c"), "-o", str(recovery)], check=True)
        subprocess.run([str(recovery)], check=True)


if __name__ == "__main__":
    main()
