#!/usr/bin/env python3
"""Rebuild the STM32H7 settings catalog from checked-in Sandwitch templates.

Optional --source-header imports just the required HTML literals from the original
Sandwitch Inc/http.h (UTF-8 or Windows-1250). No runtime Const structures or device
credentials are imported. The original source is not required for normal rebuilds.
"""
from __future__ import annotations

import argparse
import hashlib
import html
import json
import re
from html.parser import HTMLParser
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / "tools/sandwitch_settings_templates.json"
OUTPUT = ROOT / "Application/http_settings_catalog.c"
NAMES = ["HttpWeCyfr", "HttpWeEOL", "HttpWyCyfr", "HttpEtr", "HttpTermost",
         "HttpTimer", "HttpUstawIP", "HttpUstawWifiAP", "HttpStrWWW", "HttpCzas",
         "HttpEkr", "HttpRejestr", "HttpUstawGSM", "HttpLora", "HttpUstawEmailNad"]


def remove_comments(source):
    # Preserve comments appearing inside C strings (URLs, HTML and JavaScript).
    pat = r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*.*?\*/|//[^\n]*'
    return re.sub(pat, lambda m: m[0] if m[0][0] in "\"'" else " " * len(m[0]), source, flags=re.S)


def decode_literal(text):
    # Historical source has whitespace after continuation backslashes. Normalize
    # this legacy spelling, retaining all actual HTML characters and line breaks.
    text = re.sub(r"\\[ \t]*\n", "", text)
    escapes = {"n": "\n", "r": "\r", "t": "\t", '"': '"', "'": "'", "\\": "\\", "0": "\0"}
    return re.sub(r"\\(?:x[0-9A-Fa-f]{2}|.)", lambda m: chr(int(m[0][2:], 16)) if m[0][1] == "x" else escapes.get(m[0][1], m[0][1]), text, flags=re.S)


def extract_header(path):
    raw = path.read_bytes()
    try:
        source = raw.decode("utf-8")
    except UnicodeDecodeError:
        source = raw.decode("cp1250")
    source = source.replace("\r\n", "\n")
    clean = remove_comments(source)
    templates = {}
    for m in re.finditer(r'static\s+const\s+char\s+(\w+)\[\]\s*=\s*"((?:\\.|[^"\\])*)"\s*;', clean, re.S):
        if m[1] in NAMES:
            templates[m[1]] = decode_literal(m[2])
    assert set(templates) == set(NAMES), set(NAMES) - set(templates)
    fixture = {"provenance": "Sandwitch Inc/http.h HTML literals only; runtime configuration excluded",
               "source_sha256": hashlib.sha256(raw).hexdigest(), "templates": templates}
    FIXTURE.write_text(json.dumps(fixture, indent=2, ensure_ascii=False) + "\n")


def options(values):
    return "".join(f'<option value="{value}">{label}</option>' for value, label in values)


def clean_template(text):
    text = text.replace("##**", "").replace("**@", "").replace("lan_disabled", "")
    text = re.sub(r"</?form\b[^>]*>", "", text, flags=re.I)
    # The common renderer owns the submit control and form scopes.
    text = re.sub(r'<button\b[^>]*type="submit"[^>]*>.*?</button>', "", text, flags=re.S | re.I)
    # These source placeholders depended on the legacy device's Const data.
    # Offer only ports with the corresponding capability on the H753 board;
    # retain all capable ports so mode changes do not erase saved associations.
    text = re.sub(r"v_selautowe\s+", options([("P00", "----")] + [(f"P{i:02}", f"port {i}") for i in range(21, 34)]), text)
    sensor_ports = list(range(1, 15)) + list(range(17, 21))
    text = re.sub(r"v_seltempwilg\s+", options([("T00", "----------")] + [(f"{kind}{i:02}", f"{label} port {i}") for i in sensor_ports for kind, label in (("T", "Temp."), ("W", "Wilgot."))]), text)
    text = re.sub(r"v_progrIRa\s*", '<a href="/IRprga" class="shiny-button">Programuj IR ON</a>', text)
    text = re.sub(r"v_progrIRb\s*", '<a href="/IRprgb" class="shiny-button">Programuj IR OFF</a>', text)
    text = re.sub(r"v_info_email_ENC\s*", "{{EMAIL_STATUS}}", text)
    text = text.replace("°C (x@)", "°C / %RH")
    # Preserve label layout but identify the page's actual numbered port.
    text = text.replace("Wejscie Cyfrowe 000", "Wejscie Cyfrowe {{INPUT_PORT}}")
    text = text.replace("Wejscie EOL 000", "Wejscie EOL {{N}}")
    text = text.replace("Licznik Impulsow 001", "Licznik Impulsow {{N}}")
    text = text.replace("Wyjscie Cyfrowe port 0  ", "Wyjscie Cyfrowe port {{OUTPUT_PORT}}")
    text = text.replace('id="n_s_Term_nr_wy_000">00', 'id="n_s_Term_nr_wy_000">{{OUTPUT_PORT}}')
    # Avoid stale source-specific DHCP JavaScript; the settings form remains editable.
    text = text.replace(' onmousedown="enableChart();"', "")
    text = text.replace('href="lpc.cgi/', 'href="/lpc.cgi/')
    return text


