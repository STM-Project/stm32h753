#!/usr/bin/env python3
"""Verify committed Sandwitch forms reach supported H7 configuration safely.

The real catalog, transaction parser and binding implementation run against a
small hardware configuration stub. No test configuration values are logged.
"""
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

VARIABLES_STUB = r'''
#include <stdint.h>
#include <time.h>
#define WIFI_AP_MAX 3
#define WIFI_STA_MAX 3
#define MAX_EMAIL_RECIPIENTS 8
typedef struct __attribute__((packed)) {
 uint64_t mac; uint32_t ip, mask, gate; uint16_t port; uint8_t dhcp;
 char name[64], pass[64];
} s_wifi_AP, s_wifi_STA;
typedef struct __attribute__((packed)) {
 int nrAP, nrSTA; uint8_t mode;
} s_wifi_select;
typedef struct __attribute__((packed)) {
 char server[96]; uint32_t IP; uint16_t port;
 char name[32], login[96], password[96]; uint8_t useSSL;
} s_smtp_sender;
typedef struct { char email[96]; } s_smtp_recipient;
typedef struct {
 s_wifi_AP wifiAP[WIFI_AP_MAX]; s_wifi_STA wifiSTA[WIFI_STA_MAX];
 s_wifi_select wifiGeneral;
 s_smtp_sender emailSend[3]; s_smtp_recipient emailRecv[8];
 struct { int time; } sntp;
} Zmienne_zapisywalne;
extern Zmienne_zapisywalne Const;
extern struct tm *sntpTime;
'''


def main():
    with tempfile.TemporaryDirectory(prefix="h7-settings-bindings-") as directory:
        directory = Path(directory)
        (directory / "variables.h").write_text(VARIABLES_STUB)
        # Quoted includes search the implementation directory first. Copy only
        # this translation unit to isolate MCU headers from a host build.
        shutil.copyfile(ROOT / "Application/HTTP/http_settings_bindings.c",
                        directory / "http_settings_bindings.c")
        executable = directory / "bindings-test"
        subprocess.run([
            "gcc", "-std=c11", "-Wall", "-Wextra", "-Werror",
            "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-no-pie",
            "-I" + str(directory), "-I" + str(ROOT / "Application"), "-I" + str(ROOT / "Application/HTTP"),
            str(directory / "http_settings_bindings.c"),
            str(ROOT / "Application/HTTP/http_settings_model.c"),
            str(ROOT / "Application/HTTP/http_settings_catalog.c"),
            str(ROOT / "tests/fixtures/http_settings_bindings_harness.c"),
            "-o", str(executable),
        ], check=True)
        subprocess.run([str(executable)], check=True,
                       env={**__import__("os").environ, "TZ": "UTC"})


if __name__ == "__main__":
    main()
