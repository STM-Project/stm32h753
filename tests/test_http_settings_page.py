#!/usr/bin/env python3
"""Exercise the imported Sandwitch pages as browser forms and real GET requests.

Run: python3 tests/test_http_settings_page.py
The host harness uses the production catalog/model/renderer, real shared CSS,
and an in-memory durable-save callback. No board, account or secrets are needed.
"""
import ctypes as C
from html.parser import HTMLParser
import json
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
from urllib.parse import urlencode

from test_settings_templates import literal


ROOT = Path(__file__).resolve().parents[1]
CAPACITY = 128 * 1024
CHECKBOX, SECRET = 10, 8


class Field(C.Structure):
    _fields_ = [("name", C.c_char_p), ("default_value", C.c_char_p),
                ("choices", C.c_char_p), ("max_length", C.c_uint16),
                ("kind", C.c_uint8), ("flags", C.c_uint8),
                ("form_id", C.c_uint8), ("minimum", C.c_int32),
                ("maximum", C.c_int32)]


class Page(C.Structure):
    _fields_ = [("route", C.c_char_p), ("title", C.c_char_p),
                ("html", C.c_char_p), ("fields", C.POINTER(Field)),
                ("field_count", C.c_uint16), ("max_instance", C.c_uint16)]


class Form(HTMLParser):
    """Apply successful-control rules used by a normal browser GET form."""
    def __init__(self):
        super().__init__()
        self.forms = []
        self.depth = 0
        self.fields = {}
        self.controls = []
        self.submit = []
        self.select = None
        self.options = []
        self.links = []

    def handle_starttag(self, tag, attributes):
        a = dict(attributes)
        if tag == "option" and self.select is not None:
            self.options.append((a.get("value", ""), "selected" in a))
        if tag == "form":
            assert self.depth == 0, "Nested form"
            self.depth += 1
            self.forms.append(a)
        if tag == "a":
            self.links.append(a.get("href"))
        if tag not in ("input", "select", "button") or "name" not in a:
            return
        assert self.depth == 1, (tag, a["name"], "outside form")
        if tag == "button":
            if a.get("type", "submit") == "submit":
                self.submit.append((a["name"], a.get("value", "")))
            return
        assert a["name"] not in self.fields, ("Duplicate field", a["name"])
        self.fields[a["name"]] = a
        if tag == "select":
            self.select = a["name"]
            self.options = []
        elif "disabled" not in a:
            if a.get("type") not in ("checkbox", "radio") or "checked" in a:
                self.controls.append((a["name"], a.get("value", "on")))

    def handle_startendtag(self, tag, attrs):
        self.handle_starttag(tag, attrs)

    def handle_endtag(self, tag):
        if tag == "form":
            assert self.depth == 1, "Unmatched closing form"
            self.depth = 0
        if tag == "select" and self.select is not None:
            selected = [value for value, chosen in self.options if chosen]
            assert len(selected) == 1, (self.select, "select must choose exactly one option")
            self.controls.append((self.select, selected[0]))
            self.fields[self.select]["options"] = [value for value, _ in self.options]
            self.select = None

    def feed(self, data):
        # HTMLParser dispatches option handling through the same starttag method.
        super().feed(data)
        assert self.depth == 0, "Unclosed form"

