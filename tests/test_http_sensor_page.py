#!/usr/bin/env python3
"""Render the Sandwitch sensor form with the real host model and shared CSS.

Run: python3 tests/test_http_sensor_page.py
No STM32 board, SD card, credentials or web service is required.
"""
from collections import Counter
from html.parser import HTMLParser
from pathlib import Path
import json
import re
import shutil
import subprocess
import tempfile
from urllib.parse import urlencode

from test_settings_templates import literal


ROOT = Path(__file__).resolve().parents[1]
FIELDS = {
    "nazwa", "prog_max1", "prog_max2", "prog_min1", "prog_min2", "hist",
    "korekcja", "czas_reak",
}


class PageControls(HTMLParser):
    def __init__(self):
        super().__init__()
        self.inputs = {}
        self.forms = []
        self.buttons = []
        self.classes = Counter()
        self.links = []
        self.text = []

    def handle_starttag(self, tag, attributes):
        attrs = dict(attributes)
        self.classes.update(attrs.get("class", "").split())
        if tag == "input":
            assert attrs["name"] not in self.inputs, "Duplicate form field"
            self.inputs[attrs["name"]] = attrs
        if tag == "form":
            self.forms.append(attrs)
        if tag == "button":
            self.buttons.append(attrs)
        if tag == "a":
            self.links.append(attrs.get("href"))

    def handle_data(self, data):
        self.text.append(data)


HARNESS = r'''
#include "http_sensor_page.h"
#include "http_sensor_settings.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned saves;

static int save_settings(const void *blob, size_t length, void *context)
{
    assert(context == &saves);
    assert(HTTP_SensorSettingsValidateBlob(blob, length));
    ++saves;
    return 1;
}

static void write_page(const char *name, const char *body, size_t length)
{
    FILE *file = fopen(name, "wb");
    assert(file != NULL);
    assert(fwrite(body, 1, length, file) == length);
    assert(fclose(file) == 0);
}

int main(int argc, char **argv)
{
    char *body = malloc(65536);
    char *bounded;
    size_t length, full;
    unsigned port;
    int humidity;
    assert(body != NULL && argc == 2);
    assert(HTTP_SensorSettingsInit(NULL, save_settings, &saves) == 0);
    assert(HTTP_SensorSettingsApplyTarget(argv[1], strlen(argv[1]), &port) ==
           HTTP_SENSOR_SETTINGS_SAVED);
    assert(port == 1 && saves == 1);
    for (port = 1; port <= HTTP_SENSOR_PORT_COUNT; ++port) {
        for (humidity = 0; humidity <= 1; ++humidity) {
            char filename[32];
            length = HTTP_SensorPageRender(body, 65536, port, humidity, NULL);
            assert(length > 0 && length == strlen(body));
            snprintf(filename, sizeof filename, "page-%02u-%d.html", port, humidity);
            write_page(filename, body, length);
        }
    }
    length = HTTP_SensorPageRender(body, 65536, 1, 1, "Saved <&>");
    assert(length > 0);
    write_page("notice.html", body, length);
    assert(HTTP_SensorPageRender(body, 65536, 0, 1, NULL) == 0 && body[0] == 0);
    assert(HTTP_SensorPageRender(body, 65536, HTTP_SENSOR_PORT_COUNT + 1, 1, NULL) == 0);
    assert(body[0] == 0);
    assert(HTTP_SensorPageRender(NULL, 65536, 1, 1, NULL) == 0);
    full = HTTP_SensorPageRender(body, 65536, 1, 1, NULL);
    assert(full > 0);
    bounded = malloc(full + 2);
    assert(bounded != NULL);
    /* Guard the byte beyond capacity, including very small capacities. */
    for (size_t capacity = 1; capacity <= full; capacity += capacity < 128 ? 1 : 127) {
        memset(bounded, 0x55, full + 2);
        assert(HTTP_SensorPageRender(bounded, capacity, 1, 1, NULL) == 0);
        assert(bounded[0] == 0 && (unsigned char)bounded[capacity] == 0x55);
    }
    assert(HTTP_SensorPageRender(bounded, full, 1, 1, NULL) == 0);
    assert(bounded[0] == 0);
    memset(bounded, 0x55, full + 2);
    assert(HTTP_SensorPageRender(bounded, full + 1, 1, 1, NULL) == full);
    assert(strcmp(bounded, body) == 0 && (unsigned char)bounded[full + 1] == 0x55);
    bounded[0] = 'x';
    assert(HTTP_SensorPageRender(bounded, 0, 1, 1, NULL) == 0 && bounded[0] == 'x');
    printf("PASS: renderer capacity checks, invalid arguments and real model save; %zu bytes.\n", full);
    free(bounded);
    free(body);
    return 0;
}
'''


