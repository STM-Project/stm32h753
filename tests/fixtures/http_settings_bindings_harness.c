#include "http_settings_bindings.h"
#include "http_settings_model.h"
#include "variables.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

Zmienne_zapisywalne Const;
struct tm *sntpTime;
static unsigned char blob[HTTP_SETTINGS_BLOB_SIZE];
static int have_blob, fail_save;
static char target[HTTP_SETTINGS_TARGET_MAX + 1u];

static int load(void *data, size_t length, void *context)
{
    (void)context;
    assert(length == sizeof blob);
    if (!have_blob) return 0;
    memcpy(data, blob, length);
    return 1;
}

static int save(const void *data, size_t length, void *context)
{
    (void)context;
    assert(length == sizeof blob);
    assert(HTTP_SettingsModelValidateBlob(data, length));
    if (fail_save) return -1;
    memcpy(blob, data, length);
    have_blob = 1;
    return 1;
}

static void append(const char *text)
{
    assert(strlen(target) + strlen(text) < sizeof target);
    strcat(target, text);
}

static void encoded(const char *text)
{
    char hex[4];
    while (*text) {
        unsigned char c = (unsigned char)*text++;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_' || c == '-') {
            hex[0] = (char)c; hex[1] = 0;
        } else {
            snprintf(hex, sizeof hex, "%%%02X", (unsigned)c);
        }
        append(hex);
    }
}

/* Submit an actual complete form, with a small list of field overrides. */
static int submit(const char *path, const char *const *overrides)
{
    uint16_t instance, result_instance;
    const HTTP_SettingPage *page = HTTP_SettingsCatalogFind(path, &instance), *result_page;
    unsigned i;
    int status;
    assert(page && instance == 0);
    strcpy(target, "/lpc.cgi/A117?h7_page="); encoded(path);
    append("&h7_form=0&n_109_zapisz=");
    for (i = 0; i < page->field_count; ++i) {
        const HTTP_SettingField *field = &page->fields[i];
        const char *value = HTTP_SettingsGet(page, instance, field);
        unsigned n;
        if (field->form_id != 0 || field->kind == HTTP_SETTING_READONLY) continue;
        if (field->kind == HTTP_SETTING_SECRET) value = "";
        for (n = 0; overrides && overrides[n]; n += 2u)
            if (strcmp(overrides[n], field->name) == 0) { value = overrides[n + 1u]; break; }
        append("&"); encoded(field->name); append("="); encoded(value);
    }
    status = HTTP_SettingsApplyTarget(target, strlen(target), &result_page, &result_instance);
    assert(result_page == page && result_instance == 0);
    if (status == HTTP_SETTINGS_SAVED)
        HTTP_SettingsBindingsCommitted(result_page, result_instance);
    return status;
}