SANITIZER_HARNESS = r'''
#include "http_settings_page.h"
#include "http_settings_model.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int save(const void *blob, size_t length, void *context)
{
    (void)context;
    assert(HTTP_SettingsModelValidateBlob(blob, length));
    return 1;
}

int main(int argc, char **argv)
{
    char *body = malloc(131074), *guard = malloc(131074), *target = malloc(65538);
    const HTTP_SettingPage *page;
    uint16_t instance;
    size_t longest = 0, tested = 0;
    FILE *requests;
    assert(body && guard && target && argc == 2);
    assert(HTTP_SettingsModelInit(NULL, save, NULL) == 0);
    for (size_t p = 0; p < HTTP_SettingsCatalogCount(); ++p) {
        page = HTTP_SettingsCatalogAt(p);
        if (!strcmp(page->route, "/set")) {
            assert(HTTP_SettingsPageRender(body, 131072, page, 0, NULL) == 0);
            continue;
        }
        instance = page->max_instance ? page->max_instance : 0;
        size_t length = HTTP_SettingsPageRender(body, 131072, page, instance, "<&>\"'");
        assert(length && length == strlen(body));
        if (length > longest) longest = length;
        for (size_t capacity = 1; capacity <= length;
             capacity += capacity < 128 ? 1 : 4093) {
            memset(guard, 0x55, 131074);
            assert(HTTP_SettingsPageRender(guard, capacity, page, instance, "<&>\"'") == 0);
            assert(guard[0] == 0 && (unsigned char)guard[capacity] == 0x55);
        }
        memset(guard, 0x55, 131074);
        assert(HTTP_SettingsPageRender(guard, length, page, instance, "<&>\"'") == 0);
        assert(guard[0] == 0 && (unsigned char)guard[length] == 0x55);
        assert(HTTP_SettingsPageRender(guard, length + 1, page, instance, "<&>\"'") == length);
        assert(!strcmp(body, guard) && (unsigned char)guard[length + 1] == 0x55);
        assert(HTTP_SettingsPageRender(body, 131072, page, page->max_instance + 1, NULL) == 0);
    }
    assert(HTTP_SettingsPageRender(NULL, 131072, HTTP_SettingsCatalogAt(0), 0, NULL) == 0);
    assert(HTTP_SettingsPageRender(body, 131072, NULL, 0, NULL) == 0 && body[0] == 0);
    body[0] = 'x';
    assert(HTTP_SettingsPageRender(body, 0, HTTP_SettingsCatalogAt(0), 0, NULL) == 0 && body[0] == 'x');
    requests = fopen(argv[1], "rb");
    assert(requests);
    while (fgets(target, 65538, requests)) {
        size_t length = strcspn(target, "\r\n");
        target[length] = 0;
        assert(HTTP_SettingsModelInit(NULL, save, NULL) == 0);
        int result = HTTP_SettingsApplyTarget(target, length, &page, &instance);
        if (result != HTTP_SETTINGS_SAVED)
            fprintf(stderr, "GET roundtrip failed: %s (%d)\n", page ? page->route : "unknown", result);
        assert(result == HTTP_SETTINGS_SAVED);
        assert(HTTP_SettingsPageRender(body, 131072, page, instance, NULL) > 0);
        ++tested;
    }
    assert(fclose(requests) == 0);
    printf("PASS: ASan/UBSan boundaries on every page; %zu browser GET roundtrips; largest page %zu bytes.\n", tested, longest);
    free(target); free(guard); free(body);
    return 0;
}
'''


def configure(library):
    library.HTTP_SettingsCatalogCount.restype = C.c_size_t
    library.HTTP_SettingsCatalogAt.argtypes = [C.c_size_t]
    library.HTTP_SettingsCatalogAt.restype = C.POINTER(Page)
    library.HTTP_SettingsCatalogFieldName.argtypes = [C.POINTER(Field), C.c_uint16, C.c_char_p, C.c_size_t]
    library.HTTP_SettingsCatalogPath.argtypes = [C.POINTER(Page), C.c_uint16, C.c_char_p, C.c_size_t]
    library.HTTP_SettingsPageRender.argtypes = [C.c_char_p, C.c_size_t, C.POINTER(Page), C.c_uint16, C.c_char_p]
    library.HTTP_SettingsPageRender.restype = C.c_size_t
    library.HTTP_SettingsApplyTarget.argtypes = [C.c_char_p, C.c_size_t, C.POINTER(C.POINTER(Page)), C.POINTER(C.c_uint16)]
    library.HTTP_SettingsGetByName.argtypes = [C.POINTER(Page), C.c_uint16, C.c_char_p]
    library.HTTP_SettingsGetByName.restype = C.c_char_p
    library.HTTP_SettingsModelInit.argtypes = [C.c_void_p, C.c_void_p, C.c_void_p]
    library.HTTP_SettingsModelValidateBlob.argtypes = [C.c_void_p, C.c_size_t]


