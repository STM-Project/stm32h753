#!/usr/bin/env python3
"""Real ESP routes + catalog/model/page integration, without MCU headers.

Includes the sensor routing regression suite, then exercises every imported
settings route, isolated queued replies and browser-sized CGI requests split
over multiple +IPD frames. Persistence uses durable-memory callbacks; hardware
runtime bindings have their own test and are a counted stub in this harness.
Run: python3 tests/test_http_settings_routes.py
"""
import test_http_sensor_routes as sensor_routes


TESTS = r'''
static unsigned char generic_persisted[HTTP_SETTINGS_BLOB_SIZE];
static unsigned generic_saves;
static int generic_saved, generic_failure;
static char generic_target[HTTP_SETTINGS_TARGET_MAX + 1u];
static const char *main_mode_override[34];

static int generic_load(void *out, size_t size, void *context) {
    assert(context == generic_persisted); assert(size == sizeof generic_persisted);
    if (!generic_saved) return 0;
    memcpy(out, generic_persisted, size); return 1;
}
static int generic_save(const void *data, size_t size, void *context) {
    assert(context == generic_persisted); assert(size == sizeof generic_persisted);
    ++generic_saves;
    if (generic_failure) return -1;
    assert(HTTP_SettingsModelValidateBlob(data, size));
    memcpy(generic_persisted, data, size); generic_saved = 1; return 1;
}
static void encode_append(const char *value) {
    static const char hex[] = "0123456789ABCDEF";
    size_t used = strlen(generic_target);
    while (*value) {
        unsigned char c = (unsigned char)*value++;
        assert(used + 4u < sizeof generic_target);
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.') {
            generic_target[used++] = (char)c;
        } else {
            generic_target[used++] = '%';
            generic_target[used++] = hex[c >> 4];
            generic_target[used++] = hex[c & 15u];
        }
    }
    generic_target[used] = 0;
}
/* A browser submits every editable successful control from the selected form.
 * Preserve the source catalog values, except a few deliberately changed inputs.
 */
static const HTTP_SettingPage *generic_form(const char *route, unsigned form_id,
                                          int checked, int temperature_only) {
    char name[HTTP_SETTINGS_FIELD_NAME_MAX + 1u];
    uint16_t instance;
    const HTTP_SettingPage *page = HTTP_SettingsCatalogFind(route, &instance);
    assert(page);
    strcpy(generic_target, "/lpc.cgi/A117?h7_page="); encode_append(route);
    strcat(generic_target, form_id ? "&h7_form=1" : "&h7_form=0");
    for (unsigned f = 0; f < page->field_count; ++f) {
        const HTTP_SettingField *field = &page->fields[f];
        const char *value = field->default_value;
        if (field->form_id != form_id || field->kind == HTTP_SETTING_READONLY) continue;
        if (field->kind == HTTP_SETTING_CHECKBOX) {
            if (!checked) continue;
            value = "1";
        }
        assert(HTTP_SettingsCatalogFieldName(field, instance, name, sizeof name));
        if (!strcmp(name, "n_s_Port_nazwa5_001")) value = "Kuchnia & <ok> Łódź";
        if (!strcmp(name, "n_GPIO_val_005")) value = temperature_only ? "02" : "09";
        if (!strcmp(name, "n_s_ttd0_099")) value = "1234";
        if (!strcmp(name, "n_s_ttm0_099")) value = "56";
        if (!strcmp(name, "n_s_nrtel1_000")) value = "+48123456789";
        if (!strcmp(name, "n_s_email_odb1_000")) value = "test@example.invalid";
        if (!strncmp(name, "n_GPIO_val_", 11u)) {
            unsigned port = (unsigned)atoi(name + 11u);
            assert(port >= 1u && port <= 33u);
            if (main_mode_override[port]) value = main_mode_override[port];
        }
        strcat(generic_target, "&"); encode_append(name);
        strcat(generic_target, "="); encode_append(value);
    }
    strcat(generic_target, "&n_109_zapisz=");
    return page;
}
static void feed_large_request(HTTP_RequestStream *stream, unsigned channel) {
    static char request_line[HTTP_SETTINGS_TARGET_MAX + 128u];
    char prefix[64];
    int length = snprintf(request_line, sizeof request_line,
        "GET %s HTTP/1.1\r\nHost: stm32\r\n\r\n", generic_target);
    assert(length > 0 && (size_t)length < sizeof request_line);
    clear_queue(); HTTP_RequestStreamInit(stream, HTTP_HandleTarget, NULL);
    for (size_t used = 0; used < (size_t)length;) {
        size_t frame_length = (size_t)length - used;
        if (frame_length > 1024u) frame_length = 1024u;
        int prefix_length = snprintf(prefix, sizeof prefix, "+IPD,%u,%zu:", channel, frame_length);
        assert(prefix_length > 0 && (size_t)prefix_length < sizeof prefix);
        for (int i = 0; i < prefix_length; ++i)
            HTTP_RequestStreamFeed(stream, (const uint8_t *)prefix + i, 1);
        for (size_t i = 0; i < frame_length;) {
            size_t chunk = frame_length - i;
            if (chunk > 17u) chunk = 17u;
            HTTP_RequestStreamFeed(stream, (const uint8_t *)request_line + used + i, chunk);
            i += chunk;
        }
        used += frame_length;
    }
}
static void check_masks(const HTTP_SettingPage *page, int checked) {
    unsigned boxes = 0;
    for (unsigned f = 0; f < page->field_count; ++f) {
        const HTTP_SettingField *field = &page->fields[f];
        if (field->kind != HTTP_SETTING_CHECKBOX) continue;
        assert(!strcmp(HTTP_SettingsGet(page, 0, field), checked ? "1" : "0"));
        ++boxes;
    }
    assert(boxes >= 160u);
}

/* Follow the rendered DOM rather than matching a particular tile implementation:
 * refresh targets must be unique, and a disabled mode may keep hidden targets
 * so the original one-second refresh script can still update every port. */
static int main_id_visible(const char *html, const char *id) {
    unsigned count = 0, depth = 0;
    int visibility = 0, hidden[128] = {0};
    char quoted[128], single[128];
    snprintf(quoted, sizeof quoted, "id=\"%s\"", id);
    snprintf(single, sizeof single, "id='%s'", id);
    for (const char *p = html; (p = strchr(p, '<')) != NULL;) {
        const char *end = strchr(p, '>');
        assert(end);
        if (!strncmp(p, "<script", 7u)) {
            p = strstr(end, "</script>"); assert(p); p += 9u; continue;
        }
        if (!strncmp(p, "<style", 6u)) {
            p = strstr(end, "</style>"); assert(p); p += 8u; continue;
        }
        if (!strncmp(p, "</div", 5u)) {
            assert(depth > 0u); --depth;
        } else {
            char tag[2048];
            assert((size_t)(end - p) < sizeof tag);
            memcpy(tag, p, (size_t)(end - p)); tag[end - p] = 0;
            const int is_div = !strncmp(p, "<div", 4u) &&
                (p[4] == ' ' || p[4] == '\t' || p[4] == '\r' || p[4] == '\n' || p[4] == '>');
            int own_hidden = 0;
            const char *style = strstr(tag, "display:");
            if (style) {
                style += 8u;
                while (*style == ' ') ++style;
                own_hidden = !strncmp(style, "none", 4u);
            }
            int tag_hidden = hidden[depth] || own_hidden;
            const char *found = strstr(tag, quoted);
            if (!found) found = strstr(tag, single);
            if (found) { ++count; visibility = !tag_hidden; }
            if (is_div) {
                assert(depth + 1u < sizeof hidden / sizeof hidden[0]);
                hidden[++depth] = tag_hidden;
            }
        }
        p = end + 1u;
    }
    if (count != 1u) fprintf(stderr, "Main DOM target %s occurs %u times\n", id, count);
    assert(count == 1u);
    return visibility;
}
static void check_main_refresh_dom(const char *html) {
    /* These IDs are taken from the real retained refresh JavaScript. Checking
     * every lookup catches missing/duplicate inactive targets after mode changes. */
    static const char lookup[] = "document.getElementById(\"";
    const char *p = HttpStructMainReadPanel;
    unsigned lookups = 0;
    while ((p = strstr(p, lookup)) != NULL) {
        char id[64]; p += sizeof lookup - 1u;
        const char *end = strchr(p, '"');
        assert(end && (size_t)(end - p) < sizeof id);
        memcpy(id, p, (size_t)(end - p)); id[end - p] = 0;
        (void)main_id_visible(html, id); ++lookups; p = end + 1u;
    }
    assert(lookups > 150u);
    assert(strstr(html, "setInterval(loadXMLDoc, 1000"));
}
static void check_main_route(unsigned port, const char *id, int visible) {
    static char html[250000], alias[250000];
    request(0, "/main"); check_response(0, 200, "text/html; charset=UTF-8");
    body(0, html, sizeof html);
    assert(main_id_visible(html, id) == visible);
    check_main_refresh_dom(html);
    if (port == 5u) {
        assert((strstr(html, "Kuchnia &amp; &lt;ok&gt; Łódź") != NULL) == visible);
        assert(!strstr(html, "Kuchnia & <ok> Łódź"));
    }
    request(0, "/"); check_response(0, 200, "text/html; charset=UTF-8");
    body(0, alias, sizeof alias); assert(!strcmp(html, alias));
}
static void check_main_settings_integration(void) {
    static const struct { unsigned port; const char *mode, *id; int visible; } cases[] = {
        {5, "02", "tu_temp005a", 1}, {5, "09", "tu_temp005b", 1},
        {5, "04", "tu_we005", 1}, {5, "13", "tu_eol005", 1},
        {5, "03", "tu_ptt005", 1}, {5, "06", "tu_temp005a", 0},
        {15, "05", "tu_adc015", 1}, {15, "06", "tu_adc015", 0},
        {17, "07", "tu_imp0_017", 1}, {17, "02", "tu_temp017a", 1},
        {17, "09", "tu_temp017b", 1}, {17, "04", "tu_we017", 1},
        {17, "06", "tu_imp0_017", 0},
        {21, "00", "tu_pk021", 1}, {21, "01", "volume021", 1},
        {21, "08", "tu_pk021", 1}, {21, "10", "tu_ti021", 1},
        {21, "11", "tu_te021", 1}, {21, "06", "tu_pk021", 0}
    };
    static char html[250000], queued[250000];
    /* The complete panel must respect its caller's capacity, including the
     * terminator, even though individual legacy templates use scratch space. */
    size_t length = HTTP_MainReadPanelRender(html, sizeof html);
    assert(length > 4096u && length == strlen(html));
    assert(HTTP_MainReadPanelRender(NULL, sizeof html) == 0);
    const size_t capacities[] = {0, 1, 127, 4096, length, length + 1u};
    for (unsigned i = 0; i < sizeof capacities / sizeof capacities[0]; ++i) {
        size_t capacity = capacities[i];
        memset(queued, 0x55, sizeof queued);
        size_t result = HTTP_MainReadPanelRender(queued + 1, capacity);
        assert((unsigned char)queued[0] == 0x55);
        assert((unsigned char)queued[capacity + 1u] == 0x55);
        if (capacity > length) {
            assert(result == length && !strcmp(queued + 1, html));
        } else {
            assert(result == 0);
            assert(capacity == 0 || queued[1] == 0);
        }
    }
    check_main_route(5, "tu_temp005b", 1);
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        unsigned port = cases[i].port;
        memset(main_mode_override, 0, sizeof main_mode_override);
        main_mode_override[port] = cases[i].mode;
        generic_form("/set", 1, 0, 0);
        unsigned before = generic_saves;
        request(0, generic_target); check_response(0, 200, "text/html; charset=UTF-8");
        assert(generic_saves == before + 1u);
        assert(HTTP_SettingsPortMode(port) == (unsigned)atoi(cases[i].mode));
        check_main_route(port, cases[i].id, cases[i].visible);
        if (port == 5u && !strcmp(cases[i].mode, "02")) {
            body(0, html, sizeof html);
            assert(!main_id_visible(html, "tu_temp005b"));
            assert(!main_id_visible(html, "tu_temp005c"));
        }
        if (port == 5u && !strcmp(cases[i].mode, "09")) {
            body(0, html, sizeof html);
            assert(!main_id_visible(html, "tu_temp005a"));
            assert(main_id_visible(html, "tu_temp005c"));
        }
    }
    memset(main_mode_override, 0, sizeof main_mode_override);
    generic_form("/set", 1, 0, 0); request(0, generic_target);
    assert(HTTP_SettingsModelInit(generic_load, generic_save, generic_persisted) == 1);
    check_main_route(5, "tu_temp005b", 1);

    /* /main must not reuse another queue slot's generated bytes. A settings
     * save on a second channel changes only subsequent main responses. */
    clear_queue(); HTTP_HandleTarget(4, "/main", 5u, NULL);
    body(0, queued, sizeof queued);
    main_mode_override[5] = "04"; generic_form("/set", 1, 0, 0);
    HTTP_HandleTarget(2, generic_target, strlen(generic_target), NULL);
    check_response(2, 200, "text/html; charset=UTF-8");
    assert(slot_for(2) == 1); body(0, html, sizeof html); assert(!strcmp(html, queued));
    HTTP_HandleTarget(1, "/main", 5u, NULL);
    assert(slot_for(1) == 2); body(2, html, sizeof html);
    assert(main_id_visible(html, "tu_we005"));
    assert(!main_id_visible(html, "tu_temp005b"));
    body(0, html, sizeof html); assert(!strcmp(html, queued));
    main_mode_override[5] = NULL;
    generic_form("/set", 1, 0, 0); request(0, generic_target);
    check_main_route(5, "tu_temp005b", 1);
}
int main(void) {
    static char html[250000], snapshot[ESP_MAX_HTTP_CONN][250000];
    static HTTP_RequestStream stream;
    assert(sensor_routes_suite() == 0);
    assert(HTTP_SettingsModelInit(generic_load, generic_save, generic_persisted) == 0);

    /* Every generated route and every numbered instance reaches the real renderer. */
    unsigned page_responses = 0, action_responses = 0;
    for (size_t p = 0; p < HTTP_SettingsCatalogCount(); ++p) {
        const HTTP_SettingPage *page = HTTP_SettingsCatalogAt(p);
        unsigned first = page->max_instance ? 1u : 0u;
        unsigned last = page->max_instance ? page->max_instance : 0u;
        for (unsigned instance = first; instance <= last; ++instance) {
            char path[64];
            assert(HTTP_SettingsCatalogPath(page, (uint16_t)instance, path, sizeof path));
            request(0, path);
            check_response(0, page->field_count ? 200u : 501u, "text/html; charset=UTF-8");
            body(0, html, sizeof html);
            assert(strstr(html, "<html>")); assert(strstr(html, "</html>"));
            if (!page->field_count) {
                assert(strstr(html, "Ta akcja nie zostala wykonana."));
                ++action_responses;
            } else {
                assert(strstr(html, "h7_page")); ++page_responses;
            }
        }
    }
    assert(page_responses >= 100u); assert(action_responses >= 8u);
    assert(generic_saves == 0); assert(bindings_count == 0);
    request(0, "/lora"); check_response(0, 200, "text/html; charset=UTF-8");
    request(0, "/logic"); check_response(0, 200, "text/html; charset=UTF-8");
    const char *invalid[] = {"/we000", "/we019", "/wypk014", "/eol015", "/nap003",
        "/etr005", "/term014", "/tim000", "/logic014", "/wifiX", "/lan?x=1",
        "/lpc.cgi/A117?h7_page=%2Funknown&h7_form=0&n_109_zapisz="};
    for (unsigned i = 0; i < sizeof invalid / sizeof invalid[0]; ++i) {
        request(0, invalid[i]);
        check_response(0, i + 1u == sizeof invalid / sizeof invalid[0] ? 400 : 404,
                       "text/html; charset=UTF-8");
    }

    /* All five slots retain their own bytes, irrespective of channel order. */
    const char *queued[] = {"/lan", "/wifi", "/tim013", "/nap002", "/set"};
    clear_queue();
    for (unsigned i = 0; i < ESP_MAX_HTTP_CONN; ++i) {
        unsigned channel = ESP_MAX_HTTP_CONN - i - 1u;
        HTTP_HandleTarget(channel, queued[i], strlen(queued[i]), NULL);
        assert(slot_for(channel) == (int)i); body(i, snapshot[i], sizeof snapshot[i]);
    }
    for (unsigned i = 0; i < ESP_MAX_HTTP_CONN; ++i) {
        body(i, html, sizeof html); assert(!strcmp(html, snapshot[i]));
        check_response(ESP_MAX_HTTP_CONN - i - 1u, 200, "text/html; charset=UTF-8");
        for (unsigned j = 0; j < i; ++j) assert(strcmp(snapshot[i], snapshot[j]));
    }
    generic_form("/set", 0, 0, 0);
    HTTP_HandleTarget(0, generic_target, strlen(generic_target), NULL);
    assert(generic_saves == 0);

    /* Names and modes use distinct forms and both survive model reload. */
    request(0, generic_target); check_response(0, 200, "text/html; charset=UTF-8");
    assert(generic_saves == 1 && bindings_count == 1);
    body(0, html, sizeof html);
    assert(strstr(html, "Kuchnia &amp; &lt;ok&gt; Łódź"));
    assert(strstr(html, "window.history.replaceState(null,'','/set')"));
    assert(!strcmp(HTTP_SettingsPortName(5, "fallback"), "Kuchnia & <ok> Łódź"));
    generic_form("/set", 1, 0, 1); request(0, generic_target);
    check_response(0, 200, "text/html; charset=UTF-8");
    assert(generic_saves == 2 && bindings_count == 2);
    assert(HTTP_SettingsPortMode(5) == HTTP_MODE_TEMP);
    assert(HTTP_SettingsModelInit(generic_load, generic_save, generic_persisted) == 1);
    assert(HTTP_SettingsPortMode(5) == HTTP_MODE_TEMP);
    assert(!strcmp(HTTP_SettingsPortName(5, "fallback"), "Kuchnia & <ok> Łódź"));
    request(0, "/temp005"); body(0, html, sizeof html);
    assert(strstr(html, "Temperatura Port 5")); assert(!strstr(html, "Wilgotnosc Port 5"));
    generic_form("/set", 1, 0, 0); request(0, generic_target);
    check_response(0, 200, "text/html; charset=UTF-8");
    assert(HTTP_SettingsPortMode(5) == HTTP_MODE_TEMP_RHU);
    request(0, "/temp005"); body(0, html, sizeof html);
    assert(strstr(html, "Wilgotnosc Port 5"));
    check_main_settings_integration();

    /* Real browser ADC tables and complete masks exceed the old target limit. */
    const HTTP_SettingPage *adc = generic_form("/nap001", 0, 0, 0);
    assert(adc->field_count == 301u); assert(strlen(generic_target) > 3072u);
    unsigned before = generic_saves;
    feed_large_request(&stream, 2); check_response(2, 200, "text/html; charset=UTF-8");
    assert(generic_saves == before + 1u);
    assert(!strcmp(HTTP_SettingsGetByName(adc, 1, "n_s_ttd0_099"), "1234"));
    assert(!strcmp(HTTP_SettingsGetByName(adc, 1, "n_s_ttm0_099"), "56"));
    body(0, html, sizeof html); assert(strstr(html, "value=\"1234\""));
    assert(strstr(html, "window.history.replaceState(null,'','/nap001')"));
    assert(strstr(html, "value=\"56\""));
    const HTTP_SettingPage *sms = generic_form("/maskisms", 0, 1, 0);
    assert(strlen(generic_target) > 3072u);
    feed_large_request(&stream, 3); check_response(3, 200, "text/html; charset=UTF-8");
    check_masks(sms, 1);
    const HTTP_SettingPage *mail = generic_form("/maskimail", 0, 1, 0);
    assert(strlen(generic_target) > 3072u);
    feed_large_request(&stream, 4); check_response(4, 200, "text/html; charset=UTF-8");
    check_masks(mail, 1);
    generic_form("/maskisms", 0, 0, 0); request(0, generic_target);
    check_response(0, 200, "text/html; charset=UTF-8"); check_masks(sms, 0);
    check_masks(mail, 1); /* Clearing SMS must leave the independent mail mask intact. */
    assert(HTTP_SettingsModelInit(generic_load, generic_save, generic_persisted) == 1);
    check_masks(sms, 0); check_masks(mail, 1);
    assert(!strcmp(HTTP_SettingsGetByName(adc, 1, "n_s_ttd0_099"), "1234"));

    /* Validation/storage failures do not report success or change the model. */
    before = generic_saves;
    generic_form("/nap001", 0, 0, 0); strcat(generic_target, "&unknown=1");
    request(0, generic_target); check_response(0, 400, "text/html; charset=UTF-8");
    assert(generic_saves == before);
    generic_form("/set", 1, 0, 1); generic_failure = 1;
    unsigned before_bindings = bindings_count;
    request(0, generic_target); check_response(0, 503, "text/html; charset=UTF-8");
    assert(generic_saves == before + 1u); assert(bindings_count == before_bindings);
    assert(HTTP_SettingsPortMode(5) == HTTP_MODE_TEMP_RHU);
    body(0, html, sizeof html); assert(strstr(html, "Sprawdz karte SD."));
    assert(strstr(html, "window.history.replaceState(null,'','/set')"));
    generic_failure = 0;
    check_main_route(5, "tu_temp005b", 1);
    puts("PASS: all Sandwitch settings routes/instances, explicit 501 actions, independent queued pages, UTF-8 HTTP lengths, /set name/mode persistence driving /main and / tiles, unique one-second refresh targets, mode-driven RH panel, fragmented 301-field ADC and complete SMS/mail forms, scoped checkbox clearing and 400/503 rollback.");
    return 0;
}
'''


def main():
    sensor_suite = sensor_routes.TESTS.replace(
        "int main(void)", "static int sensor_routes_suite(void)"
    )
    sensor_routes.compile_and_run(sensor_routes.harness_source(sensor_suite + TESTS))


if __name__ == "__main__":
    main()