def suffix(text, token):
    # Only field/label identifiers are indexed; CSS lengths and numeric values stay.
    return re.sub(r'((?:name|id|for)="[^"\n]+)_000"', lambda m: m[1] + "_" + token + '"', text)


class FieldParser(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self.fields = []
        self.select = None

    def handle_starttag(self, tag, attrs):
        a = dict(attrs)
        if tag == "input" and "name" in a:
            self.fields.append((a, None))
        elif tag == "select" and "name" in a:
            self.select = (a, [])
            self.fields.append(self.select)
        elif tag == "option" and self.select:
            self.select[1].append(a)

    def handle_endtag(self, tag):
        if tag == "select":
            self.select = None


def metadata(text):
    parser = FieldParser()
    parser.feed(text)
    result = []
    seen = set()
    for a, choices in parser.fields:
        name = a["name"]
        if name in seen:
            raise ValueError("Duplicate source field " + name)
        seen.add(name)
        default = a.get("value", "").strip()
        maxlen = int(a.get("maxlength", "40"))
        field = dict(name=name, default_value=default, choices="", max_length=maxlen,
                     kind="TEXT", flags=1, form_id=0, minimum=0, maximum=2147483647)
        if choices is not None:
            values = [item.get("value", "") for item in choices]
            field.update(kind="SELECT", choices="|".join(values), flags=0,
                         default_value=next((item.get("value", "") for item in choices if "selected" in item), values[0] if values else ""), max_length=max(map(len, values), default=1))
        elif a.get("type") == "checkbox":
            field.update(kind="CHECKBOX", default_value="1" if "checked" in a else "0", max_length=1, flags=0, maximum=1)
        elif "readonly" in a or "Czas_tyg_" in name:
            field.update(kind="READONLY")
        elif any(part in name for part in ("haslo", "_has_", "GPRShas")):
            field["kind"] = "SECRET"
        elif "Lan_mac" in name:
            field.update(kind="MAC", max_length=12)
        elif any(part in name for part in ("Lan_ip", "Lan_mask", "Lan_br")):
            field["kind"] = "IPV4"
        elif "00:00" in default:
            field.update(kind="TIME" if "Tim_godz" in name or "RRapTimGodz" in name else "DURATION", flags=0)
        elif "+00.0" in default:
            field.update(kind="DECIMAL", minimum=0 if "hist" in name else -3276, maximum=3276, flags=0)
        elif name.startswith("n_s_Czas_"):
            part = name.split("_")[-2]
            limits = dict(godz=(0, 23), min=(0, 59), sec=(0, 59), rok=(0, 99), mies=(1, 12), dzien=(1, 31))
            low, high = limits.get(part, (0, 99))
            field.update(kind="UINT", default_value="01" if low else "00", flags=0, minimum=low, maximum=high)
        elif re.match(r"n_s_Wykr(?:Etr)?_zaw_", name):
            field.update(kind="UINT", minimum=1, maximum=500, flags=0)
        elif re.match(r"n_s_Wykr(?:Etr)?_sze_", name):
            field.update(kind="UINT", minimum=1000, maximum=30000, flags=0)
        elif any(part in name for part in ("_port_", "Imp_licz", "Imp_wart")):
            field.update(kind="UINT", default_value="0", maximum=65535)
        # Keep the source's visual widths but allow existing H753 configuration
        # capacities (SMTP hostnames and WPA passphrases exceed legacy lengths).
        if name == "n_s_email_ip_000":
            field["max_length"] = 95
        elif name.startswith(("n_s_Wifi_ssid_", "n_s_Wifi_haslo_")):
            field["max_length"] = 63
        result.append(field)
    return result


def value_markers(text, fields):
    names = {f["name"]: index for index, f in enumerate(fields)}
    # Transform opening tags alone so arbitrary user strings are never printf formats.
    def input_tag(m):
        tag = m[0]
        name = re.search(r'\bname="([^"]+)"', tag)
        if not name or name[1] not in names:
            return tag
        index = names[name[1]]
        field = fields[index]
        tag = re.sub(r'\bmaxlength="\d+"', f'maxlength="{field["max_length"]}"', tag)
        if field["kind"] == "CHECKBOX":
            tag = re.sub(r"\schecked(?:=\"[^\"]*\")?", "", tag)
            tag = tag.replace("/>", f" {{{{C:{index}}}}}/>") if tag.endswith("/>") else tag[:-1] + f" {{{{C:{index}}}}}>"
        else:
            if field["kind"] == "SECRET":
                tag = tag.replace('type="text"', 'type="password"')
                tag = tag[:-1] + ' autocomplete="new-password" placeholder="pozostaw puste, aby zachowac">'
            if field["kind"] == "READONLY" and "readonly" not in tag:
                tag = tag[:-1] + " readonly>"
            tag = re.sub(r'\bvalue="[^"]*"', f'value="{{{{V:{index}}}}}"', tag)
        return tag
    text = re.sub(r"<input\b[^>]*>", input_tag, text, flags=re.I)
    def select_tag(m):
        body = m[0]
        name = re.search(r'\bname="([^"]+)"', body)
        if not name or name[1] not in names:
            return body
        index = names[name[1]]
        def option_tag(o):
            tag = re.sub(r"\sselected\b", "", o[0])
            value = re.search(r'\bvalue="([^"]*)"', tag)
            return tag[:-1] + f" {{{{S:{index}:{value[1]}}}}}>" if value else tag
        return re.sub(r"<option\b[^>]*>", option_tag, body)
    return re.sub(r"<select\b.*?</select>", select_tag, text, flags=re.S | re.I)


def build_pages(templates):
    t = {key: clean_template(value) for key, value in templates.items()}
    pages = []
    def add(route, title, body, count=0):
        fields = metadata(body)
        pages.append(dict(route=route, title=title, html=body, fields=fields, max_instance=count))
    lan = suffix(t["HttpUstawIP"], "001").replace("Ustawienia sieci Wifi", "Ustawienia sieci LAN")
    add("/lan", "Ustawienia sieci LAN i WWW", lan + t["HttpStrWWW"])
    sta = suffix(t["HttpUstawWifiAP"], "001").replace("Ustawienia sieci Wifi", "Ustawienia sieci Wifi STA")
    ap = suffix(t["HttpUstawWifiAP"], "002").replace("Ustawienia sieci Wifi", "Ustawienia punktu AP")
    add("/wifi", "Ustawienia sieci WiFi", suffix(t["HttpUstawIP"], "002") + sta + ap)
    add("/czas", "Ustawienia czasu", t["HttpCzas"] + t["HttpEkr"])
    add("/gsm", "Ustawienia sieci GSM", t["HttpUstawGSM"] + "{{SMS_COMMANDS}}")
    add("/nrf", "Ustawienia sieci LoRa", t["HttpLora"])
    # These route instances index the original t_we/t_eol/t_wy arrays. They
    # differ from physical port numbers used by the main settings panel.
    add("/we", "Wejscie cyfrowe", suffix(t["HttpWeCyfr"], "{{N}}"), 18)
    add("/eol", "Wejscie EOL", suffix(t["HttpWeEOL"], "{{N}}"), 14)
    add("/wypk", "Wyjscie cyfrowe", suffix(t["HttpWyCyfr"], "{{N}}"), 13)
    add("/tim", "Timer", suffix(t["HttpTimer"] + t["HttpWyCyfr"], "{{N}}"), 13)
    add("/term", "Termo(Higro)stat", suffix(t["HttpTermost"] + t["HttpWyCyfr"], "{{N}}"), 13)
    add("/etr", "Licznik impulsow", suffix(t["HttpEtr"], "{{N}}"), 4)
    add("/rej", "Rejestrator", t["HttpRejestr"])
    for extra in json.loads((ROOT / "tools/sandwitch_settings_extra.json").read_text())["pages"]:
        page = next((page for page in pages if page["route"] == extra["route"]), None)
        if page:
            page["fields"].extend(extra["fields"])
            page["html"] += extra["html"]
        else:
            pages.append(extra)
    mail = next(page for page in pages if page["route"] == "/maskimail")
    sender = t["HttpUstawEmailNad"]
    mail["html"] += sender
    mail["fields"].extend(metadata(sender))
    pages.append(dict(route="/logic", title="Operacje logiczne", html="{{LOGIC}}", max_instance=13,
                      fields=json.loads((ROOT / "tools/sandwitch_logic_fields.json").read_text())["fields"]))
    for graph in json.loads((ROOT / "tools/sandwitch_graph_settings_templates.json").read_text())["pages"]:
        add(graph["route"], graph["title"], graph["html"], graph["max_instance"])
    # Existing H7 /set page retains its proven renderer and gains persistence.
    port_fields = []
    names = [f"Nazwa portu nr {nr}" for nr in range(1, 34)]
    for nr, label in {15: "0-10V", 16: "0-10V", 17: "E1", 18: "E2", 19: "E3", 20: "E4", 21: "PW1", 22: "PW2", 23: "PW3", 24: "OC1", 25: "OC2", 26: "OC3", 27: "OC4", 28: "PK1", 30: "PK3", 31: "PK4", 32: "PK5", 33: "PK6"}.items():
        names[nr - 1] += "     " + label
    names[28] = "Moj Przekaznik"
    # Same defaults as HTTP_SettingsPortMode in wwwPages.c (CGI HTTP_MODE_*,
    # not the older template type enum or legacy Sandwitch GPIO defaults).
    modes = [2, 2, 3, 3, 9, 9, 4, 4, 13, 13, 6, 6, 6, 2, 5, 5, 7, 7, 2, 9,
             0, 0, 10, 10, 11, 11, 8, 0, 10, 11, 6, 6, 0]
    for nr in range(1, 34):
        port_fields.append(dict(name=f"n_s_Port_nazwa{nr}_001", default_value=names[nr - 1], kind="TEXT", max_length=30, flags=1, form_id=0))
    for nr in range(1, 34):
        values = ([2, 3, 9, 4, 13, 6] if nr <= 14 else
                  [5, 6] if nr <= 16 else [2, 9, 4, 7, 6] if nr <= 20 else
                  [0, 10, 11, 1, 8, 6] if nr <= 23 else
                  [0, 10, 11, 8, 6] if nr <= 27 else [0, 10, 11, 6])
        port_fields.append(dict(name=f"n_GPIO_val_{nr:03}", default_value=f"{modes[nr - 1]:02}", choices="|".join(f"{value:02}" for value in values), kind="SELECT", max_length=2, flags=0, form_id=1))
    pages.append(dict(route="/set", title="Konfiguracja portow", html="", fields=port_fields, max_instance=0))
    actions = {
        "/upload": ("Aktualizacja oprogramowania", "Aktualizacja wymaga bootloadera STM32H7; bootloadera Sandwitch STM32F1 nie mozna uzyc dla tej plytki."),
        "/load": ("Przesylanie oprogramowania", "Przesylanie firmware jest niedostepne do czasu podlaczenia bootloadera STM32H7."),
        "/lcd": ("Ekran LCD", "Podglad i sterowanie LCD wymagaja podlaczenia obslugi wyswietlacza STM32H7."),
        "/searchwifi": ("Wyszukiwanie WiFi", "Skanowanie sieci wymaga podlaczenia polecenia ESP32 AT+CWLAP."),
        "/IRprga": ("Programowanie IR ON", "Uczenie kodu IR ON wymaga podlaczenia obslugi odbiornika IR."),
        "/IRprgb": ("Programowanie IR OFF", "Uczenie kodu IR OFF wymaga podlaczenia obslugi odbiornika IR."),
        "/rstdp": ("Reset pamieci pomiarow", "Kasowanie historii pomiarow Sandwitch nie jest podlaczone do rejestratora STM32H7."),
        "/KRZ": ("Reset pamieci pomiarow", "Kasowanie historii pomiarow Sandwitch nie jest podlaczone do rejestratora STM32H7."),
        "/pomiar": ("Historia pomiarow", "Podglad wykresow wymaga podlaczenia rejestratora pomiarow STM32H7."),
        "/sesjaSMTP.txt": ("Sesja SMTP", "Dziennik SMTP wymaga podlaczenia klienta poczty STM32H7."),
        "/sesjaLogGSM.txt": ("Sesja GSM", "Dziennik GSM wymaga podlaczenia obslugi modemu GSM."),
        "/lpc.cgi/testemail": ("Test poczty", "Wysylanie wiadomosci testowej wymaga podlaczenia klienta poczty STM32H7."),
    }
    for route, (title, message) in actions.items():
        pages.append(dict(route=route, title=title, html='<div class="corner-radius2"><div class="corner-radius"><p>' + message + '</p><p>Ta akcja nie zostala wykonana.</p></div></div>', fields=[], max_instance=0))
    for page in pages:
        # There are no index markers in procedural fragments; static suffixes use
        # full-page indices, including the sender after contact-mask fields.
        page["html"] = value_markers(page["html"], page["fields"])
        assert len({f["name"] for f in page["fields"]}) == len(page["fields"]), page["route"]
        for f in page["fields"]:
            assert len(f["name"]) < 96 and f["max_length"] <= 128, f
    return pages


def cstring(text):
    return json.dumps(text, ensure_ascii=False).replace("\\/", "/")


def generate(pages):
    out = ['/* Generated by tools/import_sandwitch_settings.py; do not edit by hand. */', '#include "http_settings_catalog.h"', '#include <stdio.h>', '#include <string.h>', '']
    for index, page in enumerate(pages):
        out.append(f"static const char page_{index}_html[] =")
        lines = page["html"].splitlines(keepends=True) or [""]
        out.extend("    " + cstring(line) for line in lines)
        out[-1] += ";"
        if not page["fields"]:
            out.append("")
            continue
        out.append(f"static const HTTP_SettingField page_{index}_fields[] = {{")
        for f in page["fields"]:
            args = [cstring(f["name"]), cstring(f.get("default_value", "")), cstring(f.get("choices", "")), str(f["max_length"]), "HTTP_SETTING_" + f["kind"], str(f.get("flags", 0)), str(f.get("form_id", 0)), str(f.get("minimum", 0)), str(f.get("maximum", 2147483647))]
            out.append("    {" + ", ".join(args) + "},")
        out.append("};\n")
    out.append("static const HTTP_SettingPage pages[] = {")
    for index, page in enumerate(pages):
        fields = f"page_{index}_fields" if page["fields"] else "NULL"
        out.append(f'    {{{cstring(page["route"])}, {cstring(page["title"])}, page_{index}_html, {fields}, {len(page["fields"])}u, {page["max_instance"]}u}},')
    out.append("};\n")
    out.append(RUNTIME)
    return "\n".join(out)


RUNTIME = r'''
size_t HTTP_SettingsCatalogCount(void)
{
    return sizeof pages / sizeof pages[0];
}

const HTTP_SettingPage *HTTP_SettingsCatalogAt(size_t index)
{
    return index < HTTP_SettingsCatalogCount() ? &pages[index] : NULL;
}

const HTTP_SettingPage *HTTP_SettingsCatalogFind(const char *path, uint16_t *instance)
{
    size_t i, path_length;
    if (!path || !instance) return NULL;
    path_length = strcspn(path, "? ");
    for (i = 0; i < HTTP_SettingsCatalogCount(); ++i) {
        const HTTP_SettingPage *page = &pages[i];
        size_t prefix = strlen(page->route);
        if (strncmp(path, page->route, prefix) != 0) continue;
        if (!page->max_instance && path_length == prefix) {
            *instance = 0;
            return page;
        }
        if (page->max_instance && path_length == prefix + 3u &&
            path[prefix] >= '0' && path[prefix] <= '9' &&
            path[prefix + 1u] >= '0' && path[prefix + 1u] <= '9' &&
            path[prefix + 2u] >= '0' && path[prefix + 2u] <= '9') {
            unsigned number = (unsigned)(path[prefix] - '0') * 100u +
                              (unsigned)(path[prefix + 1u] - '0') * 10u +
                              (unsigned)(path[prefix + 2u] - '0');
            if (number >= 1u && number <= page->max_instance) {
                *instance = (uint16_t)number;
                return page;
            }
        }
        if ((strcmp(page->route, "/logic") == 0 || strcmp(page->route, "/wykres1") == 0 ||
             strcmp(page->route, "/wykres2") == 0) && path_length == prefix) {
            *instance = 1;
            return page;
        }
    }
    /* Keep original source route /nrf and accept the more descriptive alias. */
    if (path_length == 5u && strncmp(path, "/lora", 5u) == 0)
        return HTTP_SettingsCatalogFind("/nrf", instance);
    if (path_length == 6u && strncmp(path, "/3main", 6u) == 0)
        return HTTP_SettingsCatalogFind("/set", instance);
    return NULL;
}

static int expand_name(const char *source, uint16_t instance, char *output, size_t capacity)
{
    size_t used = 0;
    if (!source || !output || !capacity) return 0;
    while (*source) {
        if (strncmp(source, "{{N}}", 5u) == 0 || strncmp(source, "{{I}}", 5u) == 0) {
            int result;
            int numbered = source[2] == 'N';
            if (!numbered && instance == 0u) return 0;
            result = snprintf(output + used, capacity - used,
                              numbered ? "%03u" : "%u",
                              numbered ? (unsigned)instance : (unsigned)(instance - 1u));
            if (result < 0 || (size_t)result >= capacity - used) return 0;
            used += (size_t)result;
            source += 5;
        } else {
            if (used + 1u >= capacity) return 0;
            output[used++] = *source++;
        }
    }
    output[used] = '\0';
    return 1;
}

int HTTP_SettingsCatalogFieldName(const HTTP_SettingField *field, uint16_t instance,
                                 char *output, size_t capacity)
{
    return field && expand_name(field->name, instance, output, capacity);
}

int HTTP_SettingsCatalogPath(const HTTP_SettingPage *page, uint16_t instance,
                            char *output, size_t capacity)
{
    int result;
    if (!page || !output || !capacity) return 0;
    if (page->max_instance) {
        if (!instance || instance > page->max_instance) return 0;
        result = snprintf(output, capacity, "%s%03u", page->route, (unsigned)instance);
    } else {
        if (instance) return 0;
        result = snprintf(output, capacity, "%s", page->route);
    }
    return result >= 0 && (size_t)result < capacity;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-header", type=Path)
    parser.add_argument("--check", action="store_true", help="Fail if generated catalog differs")
    args = parser.parse_args()
    if args.source_header:
        extract_header(args.source_header)
    pages = build_pages(json.loads(FIXTURE.read_text())["templates"])
    output = generate(pages)
    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_text() != output:
            raise SystemExit("Generated settings catalog is stale")
    else:
        OUTPUT.write_text(output)
    print(f"Settings catalog: {len(pages)} route families, {sum(len(p['fields']) for p in pages)} template fields")


if __name__ == "__main__":
    main()