def render(library, page, instance, notice=None):
    body = C.create_string_buffer(CAPACITY)
    length = library.HTTP_SettingsPageRender(body, CAPACITY, page, instance, notice)
    assert length > 0 and length == len(body.value), (page.contents.route, instance, "render failed")
    return body.value.decode()


def main():
    source = (ROOT / "Application/wwwPages.c").read_text()
    style, menu = literal(source, "HttpStyle"), literal(source, "HttpMainMenu")
    with tempfile.TemporaryDirectory(prefix="h7-settings-page-") as directory:
        tmp = Path(directory)
        common = tmp / "common.c"
        common.write_text("const char HttpStyle[]=" + json.dumps(style) + ";\n" +
                          "const char HttpMainMenu[]=" + json.dumps(menu) + ";\n" +
                          "unsigned HTTP_SettingsPortMode(unsigned port) { return port % 14; }\n")
        sources = [str(ROOT / "Application" / file) for file in
                   ("http_settings_catalog.c", "http_settings_model.c", "http_settings_page.c")]
        flags = ["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-I" + str(ROOT / "Application")]
        binary = tmp / "settings.so"
        subprocess.run(flags + ["-shared", "-fPIC", str(common)] + sources + ["-o", str(binary)], check=True)
        library = C.CDLL(str(binary))
        configure(library)

        @C.CFUNCTYPE(C.c_int, C.c_void_p, C.c_size_t, C.c_void_p)
        def save(blob, length, context):
            assert library.HTTP_SettingsModelValidateBlob(blob, length) == 1
            return 1

        def reset():
            assert library.HTTP_SettingsModelInit(None, C.cast(save, C.c_void_p), None) == 0

        reset()
        requests, scripts, pages, indexed = [], set(), {}, 0
        for index in range(library.HTTP_SettingsCatalogCount()):
            page = library.HTTP_SettingsCatalogAt(index)
            schema = page.contents
            route = schema.route.decode()
            if route == "/set":
                continue  # Existing per-port HTTP_TEMPLATE_2 renderer owns this page.
            pages[route] = page
            for instance in range(1, schema.max_instance + 1) if schema.max_instance else [0]:
                indexed += 1
                reset()
                html = render(library, page, instance)
                assert html.startswith(style + menu)
                assert html.count("<body ") == html.count("</body>") == html.count("</html>") == 1
                assert not re.search(r"\{\{[A-Z][A-Z_0-9:]*\}\}", html), route
                assert not re.search(r"\bv_(?:sel|progr|info|[A-Za-z]+%v)", html), route
                form = Form()
                form.feed(html)
                scripts.update(re.findall(r"<script[^>]*>(.*?)</script>", html, re.S))
                if not schema.field_count:
                    assert not form.forms and not form.submit and not form.fields, route
                    continue
                assert form.forms == [{"method": "get", "action": "/lpc.cgi/A117", "data-ajax": "false"}], route
                assert len(form.submit) == 1, (route, form.submit)
                fields = {}
                for field_index in range(schema.field_count):
                    field = schema.fields[field_index]
                    expanded = C.create_string_buffer(96)
                    assert library.HTTP_SettingsCatalogFieldName(C.byref(field), instance, expanded, 96)
                    name = expanded.value.decode()
                    assert field.form_id == 0, (route, name)
                    fields[name] = field
                assert set(form.fields) == set(fields) | {"h7_page", "h7_form"}, (route, set(fields) - set(form.fields), set(form.fields) - set(fields))
                values = dict(form.controls)
                assert values["h7_form"] == "0"
                for name, field in fields.items():
                    attrs = form.fields[name]
                    if field.kind == 7:
                        choices = field.choices.decode().split("|")
                        if "options" in attrs:
                            assert attrs["options"] == choices, (route, name, attrs)
                        else:
                            # Sandwitch translation signs are one-character text
                            # controls, validated against the two allowed signs.
                            assert route == "/nap" and attrs.get("value") in choices
                    if field.kind == SECRET:
                        assert attrs["type"] == "password" and attrs["value"] == "", (route, name)
                        assert "pozostaw puste" in attrs["placeholder"]
                target = "/lpc.cgi/A117?" + urlencode(form.controls + form.submit)
                assert len(target) <= 65535
                requests.append(target)
                result = library.HTTP_SettingsApplyTarget(target.encode(), len(target), None, None)
                assert result == 1, (route, instance, "browser default GET was rejected", result)
                if route == "/logic":
                    assert len(fields) == 92
                    assert values["n_s_SigWy"] == str(instance - 1)
                    assert html.count('id="myCanvas"') == 1

        # A real form save must safely escape text/notice, and never reflect a secret.
        reset()
        page = pages["/lan"]
        form = Form()
        form.feed(render(library, page, 0))
        replacement = {"n_s_log_000": 'A<&\"\'%', "n_s_has_000": "test-secret-only"}
        pairs = [(name, replacement.get(name, value)) for name, value in form.controls]
        target = "/lpc.cgi/A117?" + urlencode(pairs + form.submit)
        assert library.HTTP_SettingsApplyTarget(target.encode(), len(target), None, None) == 1
        html = render(library, page, 0, b"Saved <&>\"'")
        assert "A&lt;&amp;&quot;&#39;%" in html
        assert "Saved &lt;&amp;&gt;&quot;&#39;" in html
        assert "test-secret-only" not in html
        assert library.HTTP_SettingsGetByName(page, 0, b"n_s_has_000") == b"test-secret-only"
        blank = Form()
        blank.feed(html)
        target = "/lpc.cgi/A117?" + urlencode(blank.controls + blank.submit)
        assert library.HTTP_SettingsApplyTarget(target.encode(), len(target), None, None) == 1
        assert library.HTTP_SettingsGetByName(page, 0, b"n_s_has_000") == b"test-secret-only"

        if shutil.which("node"):
            for index, script in enumerate(scripts):
                script_file = tmp / f"script-{index}.js"
                script_file.write_text(script)
                subprocess.run(["node", "--check", str(script_file)], check=True)
            logic = next(script for script in scripts if "data-logic-bank" in script)
            script_file = tmp / "logic-behavior.js"
            script_file.write_text("const assert=require('node:assert/strict');let changed,route;"
                "const output={value:'12',addEventListener:(name,fn)=>{assert.equal(name,'change');changed=fn;}};"
                "const document={getElementById:()=>null,querySelector:()=>output};"
                "const window={location:{assign:value=>{route=value;}}};" + logic +
                "assert.equal(typeof changed,'function');changed();assert.equal(route,'/logic013');"
                "route=null;output.value='13';changed();assert.equal(route,null);"
                "output.value='0';changed();assert.equal(route,'/logic001');")
            subprocess.run(["node", str(script_file)], check=True)

        request_file = tmp / "requests.txt"
        request_file.write_text("\n".join(requests) + "\n")
        harness = tmp / "bounds.c"
        harness.write_text(SANITIZER_HARNESS)
        executable = tmp / "bounds"
        subprocess.run(flags + ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-no-pie", str(common), str(harness)] + sources + ["-o", str(executable)], check=True)
        subprocess.run([str(executable), str(request_file)], check=True)
        arm = Path("/workspace/.stm32-tools/root/usr/bin/arm-none-eabi-gcc")
        if arm.exists():
            subprocess.run([str(arm), "-std=c11", "-Wall", "-Wextra", "-Werror", "-mcpu=cortex-m7", "-mthumb",
                            "-mfpu=fpv5-d16", "-mfloat-abi=hard", "-isystem", "/workspace/.stm32-tools/root/usr/include/newlib",
                            "-I" + str(ROOT / "Application"), "-c", str(ROOT / "Application/http_settings_page.c"),
                            "-o", str(tmp / "page.o")], check=True)
        print(f"PASS: {indexed} page instances; schema-complete browser forms, persisted defaults, escaped text, hidden secrets, logic banks and JavaScript.")


if __name__ == "__main__":
    main()