int main(void)
{
    static const char *const wifi[] = {
        "n_s_Wifi_ssid_001", "test station",
        "n_s_Wifi_haslo_001", "test-station-secret",
        "n_s_Wifi_ssid_002", "test access point",
        "n_s_Wifi_haslo_002", "test-access-secret",
        "n_s_Wifi_par1_001", "tak", "n_s_Wifi_par1_002", "nie",
        "n_s_Lan_ip_002", "192.168.4.250", "n_s_Lan_mask_002", "255.255.255.0",
        "n_s_Lan_br_002", "192.168.4.1", "n_s_Lan_mac_002", "02ABCDEF1234",
        "n_s_Lan_port_002", "8080", "n_s_Lan_param_002", "nie", NULL
    };
    static const char *const mail[] = {
        "n_s_email_ip_000", "smtp.example.test", "n_s_email_port_000", "465",
        "n_s_email_nad_000", "sender@example.test", "n_s_email_has_000", "test-email-secret",
        "n_s_email_odb1_000", "r1@example.test", "n_s_email_odb2_000", "r2@example.test",
        "n_s_email_odb3_000", "r3@example.test", "n_s_email_odb4_000", "r4@example.test",
        "n_s_email_odb5_000", "r5@example.test", "n_s_email_odb6_000", "r6@example.test",
        "n_s_email_odb7_000", "r7@example.test", "n_s_email_odb8_000", "r8@example.test", NULL
    };
    static const char *const numeric_mail[] = {"n_s_email_ip_000", "203.0.113.45", NULL};
    static const char *const bad_wifi[] = {"n_s_Lan_ip_002", "256.0.0.1", NULL};
    static const char *const clock_form[] = {
        "n_s_Czas_rok_000", "24", "n_s_Czas_mies_000", "02", "n_s_Czas_dzien_000", "29",
        "n_s_Czas_godz_000", "12", "n_s_Czas_min_000", "34", "n_s_Czas_sec_000", "56", NULL
    };
    static const char *const bad_clock[] = {"n_s_Czas_dzien_000", "30", NULL};
    static const char *const far_clock[] = {"n_s_Czas_rok_000", "99", "n_s_Czas_mies_000", "01", NULL};
    Zmienne_zapisywalne snapshot;
    struct tm calendar = {0};
    uint16_t instance;
    const HTTP_SettingPage *page;
    unsigned i;
    char expected[96];
    assert(HTTP_SettingsModelInit(load, save, NULL) == 0);
    memset(&Const, 0, sizeof Const);
    strcpy(Const.wifiSTA[1].name, "original station");
    strcpy(Const.wifiSTA[1].pass, "original-station-secret");
    strcpy(Const.wifiAP[2].name, "original access point");
    Const.wifiGeneral.nrSTA = 1; Const.wifiGeneral.nrAP = 2; Const.wifiGeneral.mode = 3;
    Const.wifiSTA[1].ip = 0x01020304; Const.wifiSTA[1].port = 80; Const.wifiAP[2].port = 80;
    Const.emailSend[0].IP = 0x01010101; Const.sntp.time = 77;
    strcpy(Const.emailSend[0].server, "smtp.long-host.example.test");
    strcpy(Const.emailSend[0].login, "original@example.test");
    strcpy(Const.emailRecv[0].email, "original-recipient@example.test");
    Const.emailSend[0].port = 465;
    memcpy(&snapshot, &Const, sizeof Const);
    HTTP_SettingsBindingsApply();
    assert(memcmp(&snapshot, &Const, sizeof Const) == 0);
    page = HTTP_SettingsCatalogFind("/wifi", &instance);
    assert(strcmp(HTTP_SettingsGetByName(page, instance, "n_s_Wifi_ssid_001"), "original station") == 0);
    assert(strcmp(HTTP_SettingsGetByName(page, instance, "n_s_Wifi_ssid_002"), "original access point") == 0);
    assert(strcmp(HTTP_SettingsGetByName(page, instance, "n_s_Lan_ip_002"), "1.2.3.4") == 0);
    assert(strcmp(HTTP_SettingsGetByName(page, instance, "n_s_Lan_port_002"), "80") == 0);
    assert(HTTP_SettingsGetSaved(page, instance, "n_s_Wifi_ssid_001") == NULL);
    assert(strcmp(HTTP_SettingsGetByName(page, instance, "n_s_Wifi_haslo_001"), "") == 0);
    assert(submit("/wifi", NULL) == HTTP_SETTINGS_SAVED);
    assert(memcmp(&snapshot, &Const, sizeof Const) == 0); /* first form preserves config */
    assert(strcmp(HTTP_SettingsGetSaved(page, instance, "n_s_Wifi_ssid_001"), "original station") == 0);
    page = HTTP_SettingsCatalogFind("/maskimail", &instance);
    assert(strcmp(HTTP_SettingsGetByName(page, instance, "n_s_email_ip_000"), "smtp.long-host.example.test") == 0);
    assert(strcmp(HTTP_SettingsGetByName(page, instance, "n_s_email_nad_000"), "original@example.test") == 0);
    assert(strcmp(HTTP_SettingsGetByName(page, instance, "n_s_email_odb1_000"), "original-recipient@example.test") == 0);
    assert(HTTP_SettingsGetSaved(page, instance, "n_s_email_ip_000") == NULL);
    calendar.tm_year = 126; calendar.tm_mon = 9; calendar.tm_mday = 17;
    calendar.tm_hour = 8; calendar.tm_min = 9; calendar.tm_sec = 10; calendar.tm_wday = 6;
    sntpTime = &calendar;
    page = HTTP_SettingsCatalogFind("/czas", &instance);
    assert(strcmp(HTTP_SettingsGetByName(page, instance, "n_s_Czas_rok_000"), "26") == 0);
    assert(strcmp(HTTP_SettingsGetByName(page, instance, "n_s_Czas_mies_000"), "10") == 0);
    assert(strcmp(HTTP_SettingsGetByName(page, instance, "n_s_Czas_godz_000"), "8") == 0);
    assert(strcmp(HTTP_SettingsGetByName(page, instance, "n_s_Czas_tyg_000"), "6") == 0);
    assert(HTTP_SettingsGetSaved(page, instance, "n_s_Czas_rok_000") == NULL);
    sntpTime = NULL;
    assert(submit("/wifi", wifi) == HTTP_SETTINGS_SAVED);
    assert(strcmp(Const.wifiSTA[1].name, "test station") == 0);
    assert(strcmp(Const.wifiSTA[1].pass, "test-station-secret") == 0);
    assert(strcmp(Const.wifiAP[2].name, "test access point") == 0);
    assert(Const.wifiGeneral.mode == 1 && Const.wifiSTA[1].dhcp == 0);
    assert(Const.wifiSTA[1].ip == UINT32_C(0xc0a804fa));
    assert(Const.wifiSTA[1].mask == UINT32_C(0xffffff00));
    assert(Const.wifiSTA[1].gate == UINT32_C(0xc0a80401));
    assert(Const.wifiSTA[1].mac == UINT64_C(0x02abcdef1234));
    assert(Const.wifiSTA[1].port == 8080 && Const.wifiAP[2].port == 8080);
    assert(Const.wifiSTA[0].port == 0 && Const.wifiAP[0].port == 0);
    memcpy(&snapshot, &Const, sizeof Const);
    assert(submit("/wifi", bad_wifi) == HTTP_SETTINGS_INVALID);
    assert(memcmp(&snapshot, &Const, sizeof Const) == 0);
    fail_save = 1;
    assert(submit("/wifi", NULL) == HTTP_SETTINGS_STORAGE_ERROR);
    assert(memcmp(&snapshot, &Const, sizeof Const) == 0);
    fail_save = 0;
    assert(submit("/wifi", NULL) == HTTP_SETTINGS_SAVED);
    assert(strcmp(Const.wifiSTA[1].pass, "test-station-secret") == 0);
    assert(submit("/maskimail", mail) == HTTP_SETTINGS_SAVED);
    assert(strcmp(Const.emailSend[0].server, "smtp.example.test") == 0);
    assert(Const.emailSend[0].IP == 0 && Const.emailSend[0].port == 465);
    assert(strcmp(Const.emailSend[0].login, "sender@example.test") == 0);
    for (i = 0; i < 8; ++i) {
        snprintf(expected, sizeof expected, "r%u@example.test", i + 1);
        assert(strcmp(Const.emailRecv[i].email, expected) == 0);
    }
    assert(submit("/maskimail", numeric_mail) == HTTP_SETTINGS_SAVED);
    assert(Const.emailSend[0].IP == UINT32_C(0xcb00712d));
    assert(strcmp(Const.emailSend[0].password, "test-email-secret") == 0);
    assert(submit("/czas", clock_form) == HTTP_SETTINGS_SAVED);
    assert(Const.sntp.time == 1709210096 && sntpTime == NULL);
    sntpTime = &calendar;
    assert(submit("/czas", NULL) == HTTP_SETTINGS_SAVED);
    assert(calendar.tm_year == 124 && calendar.tm_mon == 1 && calendar.tm_mday == 29);
    assert(calendar.tm_hour == 12 && calendar.tm_min == 34 && calendar.tm_sec == 56);
    assert(calendar.tm_wday == 4);
    assert(submit("/czas", bad_clock) == HTTP_SETTINGS_INVALID);
    assert(Const.sntp.time == 1709210096);
    assert(submit("/czas", far_clock) == HTTP_SETTINGS_SAVED);
    assert(Const.sntp.time == 1709210096); /* current runtime cannot represent 2099 */
    Const.sntp.time = 91;
    assert(HTTP_SettingsModelInit(load, save, NULL) == 1);
    memset(Const.wifiSTA[1].name, 0, sizeof Const.wifiSTA[1].name);
    HTTP_SettingsBindingsApply();
    assert(strcmp(Const.wifiSTA[1].name, "test station") == 0);
    assert(Const.sntp.time == 91); /* boot must not restore a stale saved date */
    memcpy(&snapshot, &Const, sizeof Const);
    page = HTTP_SettingsCatalogFind("/lan", &instance);
    HTTP_SettingsBindingsCommitted(page, instance);
    HTTP_SettingsBindingsCommitted(NULL, 0);
    HTTP_SettingsBindingsCommitted(page, 1);
    assert(memcmp(&snapshot, &Const, sizeof Const) == 0);
    /* Corrupt profile selectors are bounded independently of persisted forms. */
    Const.wifiGeneral.nrSTA = -1; Const.wifiGeneral.nrAP = WIFI_AP_MAX;
    HTTP_SettingsBindingsApply();
    assert(strcmp(Const.wifiSTA[0].name, "test station") == 0);
    assert(strcmp(Const.wifiAP[0].name, "test access point") == 0);
    puts("Settings bindings: saved WiFi/SMTP, transactional errors and manual clock passed");
    return 0;
}
