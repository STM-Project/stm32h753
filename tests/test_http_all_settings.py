#!/usr/bin/env python3
"""Verify the public typed settings pointer against real commits and reloads."""
import subprocess
import tempfile
from pathlib import Path

import test_http_settings_model as model

ROOT = Path(__file__).resolve().parents[1]
TESTS = r'''
#include "http_all_settings.h"
static HTTP_AllSettings snapshot;
static unsigned char sensor_saved[HTTP_SENSOR_SETTINGS_BLOB_SIZE];
static int sensor_fails;
static int sensor_save(const void *blob, size_t size, void *context) {
    (void)context; assert(size==sizeof sensor_saved);
    if(sensor_fails) return -1;
    memcpy(sensor_saved,blob,size);return 1;
}
static int sensor_load(void *blob, size_t size, void *context) {
    (void)context; assert(size==sizeof sensor_saved);
    memcpy(blob,sensor_saved,size);return 1;
}
int main(void) {
    assert(base_suite()==0);
    assert(HTTP_SettingsModelInit(NULL,save,saved)==0);
    assert(HTTP_SensorSettingsInit(NULL,sensor_save,NULL)==0);
    const HTTP_AllSettings *cfg=HTTP_SettingsGetAll();
    assert(cfg==&HTTP_AllSettingsData);
    assert(cfg->ports[0].mode==2 && cfg->ports[32].mode==0);
    assert(!strcmp(cfg->ports[0].name,"Nazwa portu nr 1"));
    assert(!strcmp(cfg->sensors[0][0].name,"Temperatura"));
    clear_overrides();change("n_s_Tim_godzON1_001","08:30:45");
    change("n_s_Tim_akt8_001","tak");change("n_s_PK_nazwa_001","Pompa");
    make(page_at("/tim001"),1,0,1);ok();
    assert(cfg==HTTP_SettingsGetAll());
    assert(cfg->timers[0].on[0].hour==8 && cfg->timers[0].on[0].minute==30 && cfg->timers[0].on[0].second==45);
    assert(cfg->timers[0].enabled==1);
    for(unsigned day=0;day<7;day++)assert(cfg->timers[0].weekdays[day]==1);
    assert(!strcmp(cfg->outputs[0].name,"Pompa"));
    assert(!strcmp(cfg->timers[0].output.name,"Pompa"));
    assert(!strcmp(cfg->thermostats[0].output.name,"Pompa"));
    clear_overrides();change("n_s_Term_prog_002","-12.3");change("n_s_Term_hist_002","+01.2");
    change("n_s_Term_czas_reak_002","02:30");
    make(page_at("/term002"),2,0,-1);ok();
    assert(cfg->thermostats[1].threshold_x100==-1230 && cfg->thermostats[1].hysteresis_x100==120);
    assert(cfg->thermostats[1].delay_seconds==150);
    clear_overrides();change("n_s_Lan_ip_002","192.168.1.2");change("n_s_Lan_mac_002","AABBCCDDEEFF");
    change("n_s_Wifi_haslo_002","Saved secret");make(page_at("/wifi"),0,0,-1);ok();
    assert(cfg->wifi.Lan_ip_002==UINT32_C(0xc0a80102));
    assert(cfg->wifi.Lan_mac_002==UINT64_C(0xaabbccddeeff));
    assert(!strcmp(cfg->wifi.Wifi_haslo_002,"Saved secret"));
    clear_overrides();change("n_s_Port_nazwa33_001","Wyjscie 33");make(page_at("/set"),0,0,-1);ok();
    assert(!strcmp(cfg->ports[32].name,"Wyjscie 33"));
    clear_overrides();change("n_GPIO_val_033","11");make(page_at("/set"),0,1,-1);ok();
    assert(cfg->ports[32].mode==11);
    /* A failed write/invalid form must not publish partially changed data. */
    memcpy(&snapshot,cfg,sizeof snapshot);
    clear_overrides();change("n_s_Tim_godzON1_001","09:00");make(page_at("/tim001"),1,0,0);
    save_fails=1;assert(apply()==HTTP_SETTINGS_STORAGE_ERROR);save_fails=0;
    assert(!memcmp(&snapshot,cfg,sizeof snapshot));
    clear_overrides();change("n_s_Tim_godzON1_001","99:00");make(page_at("/tim001"),1,0,0);reject();
    assert(!memcmp(&snapshot,cfg,sizeof snapshot));
    /* Reload restores the typed values and the stable global address. */
    assert(HTTP_SettingsModelInit(load,save,saved)==1);
    assert(cfg==HTTP_SettingsGetAll() && cfg->timers[0].on[0].hour==8);
    assert(cfg->ports[32].mode==11 && cfg->thermostats[1].threshold_x100==-1230);
    assert(!strcmp(cfg->wifi.Wifi_haslo_002,"Saved secret"));
    memcpy(&snapshot,cfg,sizeof snapshot);saved[100]^=1u;
    assert(HTTP_SettingsModelInit(load,save,saved)==-1);
    assert(!memcmp(&snapshot,cfg,sizeof snapshot));saved[100]^=1u;
    /* Sensors use a separate legacy store, but the same public root pointer. */
    const char *sensor_target="/lpc.cgi/A117?n_s_Czujki_nazwa_009=Salon&"
        "n_s_Czujki_prog_max1_009=26.0&n_s_Czujki_prog_max2_009=30.0&"
        "n_s_Czujki_prog_min1_009=-10.0&n_s_Czujki_prog_min2_009=-20.0&"
        "n_s_Czujki_hist_009=0.5&n_s_Czujki_korekcja_009=-1.2&"
        "n_s_Czujki_czas_reak_009=01:30&n_109_zapisz=";
    assert(HTTP_SensorSettingsApplyTarget(sensor_target,strlen(sensor_target),NULL)==1);
    assert(!strcmp(cfg->sensors[4][0].name,"Salon") && cfg->sensors[4][0].max1==260);
    assert(cfg->sensors[4][0].delay_seconds==90 && cfg->timers[0].on[0].hour==8);
    memcpy(&snapshot,cfg,sizeof snapshot);sensor_fails=1;
    assert(HTTP_SensorSettingsApplyTarget(sensor_target,strlen(sensor_target),NULL)==HTTP_SENSOR_SETTINGS_STORAGE_ERROR);
    sensor_fails=0;assert(!memcmp(&snapshot,cfg,sizeof snapshot));
    assert(HTTP_SensorSettingsInit(sensor_load,sensor_save,NULL)==1);
    assert(!strcmp(cfg->sensors[4][0].name,"Salon"));
    assert(HTTP_SettingsModelInit(load,save,saved)==1);
    assert(!strcmp(cfg->sensors[4][0].name,"Salon"));
    printf("PASS: one stable pointer, typed values/units, shared output aliases, secrets, sensors, transactional rollback and reload; root %zu bytes.\n",sizeof *cfg);
    return 0;
}
'''


def main():
    subprocess.run(["python3", str(ROOT / "tools/generate_settings_struct.py"), "--check"], check=True)
    with tempfile.TemporaryDirectory(prefix="h7-all-settings-") as directory:
        directory = Path(directory)
        test = directory / "all.c"
        test.write_text(model.HARNESS.replace("int main(void)", "static int base_suite(void)") + TESTS)
        executable = directory / "all"
        subprocess.run(["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-O1", "-g",
                        "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-no-pie",
                        "-I" + str(ROOT / "Application"), "-I" + str(ROOT / "Application/HTTP"), str(test),
                        *[str(ROOT / "Application/HTTP" / name) for name in
                          ("http_settings_model.c", "http_settings_catalog.c", "http_sensor_settings.c")],
                        "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