def main():
    source = (ROOT / "Application/wwwPages.c").read_text()
    style = literal(source, "HttpStyle")
    menu = literal(source, "HttpMainMenu")
    values = {
        1: {
            "nazwa": "T\"<&'%", "prog_max1": "+3276.7", "prog_max2": "+55.0",
            "prog_min1": "-3276.8", "prog_min2": "-15.0", "hist": "+0.5",
            "korekcja": "-1.2", "czas_reak": "02:05",
        },
        2: {
            "nazwa": "RH & humidity", "prog_max1": "+100.0", "prog_max2": "+95.0",
            "prog_min1": "0.0", "prog_min2": "5.0", "hist": "+4.0",
            "korekcja": "0.0", "czas_reak": "00:59",
        },
    }
    query = {
        f"n_s_Czujki_{field}_{sensor:03}": value
        for sensor, fields in values.items() for field, value in fields.items()
    }
    query["n_109_zapisz"] = ""
    target = "/lpc.cgi/A117?" + urlencode(query)

    with tempfile.TemporaryDirectory(prefix="http-sensor-page-") as directory:
        tmp = Path(directory)
        fixtures = tmp / "shared.c"
        fixtures.write_text(
            "const char HttpStyle[]=" + json.dumps(style) + ";\n" +
            "const char HttpMainMenu[]=" + json.dumps(menu) + ";\n"
        )
        harness = tmp / "test.c"
        harness.write_text(HARNESS)
        binary = tmp / "test"
        subprocess.run([
            "gcc", "-std=c11", "-Wall", "-Wextra", "-Werror",
            "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
            "-I" + str(ROOT / "Application"), str(harness), str(fixtures),
            str(ROOT / "Application/http_sensor_page.c"),
            str(ROOT / "Application/http_sensor_settings.c"), "-o", str(binary),
        ], check=True)
        subprocess.run([str(binary), target], cwd=tmp, check=True)

        for port in range(1, 21):
            for humidity in range(2):
                page = (tmp / f"page-{port:02}-{humidity}.html").read_bytes().decode()
                controls = PageControls()
                controls.feed(page)
                cards = humidity + 1
                assert page.startswith(style + menu)
                assert page.count("<body ") == page.count("</body>") == 1
                assert page.count("</html>") == 1
                assert controls.classes["corner-radius2"] == cards + 1
                assert controls.classes["corner-radius"] == cards + 1
                assert controls.forms == [{
                    "method": "get", "action": "/lpc.cgi/A117", "data-ajax": "false",
                }]
                assert len([b for b in controls.buttons if b.get("name") == "n_109_zapisz"]) == 1
                assert "/set" in controls.links
                assert "maskujKlawisze" in page
                for label in ["Prog max", "Prog min", "Histereza", "Korekta", "Czas zwloki", "m:s"]:
                    assert "".join(controls.text).count(label) >= cards
                assert f"Temperatura Port {port}" in page
                assert (f"Wilgotnosc Port {port}" in page) == bool(humidity)
                expected = {
                    f"n_s_Czujki_{field}_{sensor:03}"
                    for sensor in range(2 * port - 1, 2 * port + humidity)
                    for field in FIELDS
                }
                assert set(controls.inputs) == expected
                for name, attrs in controls.inputs.items():
                    assert attrs["type"] == "text"
                    assert attrs["class"] == "textbox"
                    field = re.fullmatch(r"n_s_Czujki_(.*)_\d{3}", name)[1]
                    assert attrs["style"] == ("width:270px;" if field == "nazwa" else "width:50px;")
                    assert attrs["maxlength"] == {"nazwa": "30", "czas_reak": "5"}.get(field, "7")
                if port == 1:
                    assert controls.inputs["n_s_Czujki_nazwa_001"]["value"] == values[1]["nazwa"]
                    assert controls.inputs["n_s_Czujki_prog_max1_001"]["value"] == "+3276.7"
                    assert controls.inputs["n_s_Czujki_prog_min1_001"]["value"] == "-3276.8"
                    assert controls.inputs["n_s_Czujki_korekcja_001"]["value"] == "-01.2"
                    assert controls.inputs["n_s_Czujki_czas_reak_001"]["value"] == "02:05"
                    assert "T&quot;&lt;&amp;&#39;%" in page
                    if humidity:
                        assert controls.inputs["n_s_Czujki_prog_max1_002"]["value"] == "+100.0"
                        assert controls.inputs["n_s_Czujki_czas_reak_002"]["value"] == "00:59"
                assert "&deg;C" in page
                if humidity:
                    assert '<font class="css-font-style2">%</font>' in page

        notice = (tmp / "notice.html").read_bytes().decode()
        assert 'role="status"' in notice and "Saved &lt;&amp;&gt;" in notice
        scripts = re.findall(r"<script[^>]*>(.*?)</script>", notice, re.S)
        if shutil.which("node"):
            for index, script in enumerate(scripts):
                script_file = tmp / f"script-{index}.js"
                script_file.write_text(script)
                subprocess.run(["node", "--check", str(script_file)], check=True)
        arm = Path("/workspace/.stm32-tools/root/usr/bin/arm-none-eabi-gcc")
        if arm.exists():
            subprocess.run([
                str(arm), "-std=c11", "-Wall", "-Wextra", "-Werror",
                "-mcpu=cortex-m7", "-mthumb", "-mfpu=fpv5-d16", "-mfloat-abi=hard",
                "-isystem", "/workspace/.stm32-tools/root/usr/include/newlib",
                "-I" + str(ROOT / "Application"), "-c",
                str(ROOT / "Application/http_sensor_page.c"), "-o", str(tmp / "page.o"),
            ], check=True)
            print("PASS: ARM Cortex-M7 renderer compilation.")
        print("PASS: all 20 ports, one/two Sandwitch cards, 8 fields per sensor, stored values, escaping and JavaScript syntax; ASan/UBSan.")


if __name__ == "__main__":
    main()
