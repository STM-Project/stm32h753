#!/usr/bin/env python3
"""Exercise the portable settings parser and versioned storage under ASan/UBSan."""
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

HARNESS = r'''
#include "http_sensor_settings.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

static unsigned char saved[HTTP_SENSOR_SETTINGS_BLOB_SIZE];
static int have_saved, save_fails, load_fails, save_count;

static int load(void *data, size_t length, void *context) {
    assert(context == saved); assert(length == sizeof saved);
    if (load_fails) return -1;
    if (!have_saved) return 0;
    memcpy(data, saved, length); return 1;
}
static int save(const void *data, size_t length, void *context) {
    assert(context == saved); assert(length == sizeof saved);
    assert(HTTP_SensorSettingsValidateBlob(data, length));
    ++save_count;
    if (save_fails) return -1;
    memcpy(saved, data, length); have_saved = 1; return 1;
}
static void sensor(char *query, unsigned id, int humidity) {
    size_t used = strlen(query);
    int length = snprintf(query + used, HTTP_SENSOR_TARGET_MAX - used,
        "n_s_Czujki_nazwa_%03u=%s&"
        "n_s_Czujki_prog_max1_%03u=%s&n_s_Czujki_prog_max2_%03u=%s&"
        "n_s_Czujki_prog_min1_%03u=%s&n_s_Czujki_prog_min2_%03u=%s&"
        "n_s_Czujki_hist_%03u=0.5&n_s_Czujki_korekcja_%03u=-1.2&"
        "n_s_Czujki_czas_reak_%03u=01%%3A30&",
        id, humidity ? "Wilgotnosc" : "Pokoj+%26+%22salon%22", id, humidity ? "85.0" : "%2B26.1",
        id, humidity ? "90.0" : "31", id, humidity ? "10.0" : "-15.2",
        id, humidity ? "5.0" : "-20.1", id, id, id);
    assert(length > 0 && (size_t)length < HTTP_SENSOR_TARGET_MAX - used);
}
static void form(char *query, unsigned port, int humidity) {
    strcpy(query, "/lpc.cgi/A117?");
    sensor(query, (port - 1) * 2 + 1, 0);
    if (humidity) sensor(query, (port - 1) * 2 + 2, 1);
    strcat(query, "n_109_zapisz=");
}
static int apply(const char *query, unsigned *port) {
    return HTTP_SensorSettingsApplyTarget(query, strlen(query), port);
}
static void replace(char *query, const char *from, const char *to) {
    char tail[HTTP_SENSOR_TARGET_MAX];
    char *p = strstr(query, from);
    assert(p); strcpy(tail, p + strlen(from)); strcpy(p, to); strcat(p, tail);
}
static void reject(const char *query) {
    unsigned char snapshot[sizeof saved];
    unsigned port;
    int old_count = save_count;
    memcpy(snapshot, saved, sizeof snapshot);
    assert(apply(query, &port) == HTTP_SENSOR_SETTINGS_INVALID);
    assert(save_count == old_count);
    assert(memcmp(saved, snapshot, sizeof saved) == 0);
}
static void repair_checksum(unsigned char *data) {
    uint32_t crc = UINT32_C(0xffffffff);
    size_t i; unsigned bit;
    for (i = 16; i < sizeof saved; ++i) {
        crc ^= data[i];
        for (bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ ((crc & 1) ? UINT32_C(0xedb88320) : 0);
    }
    crc ^= UINT32_C(0xffffffff);
    for (i = 0; i < 4; ++i) data[12 + i] = (unsigned char)(crc >> (8 * i));
}

int main(void) {
    char query[HTTP_SENSOR_TARGET_MAX + 100];
    unsigned port, n;
    const HttpSensorSettings *v;
    assert(HTTP_SensorSettingsInit(load, save, saved) == 0);
    assert(HTTP_SensorSettingsGet(0, 0) == NULL);
    assert(HTTP_SensorSettingsGet(21, 0) == NULL);
    assert(HTTP_SensorSettingsGet(1, 2) == NULL);
    assert(HTTP_SensorSettingsGet(1, 0)->max1 == 0);
    assert(apply("/set", &port) == HTTP_SENSOR_SETTINGS_NOT_HANDLED);
    assert(apply("/lpc.cgi/A117?n_GPIO_val_001=2&n_109_zapisz=", &port) == 0);
    for (n = 1; n <= HTTP_SENSOR_PORT_COUNT; ++n) {
        form(query, n, 1); assert(apply(query, &port) == 1 && port == n);
        v = HTTP_SensorSettingsGet(n, 0);
        assert(strcmp(v->name, "Pokoj & \"salon\"") == 0);
        assert(v->max1 == 261 && v->max2 == 310 && v->min1 == -152);
        assert(v->min2 == -201 && v->hysteresis == 5 && v->correction == -12);
        assert(v->delay_seconds == 90);
        assert(HTTP_SensorSettingsGet(n, 1)->max2 == 900);
    }
    assert(HTTP_SensorSettingsValidateBlob(saved, sizeof saved));
    assert(!HTTP_SensorSettingsValidateBlob(saved, sizeof saved - 1));
    assert(!HTTP_SensorSettingsValidateBlob(NULL, sizeof saved));
    /* An internally consistent CRC cannot conceal semantically invalid data. */
    unsigned char invalid_blob[sizeof saved];
    memcpy(invalid_blob, saved, sizeof saved);
    invalid_blob[4] = 2; assert(!HTTP_SensorSettingsValidateBlob(invalid_blob, sizeof invalid_blob));
    memcpy(invalid_blob, saved, sizeof saved);
    memset(invalid_blob + 16, 'A', 31); repair_checksum(invalid_blob);
    assert(!HTTP_SensorSettingsValidateBlob(invalid_blob, sizeof invalid_blob));
    memcpy(invalid_blob, saved, sizeof saved);
    invalid_blob[16 + 39] = 255; invalid_blob[16 + 40] = 255; repair_checksum(invalid_blob);
    assert(!HTTP_SensorSettingsValidateBlob(invalid_blob, sizeof invalid_blob));
    memcpy(invalid_blob, saved, sizeof saved);
    invalid_blob[16 + 45 + 31] = 255; invalid_blob[16 + 45 + 32] = 255; repair_checksum(invalid_blob);
    assert(!HTTP_SensorSettingsValidateBlob(invalid_blob, sizeof invalid_blob));
    memcpy(invalid_blob, saved, sizeof saved);
    invalid_blob[16 + 43] = 255; invalid_blob[16 + 44] = 255; repair_checksum(invalid_blob);
    assert(!HTTP_SensorSettingsValidateBlob(invalid_blob, sizeof invalid_blob));
    assert(HTTP_SensorSettingsInit(load, save, saved) == 1);
    assert(HTTP_SensorSettingsGet(20, 1)->max2 == 900);
    form(query, 1, 0); replace(query, "n_s_Czujki_nazwa", "n_s_%43zujki_nazwa");
    assert(apply(query, &port) == 1);
    form(query, 1, 0); replace(query, "%2B26.1", "3276.7");
    replace(query, "-15.2", "-3276.8"); assert(apply(query, &port) == 1);
    assert(HTTP_SensorSettingsGet(1, 0)->max1 == 32767);
    assert(HTTP_SensorSettingsGet(1, 0)->min1 == INT16_MIN);
    form(query, 1, 1); replace(query, "=85.0", "=100.0"); assert(apply(query, &port) == 1);

    /* Reject malformed numbers and preserve every setting transactionally. */
    const char *numbers[] = {"nan", "NaN", "inf", "1e2", "", "1.23", "1.",
        ".1", "3276.8", "-3276.9", "10000", "--1", "+26.1", "%0026", "%0a1", "%GG", "%"};
    for (n = 0; n < sizeof numbers / sizeof numbers[0]; ++n) {
        form(query, 1, 0); replace(query, "%2B26.1", numbers[n]); reject(query);
    }
    form(query, 1, 1); replace(query, "=85.0", "=100.1"); reject(query);
    form(query, 1, 1); replace(query, "=85.0", "=-0.1"); reject(query);
    form(query, 1, 0); replace(query, "=0.5", "=-0.1"); reject(query);
    const char *times[] = {"01:60", "100:00", "1:00", "00:0", "-1:00", "00:00x"};
    for (n = 0; n < sizeof times / sizeof times[0]; ++n) {
        form(query, 1, 0); replace(query, "01%3A30", times[n]); reject(query);
    }
    form(query, 1, 0); replace(query, "01%3A30", "99:59"); assert(apply(query, &port) == 1);
    assert(HTTP_SensorSettingsGet(1, 0)->delay_seconds == 5999);
    form(query, 1, 0); replace(query, "Pokoj+%26+%22salon%22", "123456789012345678901234567890");
    assert(apply(query, &port) == 1);
    form(query, 1, 0); replace(query, "Pokoj+%26+%22salon%22", "1234567890123456789012345678901"); reject(query);
    form(query, 1, 0); replace(query, "Pokoj+%26+%22salon%22", "A%0DB"); reject(query);
    form(query, 1, 0); strcat(query, "&n_s_Czujki_hist_001=0.5"); reject(query);
    form(query, 1, 0); strcat(query, "&n_109_zapisz="); reject(query);
    form(query, 1, 0); strcat(query, "&unknown=1"); reject(query);
    form(query, 1, 0); strcat(query, "&"); reject(query);
    form(query, 1, 0); replace(query, "n_s_Czujki_hist_001=0.5&", ""); reject(query);
    form(query, 1, 0); replace(query, "n_109_zapisz=", ""); reject(query);
    form(query, 1, 1); replace(query, "prog_max1_002", "prog_max1_004"); reject(query);
    form(query, 1, 0); replace(query, "nazwa_001", "nazwa_000"); reject(query);
    form(query, 1, 0); replace(query, "nazwa_001", "nazwa_041"); reject(query);
    form(query, 1, 0); replace(query, "nazwa_001", "nazwa_01"); reject(query);
    form(query, 1, 0); replace(query, "n_s_Czujki_hist", "n_s_Czujki_hist_bad"); reject(query);
    form(query, 1, 0); memset(query + strlen(query), 'X', 2100 - strlen(query)); query[2100] = 0; reject(query);

    /* No RAM update is observable if persistence cannot commit. */
    form(query, 1, 0);
    int16_t previous = HTTP_SensorSettingsGet(1, 0)->max1;
    replace(query, "%2B26.1", "27.0"); save_fails = 1;
    assert(apply(query, &port) == HTTP_SENSOR_SETTINGS_STORAGE_ERROR);
    assert(HTTP_SensorSettingsGet(1, 0)->max1 == previous);
    save_fails = 0;
    assert(apply(query, &port) == 1);
    assert(HTTP_SensorSettingsGet(1, 0)->max1 == 270);

    /* Truncated request targets, including non-NUL-terminated inputs, are safe. */
    form(query, 1, 1);
    for (n = 0; n < strlen(query); ++n) {
        char exact[HTTP_SENSOR_TARGET_MAX];
        memcpy(exact, query, n);
        int result = HTTP_SensorSettingsApplyTarget(exact, n, &port);
        /* n_109_zapisz= can only be recognized in full. */
        assert(result == 0 || result == HTTP_SENSOR_SETTINGS_INVALID);
    }
    /* Deterministic malformed request fuzzing with accurate buffer lengths. */
    uint32_t random = 42;
    for (n = 0; n < 5000; ++n) {
        char input[512] = "/lpc.cgi/A117?n_s_Czujki_";
        size_t i, length = 25 + n % 487;
        for (i = strlen(input); i < length; ++i) {
            random = random * 1664525u + 1013904223u;
            input[i] = (char)(random >> 24);
        }
        assert(HTTP_SensorSettingsApplyTarget(input, length, &port) == HTTP_SENSOR_SETTINGS_INVALID);
    }
    saved[100] ^= 1;
    assert(!HTTP_SensorSettingsValidateBlob(saved, sizeof saved));
    assert(HTTP_SensorSettingsInit(load, save, saved) == -1);
    assert(HTTP_SensorSettingsGet(1, 0)->max1 == 0);
    load_fails = 1; assert(HTTP_SensorSettingsInit(load, save, saved) == -1);
    assert(HTTP_SensorSettingsInit(NULL, NULL, NULL) == 0);
    form(query, 1, 0); assert(apply(query, &port) == HTTP_SENSOR_SETTINGS_STORAGE_ERROR);
    puts("PASS: 20-port round trips, strict CGI validation, transactional persistence, CRC and bounded-input ASan/UBSan checks.");
    return 0;
}
'''


def main():
    compiler = shutil.which("gcc") or shutil.which("clang")
    if not compiler:
        raise SystemExit("A host C compiler is required")
    with tempfile.TemporaryDirectory(prefix="http-sensors-") as directory:
        tmp = Path(directory)
        harness = tmp / "settings_test.c"
        harness.write_text(HARNESS)
        executable = tmp / "settings_test"
        subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
                        "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                        "-I" + str(ROOT / "Application"), "-I" + str(ROOT / "Application/HTTP"), str(harness),
                        str(ROOT / "Application/HTTP/http_sensor_settings.c"),
                        "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
