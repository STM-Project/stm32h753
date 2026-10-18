#!/usr/bin/env python3
"""Check imported Sandwitch identifiers, H753 port capabilities and catalog API.

The checked-in reference contains HTML literals only. Tests do not require the
legacy firmware, an SD card, STM32 headers or any device configuration/secrets.
"""
from collections import Counter
from html.parser import HTMLParser
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("settings_import", ROOT / "tools/import_sandwitch_settings.py")
importer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(importer)


class SourceControls(HTMLParser):
    def __init__(self, text):
        super().__init__(convert_charrefs=True)
        self.controls = {}
        self.select = None
        self.feed(text)

    def handle_starttag(self, tag, attributes):
        attrs = dict(attributes)
        if tag in ("input", "select") and "name" in attrs:
            assert attrs["name"] not in self.controls, attrs["name"]
            self.controls[attrs["name"]] = (attrs, [] if tag == "select" else None)
            if tag == "select":
                self.select = attrs["name"]
        elif tag == "option" and self.select:
            self.controls[self.select][1].append(attrs)

    def handle_endtag(self, tag):
        if tag == "select":
            self.select = None


def field_names(page):
    return {f["name"] for f in page["fields"]}


def verify_source_controls(pages, templates):
    # The offsets below are the original HttpPagesBuffer assembly contract.
    fragments = {
        "/lan": [("HttpUstawIP", "001"), ("HttpStrWWW", None)],
        "/wifi": [("HttpUstawIP", "002"), ("HttpUstawWifiAP", "001"), ("HttpUstawWifiAP", "002")],
        "/czas": [("HttpCzas", None), ("HttpEkr", None)],
        "/gsm": [("HttpUstawGSM", None)],
        "/nrf": [("HttpLora", None)],
        "/we": [("HttpWeCyfr", "{{N}}")],
        "/eol": [("HttpWeEOL", "{{N}}")],
        "/wypk": [("HttpWyCyfr", "{{N}}")],
        "/tim": [("HttpTimer", "{{N}}"), ("HttpWyCyfr", "{{N}}")],
        "/term": [("HttpTermost", "{{N}}"), ("HttpWyCyfr", "{{N}}")],
        "/etr": [("HttpEtr", "{{N}}")],
        "/rej": [("HttpRejestr", None)],
        "/maskimail": [("HttpUstawEmailNad", None)],
    }
    checked = 0
    for route, sources in fragments.items():
        fields = {f["name"]: f for f in pages[route]["fields"]}
        expected = set()
        for name, instance in sources:
            for identifier, (attrs, choices) in SourceControls(templates[name]).controls.items():
                converted = re.sub(r"_000$", "_" + instance, identifier) if instance else identifier
                expected.add(converted)
                field = fields[converted]
                if choices is None:
                    if attrs.get("type") == "checkbox":
                        assert field["kind"] == "CHECKBOX", converted
                        assert field["max_length"] == 1, converted
                        continue
                    limit = int(attrs.get("maxlength", "40"))
                    if converted == "n_s_email_ip_000":
                        limit = 95  # Current H753 sender.server[96], including hostnames.
                    elif converted.startswith(("n_s_Wifi_ssid_", "n_s_Wifi_haslo_")):
                        limit = 63  # Current H753 WiFi profile capacity.
                    assert field["max_length"] == limit, converted
                    if "readonly" in attrs or "Czas_tyg_" in identifier:
                        assert field["kind"] == "READONLY", converted
                    elif "00:00" in attrs.get("value", ""):
                        assert field["default_value"] == attrs["value"], converted
                    elif "+00.0" in attrs.get("value", ""):
                        assert field["default_value"] == attrs["value"], converted
                elif choices:
                    values = [option.get("value", "") for option in choices]
                    selected = next((o.get("value", "") for o in choices if "selected" in o), values[0])
                    assert field["choices"].split("|") == values, converted
                    assert field["default_value"] == selected, converted
                checked += 1
        if route not in ("/etr", "/maskimail"):
            assert set(fields) == expected, route
    return checked


