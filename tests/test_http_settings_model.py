#!/usr/bin/env python3
"""Check every imported settings schema and strict transactional CGI parsing."""
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

HARNESS = r'''
#include "http_settings_model.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static unsigned char saved[HTTP_SETTINGS_BLOB_SIZE], backup[HTTP_SETTINGS_BLOB_SIZE];
static char target[HTTP_SETTINGS_TARGET_MAX + 2000u];
static int have_saved, save_fails, load_fails, saves;
static const char *override_names[8], *override_values[8], *omit;
static unsigned override_count, form_count, instance_count;

static int load(void *data, size_t length, void *context) {
    assert(context == saved && length == sizeof saved);
    if (load_fails) return -1;
    if (!have_saved) return 0;
    memcpy(data, saved, length); return 1;
}
static int save(const void *data, size_t length, void *context) {
    assert(context == saved && length == sizeof saved);
    assert(HTTP_SettingsModelValidateBlob(data, length));
    ++saves;
    if (save_fails) return -1;
    memcpy(saved, data, length); have_saved = 1; return 1;
}
static void encode(const char *value) {
    static const char hex[] = "0123456789ABCDEF";
    size_t pos = strlen(target);
    while (*value) {
        unsigned char c = (unsigned char)*value++;
        assert(pos + 4u < sizeof target);
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_') target[pos++] = (char)c;
        else { target[pos++] = '%'; target[pos++] = hex[c >> 4]; target[pos++] = hex[c & 15u]; }
    }
    target[pos] = 0;
}
static void pair(const char *name, const char *value) {
    if (target[strlen(target) - 1u] != '?') strcat(target, "&");
    encode(name); strcat(target, "="); encode(value);
}
static void clear_overrides(void) { override_count = 0; omit = NULL; }
static void change(const char *name, const char *value) {
    assert(override_count < 8u);
    override_names[override_count] = name; override_values[override_count++] = value;
}
static void make(const HTTP_SettingPage *page, uint16_t instance, unsigned form, int checks) {
    unsigned f, n;
    char route[64], number[4], name[HTTP_SETTINGS_FIELD_NAME_MAX + 1u];
    assert(HTTP_SettingsCatalogPath(page, instance, route, sizeof route));
    assert(snprintf(number, sizeof number, "%u", form) > 0);
    strcpy(target, "/lpc.cgi/A117?"); pair("h7_page", route); pair("h7_form", number);
    for (f = 0; f < page->field_count; ++f) {
        const HTTP_SettingField *field = &page->fields[f];
        const char *value;
        if (field->form_id != form) continue;
        assert(HTTP_SettingsCatalogFieldName(field, instance, name, sizeof name));
        if (omit && strcmp(omit, name) == 0) continue;
        value = HTTP_SettingsGet(page, instance, field);
        assert(value);
        if (field->kind == HTTP_SETTING_SECRET) value = "";
        if (field->kind == HTTP_SETTING_CHECKBOX && checks >= 0) value = checks ? "1" : "0";
        for (n = 0; n < override_count; ++n)
            if (strcmp(override_names[n], name) == 0) value = override_values[n];
        if (field->kind == HTTP_SETTING_CHECKBOX && strcmp(value, "0") == 0) continue;
        pair(name, value);
    }
    pair(strcmp(page->route, "/czas") == 0 ? "n_czas_zapisz" : "n_109_zapisz", "");
}
static const HTTP_SettingPage *page_at(const char *route) {
    uint16_t instance;
    const HTTP_SettingPage *page = HTTP_SettingsCatalogFind(route, &instance);
    assert(page); return page;
}
static int apply(void) { return HTTP_SettingsApplyTarget(target, strlen(target), NULL, NULL); }
static void ok(void) {
    int result = apply();
    if (result != HTTP_SETTINGS_SAVED) fprintf(stderr, "result %d for %.180s\n", result, target);
    assert(result == HTTP_SETTINGS_SAVED);
}
static void reject(void) {
    int old_saves = saves;
    memcpy(backup, saved, sizeof saved);
    assert(apply() == HTTP_SETTINGS_INVALID);
    assert(saves == old_saves && memcmp(backup, saved, sizeof saved) == 0);
}
static void expect(const HTTP_SettingPage *page, uint16_t instance, const char *name, const char *value) {
    const char *actual = HTTP_SettingsGetByName(page, instance, name);
    assert(actual && strcmp(actual, value) == 0);
}
static void repair_checksum(void) {
    uint32_t crc = UINT32_C(0xffffffff);
    for (size_t i = 24u; i < sizeof saved; ++i) {
        crc ^= saved[i];
        for (unsigned bit = 0; bit < 8u; ++bit)
            crc = (crc >> 1) ^ ((crc & 1u) ? UINT32_C(0xedb88320) : 0u);
    }
    crc ^= UINT32_C(0xffffffff);
    for (unsigned i = 0; i < 4u; ++i) saved[20u + i] = (unsigned char)(crc >> (8u * i));
}
static void default_forms(void) {
    size_t p;
    for (p = 0; p < HTTP_SettingsCatalogCount(); ++p) {
        const HTTP_SettingPage *page = HTTP_SettingsCatalogAt(p);
        unsigned first = page->max_instance ? 1u : 0u;
        unsigned last = page->max_instance ? page->max_instance : 0u;
        for (unsigned instance = first; instance <= last; ++instance) {
            unsigned char forms[256] = {0};
            ++instance_count;
            for (unsigned f = 0; f < page->field_count; ++f) forms[page->fields[f].form_id] = 1;
            for (unsigned form = 0; form < 256u; ++form) if (forms[form]) {
                make(page, (uint16_t)instance, form, -1); ok(); ++form_count;
            }
            for (unsigned f = 0; f < page->field_count; ++f) {
                const char *value = HTTP_SettingsGet(page, (uint16_t)instance, &page->fields[f]);
                assert(value && strcmp(value, page->fields[f].default_value) == 0);
            }
        }
    }
}
static void parser_tests(void) {
    const HTTP_SettingPage *page = page_at("/wifi");
    static const char *const suffixes[] = {
        "&unknown=1", "&h7_schema=1", "&h7_page=%2Fwifi", "&h7_form=0",
        "&n_109_zapisz=", "&n_s_Lan_ip_002=127.0.0.1", "&broken=%",
        "&broken=%GG", "&broken=%00", "&broken=%0a", "&=1", "&x", "&", "&&x=1"
    };
    for (size_t i = 0; i < sizeof suffixes / sizeof suffixes[0]; ++i) {
        make(page, 0, 0, -1); strcat(target, suffixes[i]); reject();
    }
    clear_overrides(); change("n_s_Lan_ip_002", "999.1.1.1"); make(page, 0, 0, -1); reject();
    clear_overrides(); change("n_s_Lan_ip_002", "arbitrary text"); make(page, 0, 0, -1); reject();
    clear_overrides(); change("n_s_Lan_mask_002", "1.2.3"); make(page, 0, 0, -1); reject();
    clear_overrides(); change("n_s_Lan_port_002", "65536"); make(page, 0, 0, -1); reject();
    clear_overrides(); change("n_s_Lan_port_002", "1.5"); make(page, 0, 0, -1); reject();
    clear_overrides(); change("n_s_Lan_port_002", "text"); make(page, 0, 0, -1); reject();
    clear_overrides(); change("n_s_Lan_mac_002", "GGGGGGGGGGGG"); make(page, 0, 0, -1); reject();
    clear_overrides(); omit = "n_s_Wifi_par1_002"; make(page, 0, 0, -1); reject();
    clear_overrides(); change("n_s_Tim_godzON1_001", "24:00:00");
    make(page_at("/tim001"), 1, 0, -1); reject();
    clear_overrides(); change("n_s_WE_czas_reak_001", "00:60");
    make(page_at("/we001"), 1, 0, -1); reject();
    clear_overrides(); change("n_s_Wifi_ssid_002", "Pokoj & \"gora\" + 100%"); make(page, 0, 0, -1); ok();
    expect(page, 0, "n_s_Wifi_ssid_002", "Pokoj & \"gora\" + 100%");
    clear_overrides(); make(page, 0, 0, -1); size_t pos = strlen(target); target[pos++] = '&';
    memset(target + pos, 'x', 97u); pos += 97u; strcpy(target + pos, "=1"); reject();
    make(page, 0, 0, -1); pos = strlen(target); strcpy(target + pos, "&unknown="); pos += 9u;
    memset(target + pos, 'x', HTTP_SETTINGS_VALUE_MAX + 1u); target[pos + HTTP_SETTINGS_VALUE_MAX + 1u] = 0; reject();
    strcpy(target, "/lpc.cgi/A117?h7_page=%2Fwifi%3Fother&h7_form=0&n_109_zapisz="); reject();
    strcpy(target, "/lpc.cgi/A117?h7_page=%2Fwifi&h7_form=255&n_109_zapisz="); reject();
    strcpy(target, "/lpc.cgi/A117?h7_page=%2Fwifi&n_109_zapisz="); reject();
    memset(target, 'x', HTTP_SETTINGS_TARGET_MAX + 1u);
    assert(HTTP_SettingsApplyTarget(target, HTTP_SETTINGS_TARGET_MAX + 1u, NULL, NULL) == HTTP_SETTINGS_INVALID);
    const char no_nul[] = {'/', 's', 'e', 't'};
    assert(HTTP_SettingsApplyTarget(no_nul, sizeof no_nul, NULL, NULL) == HTTP_SETTINGS_NOT_HANDLED);
    assert(HTTP_SettingsApplyTarget(NULL, 1, NULL, NULL) == HTTP_SETTINGS_NOT_HANDLED);
    clear_overrides();
}
static const char *runtime_default(const HTTP_SettingPage *page, uint16_t instance,
                                   const HTTP_SettingField *field, void *context) {
    assert(context == saved);
    if (instance != 0u) return NULL;
    if (strcmp(page->route, "/wifi") == 0) {
        if (strcmp(field->name, "n_s_Wifi_ssid_002") == 0) return "Existing WiFi";
        if (strcmp(field->name, "n_s_Lan_ip_002") == 0) return "invalid IPv4";
    }
    if (strcmp(page->route, "/czas") == 0 && strcmp(field->name, "n_s_Czas_tyg_000") == 0)
        return "6";
    return NULL;
}
static void resolver_tests(void) {
    const HTTP_SettingPage *wifi = page_at("/wifi");
    assert(HTTP_SettingsModelInit(NULL, save, saved) == 0);
    HTTP_SettingsSetDefaultResolver(runtime_default, saved);
    expect(wifi, 0, "n_s_Wifi_ssid_002", "Existing WiFi");
    expect(wifi, 0, "n_s_Lan_ip_002", "");
    assert(HTTP_SettingsGetSaved(wifi, 0, "n_s_Wifi_ssid_002") == NULL);
    assert(HTTP_SettingsModelInit(NULL, save, saved) == 0);
    expect(wifi, 0, "n_s_Wifi_ssid_002", "Existing WiFi");
    clear_overrides(); change("n_s_Wifi_ssid_002", "Explicit WiFi"); make(wifi, 0, 0, -1); ok();
    expect(wifi, 0, "n_s_Wifi_ssid_002", "Explicit WiFi");
    clear_overrides(); make(page_at("/czas"), 0, 0, -1); ok();
    expect(page_at("/czas"), 0, "n_s_Czas_tyg_000", "6");
    assert(HTTP_SettingsGetSaved(page_at("/czas"), 0, "n_s_Czas_tyg_000") == NULL);
    HTTP_SettingsSetDefaultResolver(NULL, NULL);
    expect(page_at("/czas"), 0, "n_s_Czas_tyg_000", "--");
}
static void date_tests(void) {
    const HTTP_SettingPage *page = page_at("/czas");
    clear_overrides(); change("n_s_Czas_rok_000", "24"); change("n_s_Czas_mies_000", "02");
    change("n_s_Czas_dzien_000", "29"); make(page, 0, 0, -1); ok();
    clear_overrides(); change("n_s_Czas_rok_000", "23"); make(page, 0, 0, -1); reject();
    clear_overrides(); change("n_s_Czas_mies_000", "04"); change("n_s_Czas_dzien_000", "31");
    make(page, 0, 0, -1); reject();
    clear_overrides(); change("n_s_Czas_godz_000", "24"); make(page, 0, 0, -1); reject();
    clear_overrides(); change("n_s_Czas_min_000", "60"); make(page, 0, 0, -1); reject();
    clear_overrides(); change("n_s_Czas_mies_000", "00"); make(page, 0, 0, -1); reject();
    clear_overrides();
}
static void shared_tests(void) {
    const HTTP_SettingPage *tim = page_at("/tim001"), *wypk = page_at("/wypk001");
    const HTTP_SettingPage *term = page_at("/term001"), *we = page_at("/we001"), *eol = page_at("/eol001");
    const HTTP_SettingPage *sms = page_at("/maskisms"), *mail = page_at("/maskimail");
    clear_overrides(); change("n_s_PK_nazwa_001", "Shared output");
    change("n_s_PK_nazwa_on_001", "Start"); make(tim, 1, 0, -1); ok();
    expect(wypk, 1, "n_s_PK_nazwa_001", "Shared output");
    expect(term, 1, "n_s_PK_nazwa_on_001", "Start");
    expect(tim, 2, "n_s_PK_nazwa_002", "");
    clear_overrides(); change("n_s_PK_nazwa_001", "Shared thermostat"); make(term, 1, 0, -1); ok();
    expect(tim, 1, "n_s_PK_nazwa_001", "Shared thermostat");
    assert(strcmp(HTTP_SettingsGetSaved(wypk, 1, "n_s_PK_nazwa_001"), "Shared thermostat") == 0);
    clear_overrides(); change("n_s_WE_nazwa_001", "Shared input");
    change("n_s_WE_nazwa_uszk_001", "EOL only"); make(eol, 1, 0, -1); ok();
    expect(we, 1, "n_s_WE_nazwa_001", "Shared input");
    clear_overrides(); change("n_s_WE_czas_reak_001", "01:23"); make(we, 1, 0, -1); ok();
    expect(eol, 1, "n_s_WE_czas_reak_001", "01:23");
    expect(eol, 1, "n_s_WE_nazwa_uszk_001", "EOL only");
    clear_overrides(); change("n_s_RR_godzON1_000", "12:34"); make(mail, 0, 0, -1); ok();
    expect(sms, 0, "n_s_RR_godzON1_000", "12:34");
    clear_overrides(); change("n_s_PK_nazwa_001", "Failed shared write"); make(tim, 1, 0, -1);
    save_fails = 1; assert(apply() == HTTP_SETTINGS_STORAGE_ERROR); save_fails = 0;
    expect(wypk, 1, "n_s_PK_nazwa_001", "Shared thermostat");
    expect(term, 1, "n_s_PK_nazwa_001", "Shared thermostat");
    assert(HTTP_SettingsModelInit(load, save, saved) == 1);
    expect(wypk, 1, "n_s_PK_nazwa_001", "Shared thermostat");
    expect(eol, 1, "n_s_WE_czas_reak_001", "01:23");
    expect(sms, 0, "n_s_RR_godzON1_000", "12:34");
    clear_overrides();
}
static void persistence_tests(void) {
    const HTTP_SettingPage *wifi = page_at("/wifi"), *tim = page_at("/tim001");
    clear_overrides(); change("n_s_Wifi_haslo_002", "Test & + secret"); make(wifi, 0, 0, -1); ok();
    clear_overrides(); make(wifi, 0, 0, -1); ok();
    expect(wifi, 0, "n_s_Wifi_haslo_002", "Test & + secret");
    make(tim, 1, 0, 1); ok(); make(tim, 2, 0, 1); ok();
    make(tim, 1, 0, 0); ok();
    expect(tim, 1, "n_s_Tim_akt1_001", "0"); expect(tim, 2, "n_s_Tim_akt1_002", "1");
    make(page_at("/set"), 0, 0, -1); ok();
    expect(tim, 2, "n_s_Tim_akt1_002", "1");
    clear_overrides(); change("n_s_Wifi_ssid_002", "Must not commit");
    make(wifi, 0, 0, -1); memcpy(backup, saved, sizeof saved); save_fails = 1;
    assert(apply() == HTTP_SETTINGS_STORAGE_ERROR);
    assert(memcmp(saved, backup, sizeof saved) == 0);
    expect(wifi, 0, "n_s_Wifi_ssid_002", "Pokoj & \"gora\" + 100%");
    save_fails = 0; clear_overrides();
    assert(HTTP_SettingsModelInit(load, save, saved) == 1);
    expect(wifi, 0, "n_s_Wifi_haslo_002", "Test & + secret");
    expect(tim, 2, "n_s_Tim_akt1_002", "1");
    assert(HTTP_SettingsModelValidateBlob(saved, sizeof saved));
    assert(!HTTP_SettingsModelValidateBlob(saved, sizeof saved - 1u));
    assert(!HTTP_SettingsModelValidateBlob(NULL, sizeof saved));
    saved[100] ^= 1u;
    assert(!HTTP_SettingsModelValidateBlob(saved, sizeof saved));
    assert(HTTP_SettingsModelInit(load, save, saved) == -1);
    expect(wifi, 0, "n_s_Wifi_haslo_002", "Test & + secret");
    saved[100] ^= 1u;
    saved[16] ^= 1u; assert(!HTTP_SettingsModelValidateBlob(saved, sizeof saved)); saved[16] ^= 1u;
    memcpy(backup, saved, sizeof saved);
    saved[28] = 255u; saved[29] = 255u; repair_checksum();
    assert(!HTTP_SettingsModelValidateBlob(saved, sizeof saved));
    memcpy(saved, backup, sizeof saved);
    memcpy(saved + 24u + HTTP_SETTINGS_RECORD_SIZE, saved + 24u, HTTP_SETTINGS_RECORD_SIZE);
    repair_checksum(); assert(!HTTP_SettingsModelValidateBlob(saved, sizeof saved));
    memcpy(saved, backup, sizeof saved);
    saved[sizeof saved - 1u] = 1u; repair_checksum();
    assert(!HTTP_SettingsModelValidateBlob(saved, sizeof saved));
    memcpy(saved, backup, sizeof saved);
    load_fails = 1; assert(HTTP_SettingsModelInit(load, save, saved) == -1); load_fails = 0;
    expect(wifi, 0, "n_s_Wifi_haslo_002", "Test & + secret");
    assert(HTTP_SettingsModelInit(NULL, NULL, NULL) == 0);
    make(wifi, 0, 0, -1); assert(apply() == HTTP_SETTINGS_STORAGE_ERROR);
}
int main(void) {
    assert(HTTP_SettingsModelInit(load, save, saved) == 0);
    assert(HTTP_SettingsGetByName(NULL, 0, "x") == NULL);
    assert(HTTP_SettingsGetByName(page_at("/wifi"), 1, "x") == NULL);
    default_forms(); shared_tests(); parser_tests(); date_tests(); persistence_tests(); resolver_tests();
    printf("PASS: %u instances, %u complete forms, shared fields, runtime defaults, calendar/ranges, scoped checkboxes, secrets, CRC/reload, transactional storage, strict bounded CGI (ASan/UBSan).\n", instance_count, form_count);
    return 0;
}
'''


def main():
    compiler = shutil.which("gcc") or shutil.which("clang")
    if not compiler:
        raise SystemExit("A host C compiler is required")
    with tempfile.TemporaryDirectory(prefix="http-settings-model-") as directory:
        tmp = Path(directory)
        harness = tmp / "settings_test.c"
        harness.write_text(HARNESS)
        executable = tmp / "settings_test"
        subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra", "-Werror", "-pedantic",
                        "-Wno-overlength-strings", "-O1", "-g", "-fsanitize=address,undefined",
                        "-fno-omit-frame-pointer", "-I" + str(ROOT / "Application"), str(harness),
                        str(ROOT / "Application/http_settings_model.c"),
                        str(ROOT / "Application/http_settings_catalog.c"), "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