def verify_procedural_fields(pages):
    # Original HttpMaskiSmsEmail_* names; all 20 H753 inputs are retained.
    for route, kind, contact, extra in (("/maskisms", "S", "nrtel", ("xerrr", "derrr")),
                                        ("/maskimail", "E", "email_odb", ("merrr",))):
        expected = {f"n_s_{contact}{user}_000" for user in range(1, 9)}
        expected |= {f"n_s_{prefix}{user:03}_000" for prefix in extra for user in range(1, 9)}
        expected |= {f"n_s_Ma{kind}_akt{port:03}_{user:03}" for port in range(1, 21) for user in range(1, 9)}
        expected.add("n_s_RR_godzON1_000")
        if kind == "E":
            expected |= {"n_s_email_tryb_000", "n_s_email_ip_000", "n_s_email_nad_000",
                         "n_s_email_has_000", "n_s_email_port_000"}
        assert field_names(pages[route]) == expected, route
    expected = {f"n_s_tt{part}{{{{I}}}}_{row:03}" for row in range(100) for part in "pdm"}
    expected.add("n_s_ttj{{I}}_000")
    assert field_names(pages["/nap"]) == expected
    expected = {f"n_s_Rot_akt{cycle:03}_{output:03}" for cycle in range(1, 9) for output in range(1, 14)}
    expected |= {"n_s_xxROTxx"} | {f"n_s_Rot_{name}_001" for name in ("dob", "int", "sta", "max", "min", "rea", "hist")}
    assert field_names(pages["/Rot"]) == expected
    for f in pages["/Rot"]["fields"]:
        if f["kind"] == "DECIMAL":
            assert f["max_length"] == 5, f["name"]
    expected = {f"n_s_pEtr{period}{part}{number}_{{{{N}}}}" for period in "gdtm" for part in "du" for number in (1, 2)}
    expected |= {"n_s_Imp_licz_{{N}}", "n_s_Imp_wart_{{N}}", "n_s_Imp_jedn_{{N}}"}
    assert field_names(pages["/etr"]) == expected
    # Each logic output has independently addressable first/second equations.
    assert len(field_names(pages["/logic"])) == 92
    assert {"n_s_OL0a", "n_s_SW1a", "n_s_OL0b", "n_s_SW1b"} <= field_names(pages["/logic"])


def verify_h753_modes(pages):
    fields = {f["name"]: f for f in pages["/set"]["fields"]}
    defaults = [2] * 3 + [4] * 7 + [6] * 17 + [0] * 4 + [6] * 2
    # Exercise the actual getter: defaults now come from the catalog rather
    # than a second, duplicated C array in wwwPages.c.
    source = (ROOT / "Application/wwwPages.c").read_text()
    getter = source[source.index("unsigned HTTP_SettingsPortMode(unsigned port)"):
                    source.index("const char* HTTP_SettingsPortName(")]
    program = ('#include <stdlib.h>\n#include <stdio.h>\n#include <assert.h>\n'
               '#include "http_settings_model.h"\n#define mini_snprintf snprintf\n'
               'enum { HTTP_MODE_DISABLED=6 };\n' + getter +
               '\nint main(void){unsigned expected[]={' + ','.join(map(str, defaults)) +
               '};assert(HTTP_SettingsModelInit(NULL,NULL,NULL)==0);'
               'for(unsigned i=0;i<33;i++)assert(HTTP_SettingsPortMode(i+1)==expected[i]);'
               'assert(HTTP_SettingsPortMode(0)==6 && HTTP_SettingsPortMode(34)==6);return 0;}')
    with tempfile.TemporaryDirectory(prefix="settings-mode-defaults-") as directory:
        test = Path(directory) / "modes.c"
        test.write_text(program)
        binary = Path(directory) / "modes"
        subprocess.run(["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-I" + str(ROOT / "Application"), "-I" + str(ROOT / "Application/HTTP"),
                        str(test), str(ROOT / "Application/HTTP/http_settings_catalog.c"),
                        str(ROOT / "Application/HTTP/http_settings_model.c"), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
    assert Counter(f["form_id"] for f in fields.values()) == {0: 33, 1: 33}
    for port, mode in enumerate(defaults, 1):
        field = fields[f"n_GPIO_val_{port:03}"]
        assert field["default_value"] == f"{mode:02}"
        allowed = {int(value) for value in field["choices"].split("|")}
        assert mode in allowed and 6 in allowed
        assert (1 in allowed) == (21 <= port <= 23)
        assert (8 in allowed) == (21 <= port <= 27)
        assert (10 in allowed) == (port >= 21)
        assert (11 in allowed) == (port >= 21)
        assert 12 not in allowed
        assert (7 in allowed) == (17 <= port <= 20)
        assert (5 in allowed) == (15 <= port <= 16)
        assert (13 in allowed) == (port <= 14)
        assert fields[f"n_s_Port_nazwa{port}_001"]["form_id"] == 0
    assert fields["n_s_Port_nazwa29_001"]["default_value"] == "Moj Przekaznik"
    assert fields["n_s_Port_nazwa33_001"]["default_value"] == "Nazwa portu nr 33     PK6"


def verify_graph_fields(pages):
    maxima = [str(1100 - i * 100) for i in range(11)] + [str(90 - i * 10) for i in range(9)]
    for route, prefix in (("/wykres1", "Wykr"), ("/wykres2", "WykrEtr")):
        fields = {field["name"]: field for field in pages[route]["fields"]}
        assert set(fields) == {f"n_s_{prefix}_{part}_{{{{N}}}}" for part in ("prgmax", "prgmin", "zaw", "sze")}
        maximum = fields[f"n_s_{prefix}_prgmax_{{{{N}}}}"]
        minimum = fields[f"n_s_{prefix}_prgmin_{{{{N}}}}"]
        assert maximum["choices"].split("|") == maxima + (["1"] if prefix == "WykrEtr" else [])
        assert minimum["choices"].split("|") == (["0"] if prefix == "WykrEtr" else ["00"] + [str(i * 10) for i in range(1, 21)])
        for part, low, high in (("zaw", 1, 500), ("sze", 1000, 30000)):
            field = fields[f"n_s_{prefix}_{part}_{{{{N}}}}"]
            assert (field["kind"], field["minimum"], field["maximum"]) == ("UINT", low, high)
        assert "Brak danych pomiarowych" in pages[route]["html"]


HARNESS = r'''
#include "http_settings_catalog.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    const HTTP_SettingPage *page;
    uint16_t instance;
    char path[128], name[128];
    size_t p, matched = 0;
    unsigned i, f, g;
    for (p = 0; p < HTTP_SettingsCatalogCount(); ++p) {
        page = HTTP_SettingsCatalogAt(p);
        assert(page && page->route && page->html);
        assert(page->field_count ? page->fields != NULL : page->fields == NULL);
        for (i = page->max_instance ? 1u : 0u; i <= page->max_instance; ++i) {
            assert(HTTP_SettingsCatalogPath(page, i, path, sizeof path));
            assert(HTTP_SettingsCatalogFind(path, &instance) == page && instance == i);
            assert(!HTTP_SettingsCatalogPath(page, i, name, 1));
            for (f = 0; f < page->field_count; ++f) {
                assert(HTTP_SettingsCatalogFieldName(&page->fields[f], i, name, sizeof name));
                assert(strlen(name) < 96 && !strstr(name, "{{"));
                assert(!HTTP_SettingsCatalogFieldName(&page->fields[f], i, path, 1));
                for (g = 0; g < f; ++g) {
                    assert(HTTP_SettingsCatalogFieldName(&page->fields[g], i, path, sizeof path));
                    assert(strcmp(path, name) != 0);
                }
                matched++;
            }
        }
        if (page->max_instance) {
            assert(!HTTP_SettingsCatalogPath(page, 0, path, sizeof path));
            assert(!HTTP_SettingsCatalogPath(page, page->max_instance + 1u, path, sizeof path));
            snprintf(path, sizeof path, "%s%03u", page->route, page->max_instance + 1u);
            assert(!HTTP_SettingsCatalogFind(path, &instance));
        } else assert(!HTTP_SettingsCatalogPath(page, 1, path, sizeof path));
    }
    assert(!HTTP_SettingsCatalogAt(HTTP_SettingsCatalogCount()));
    assert(!HTTP_SettingsCatalogFind(NULL, &instance));
    assert(!HTTP_SettingsCatalogFind("/set", NULL));
    assert(!HTTP_SettingsCatalogFieldName(NULL, 1, name, sizeof name));
    assert(!HTTP_SettingsCatalogPath(NULL, 1, name, sizeof name));
    assert(HTTP_SettingsCatalogFind("/logic", &instance) && instance == 1);
    assert(HTTP_SettingsCatalogFind("/wykres1", &instance) && instance == 1);
    assert(HTTP_SettingsCatalogFind("/wykres2", &instance) && instance == 1);
    assert(HTTP_SettingsCatalogFind("/lora", &instance) == HTTP_SettingsCatalogFind("/nrf", &instance));
    assert(HTTP_SettingsCatalogFind("/3main", &instance) == HTTP_SettingsCatalogFind("/set", &instance));
    assert(!HTTP_SettingsCatalogFind("/we000", &instance));
    assert(!HTTP_SettingsCatalogFind("/we01", &instance));
    assert(!HTTP_SettingsCatalogFind("/we0001", &instance));
    assert(!HTTP_SettingsCatalogFind("/we001/x", &instance));
    assert(!HTTP_SettingsCatalogFind("/we-01", &instance));
    assert(!HTTP_SettingsCatalogFind("/we0x1", &instance));
    assert(!HTTP_SettingsCatalogFind("/setx", &instance));
    assert(!HTTP_SettingsCatalogFind("/rot", &instance));
    printf("PASS: catalog C API, %zu expanded field identifiers, route bounds and short buffers\n", matched);
    return 0;
}
'''


def main():
    templates = json.loads(importer.FIXTURE.read_text())["templates"]
    all_pages = importer.build_pages(templates)
    pages = {page["route"]: page for page in all_pages}
    assert len(pages) == len(all_pages) == 32
    bounds = {"/we": 18, "/eol": 14, "/wypk": 13, "/tim": 13, "/term": 13, "/etr": 4, "/nap": 2, "/logic": 13,
              "/wykres1": 40, "/wykres2": 16}
    assert {route: p["max_instance"] for route, p in pages.items() if p["max_instance"]} == bounds
    source_controls = verify_source_controls(pages, templates)
    verify_procedural_fields(pages)
    verify_h753_modes(pages)
    verify_graph_fields(pages)
    actions = {"/upload", "/load", "/lcd", "/searchwifi", "/IRprga", "/IRprgb", "/rstdp", "/KRZ", "/pomiar",
               "/sesjaSMTP.txt", "/sesjaLogGSM.txt", "/lpc.cgi/testemail"}
    assert {route for route, page in pages.items() if not page["fields"]} == actions
    for page in all_pages:
        assert not re.search(r"\bv_\w+|lan_disabled|##\*\*|\*\*@|x@", page["html"]), page["route"]
        for field in page["fields"]:
            assert len(field["default_value"].encode("utf-8")) <= field["max_length"], field
            if field["kind"] == "SELECT":
                assert field["default_value"] in field["choices"].split("|"), field
            if field["kind"] == "SECRET":
                assert field["default_value"] == "", field["name"]
    subprocess.run(["python3", str(ROOT / "tools/import_sandwitch_settings.py"), "--check"], check=True)
    with tempfile.TemporaryDirectory(prefix="settings-catalog-") as directory:
        directory = Path(directory)
        (directory / "harness.c").write_text(HARNESS)
        subprocess.run(["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-fsanitize=address,undefined",
                        "-fno-omit-frame-pointer", "-no-pie", "-I" + str(ROOT / "Application"), "-I" + str(ROOT / "Application/HTTP"),
                        str(ROOT / "Application/HTTP/http_settings_catalog.c"), str(directory / "harness.c"),
                        "-o", str(directory / "harness")], check=True)
        subprocess.run([str(directory / "harness")], check=True)
    print(f"PASS: {source_controls} original static controls, procedural/graph field sets, H753 mode defaults/capabilities and 12 explicit unavailable actions")


if __name__ == "__main__":
    main()
