#include "http_settings_model.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#if defined(__arm__) || defined(__thumb__)
#define SETTINGS_SDRAM __attribute__((section(".sdram")))
#else
#define SETTINGS_SDRAM
#endif

#define SETTINGS_HEADER 24u
#define SETTINGS_VERSION 1u

static unsigned char current_blob[HTTP_SETTINGS_BLOB_SIZE] SETTINGS_SDRAM;
static unsigned char work_blob[HTTP_SETTINGS_BLOB_SIZE] SETTINGS_SDRAM;
static unsigned char seen[HTTP_SETTINGS_FIELDS_MAX];
static uint32_t replacement_keys[HTTP_SETTINGS_FIELDS_MAX] SETTINGS_SDRAM;
static HTTP_SettingsSave save_settings;
static void *storage_context;
static HTTP_SettingsDefaultResolver resolve_default;
static void *default_context;
static uint32_t saved_count;
static int initialized;

static int valid_value(const HTTP_SettingField *field, const char *value);

void HTTP_SettingsSetDefaultResolver(HTTP_SettingsDefaultResolver resolver, void *context)
{
    resolve_default = resolver;
    default_context = context;
}

static uint16_t get16(const unsigned char *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t get32(const unsigned char *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void put16(unsigned char *p, uint16_t value)
{
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
}

static void put32(unsigned char *p, uint32_t value)
{
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
    p[2] = (unsigned char)(value >> 16);
    p[3] = (unsigned char)(value >> 24);
}

static uint32_t crc_byte(uint32_t crc, unsigned char value)
{
    unsigned bit;
    crc ^= value;
    for (bit = 0; bit < 8u; ++bit)
        crc = (crc >> 1) ^ ((crc & 1u) ? UINT32_C(0xedb88320) : 0u);
    return crc;
}

static uint32_t checksum(const unsigned char *p, size_t length)
{
    uint32_t crc = UINT32_C(0xffffffff);
    size_t i;
    for (i = 0; i < length; ++i) crc = crc_byte(crc, p[i]);
    return crc ^ UINT32_C(0xffffffff);
}

static uint32_t crc_string(uint32_t crc, const char *value)
{
    if (value) while (*value) crc = crc_byte(crc, (unsigned char)*value++);
    return crc_byte(crc, 0);
}

/* The positional record IDs are safe only for the catalog which produced them.
 * Incompatible catalog edits are reported as invalid data instead of applying
 * an old password, address or port setting to a newly inserted field.
 */
static uint32_t schema_checksum(void)
{
    uint32_t crc = UINT32_C(0xffffffff);
    size_t p;
    unsigned f, byte;
    for (p = 0; p < HTTP_SettingsCatalogCount(); ++p) {
        const HTTP_SettingPage *page = HTTP_SettingsCatalogAt(p);
        crc = crc_string(crc, page->route);
        for (byte = 0; byte < 2u; ++byte)
            crc = crc_byte(crc, (unsigned char)(page->max_instance >> (8u * byte)));
        for (f = 0; f < page->field_count; ++f) {
            const HTTP_SettingField *field = &page->fields[f];
            crc = crc_string(crc, field->name);
            crc = crc_string(crc, field->choices);
            crc = crc_byte(crc, field->kind);
            crc = crc_byte(crc, field->flags);
            crc = crc_byte(crc, field->form_id);
            for (byte = 0; byte < 2u; ++byte)
                crc = crc_byte(crc, (unsigned char)(field->max_length >> (8u * byte)));
            for (byte = 0; byte < 4u; ++byte) {
                crc = crc_byte(crc, (unsigned char)((uint32_t)field->minimum >> (8u * byte)));
                crc = crc_byte(crc, (unsigned char)((uint32_t)field->maximum >> (8u * byte)));
            }
        }
    }
    return crc ^ UINT32_C(0xffffffff);
}

static int page_index(const HTTP_SettingPage *page)
{
    size_t p;
    for (p = 0; p < HTTP_SettingsCatalogCount() && p <= UINT16_MAX; ++p)
        if (page == HTTP_SettingsCatalogAt(p)) return (int)p;
    return -1;
}

static int valid_instance(const HTTP_SettingPage *page, uint16_t instance)
{
    return page->max_instance ? instance >= 1u && instance <= page->max_instance : instance == 0u;
}

static int field_index(const HTTP_SettingPage *page, const HTTP_SettingField *field)
{
    unsigned f;
    if (!page || !field) return -1;
    for (f = 0; f < page->field_count; ++f)
        if (field == &page->fields[f]) return (int)f;
    return -1;
}

static int field_by_name(const HTTP_SettingPage *page, uint16_t instance, const char *name)
{
    unsigned f;
    char expanded[HTTP_SETTINGS_FIELD_NAME_MAX + 1u];
    for (f = 0; f < page->field_count; ++f) {
        if (!HTTP_SettingsCatalogFieldName(&page->fields[f], instance, expanded, sizeof expanded))
            continue;
        if (strcmp(name, expanded) == 0) return (int)f;
    }
    return -1;
}

/* Sandwitch edits the same output/input/report variables from several pages.
 * Keep only these explicit aliases shared; logic banks and all other fields
 * retain their independent page/instance identity.
 */
static void canonical_key(unsigned *p, unsigned instance, unsigned *f)
{
    const HTTP_SettingPage *page = HTTP_SettingsCatalogAt(*p), *canonical;
    const char *route = NULL;
    char name[HTTP_SETTINGS_FIELD_NAME_MAX + 1u];
    uint16_t unused;
    int index;
    if ((strcmp(page->route, "/tim") == 0 || strcmp(page->route, "/term") == 0) &&
        strncmp(page->fields[*f].name, "n_s_PK_", 7u) == 0) route = "/wypk";
    else if (strcmp(page->route, "/eol") == 0 &&
             strncmp(page->fields[*f].name, "n_s_WE_", 7u) == 0) route = "/we";
    else if (strcmp(page->route, "/maskimail") == 0 &&
             strcmp(page->fields[*f].name, "n_s_RR_godzON1_000") == 0) route = "/maskisms";
    if (!route) return;
    /* Find the canonical catalog object directly: numbered routes require an
     * expanded instance path and therefore cannot be looked up by prefix.
     */
    canonical = NULL;
    for (size_t i = 0; i < HTTP_SettingsCatalogCount(); ++i)
        if (strcmp(HTTP_SettingsCatalogAt(i)->route, route) == 0) {
            canonical = HTTP_SettingsCatalogAt(i); break;
        }
    unused = (uint16_t)instance;
    if (!canonical || !valid_instance(canonical, unused) ||
        !HTTP_SettingsCatalogFieldName(&page->fields[*f], unused, name, sizeof name)) return;
    index = field_by_name(canonical, unused, name);
    if (index < 0) return;
    *p = (unsigned)page_index(canonical);
    *f = (unsigned)index;
}

static const unsigned char *record_at(const unsigned char *blob, uint32_t index)
{
    return blob + SETTINGS_HEADER + (size_t)index * HTTP_SETTINGS_RECORD_SIZE;
}

static const char *find_saved(unsigned p, unsigned instance, unsigned f)
{
    uint32_t i;
    if (!initialized) return NULL;
    for (i = 0; i < saved_count; ++i) {
        const unsigned char *record = record_at(current_blob, i);
        if (get16(record) == p && get16(record + 2) == instance && get16(record + 4) == f)
            return (const char *)record + 8;
    }
    return NULL;
}

const char *HTTP_SettingsGet(const HTTP_SettingPage *page, uint16_t instance,
                             const HTTP_SettingField *field)
{
    int p = page_index(page), f = field_index(page, field);
    unsigned canonical_p, canonical_f;
    const char *value;
    if (p < 0 || f < 0 || !valid_instance(page, instance)) return NULL;
    canonical_p = (unsigned)p; canonical_f = (unsigned)f;
    canonical_key(&canonical_p, instance, &canonical_f);
    value = find_saved(canonical_p, instance, canonical_f);
    if (value) return value;
    value = resolve_default ? resolve_default(page, instance, field, default_context) : NULL;
    if (value && field->kind == HTTP_SETTING_READONLY) {
        HTTP_SettingField display_field = *field;
        display_field.kind = HTTP_SETTING_TEXT;
        if (valid_value(&display_field, value)) return value;
    }
    return value && valid_value(field, value) ? value : (field->default_value ? field->default_value : "");
}

const char *HTTP_SettingsGetByName(const HTTP_SettingPage *page, uint16_t instance,
                                   const char *name)
{
    int f;
    if (page_index(page) < 0 || !name || !valid_instance(page, instance)) return NULL;
    f = field_by_name(page, instance, name);
    return f < 0 ? NULL : HTTP_SettingsGet(page, instance, &page->fields[f]);
}

const char *HTTP_SettingsGetSaved(const HTTP_SettingPage *page, uint16_t instance,
                                 const char *name)
{
    int p = page_index(page), f;
    unsigned canonical_p, canonical_f;
    if (p < 0 || !name || !valid_instance(page, instance)) return NULL;
    f = field_by_name(page, instance, name);
    if (f < 0) return NULL;
    canonical_p = (unsigned)p; canonical_f = (unsigned)f;
    canonical_key(&canonical_p, instance, &canonical_f);
    return find_saved(canonical_p, instance, canonical_f);
}

static int allowed_choice(const char *choices, const char *value)
{
    size_t length = strlen(value);
    const char *end;
    if (!choices) return 0;
    do {
        end = strchr(choices, '|');
        if ((end ? (size_t)(end - choices) : strlen(choices)) == length &&
            strncmp(choices, value, length) == 0) return 1;
        if (!end) return 0;
        choices = end + 1;
    } while (1);
}

static int decimal_value(const char *value, int fractional, int64_t *result)
{
    const unsigned char *p = (const unsigned char *)value;
    uint64_t whole = 0, fraction = 0;
    unsigned digits = 0;
    int negative = 0;
    if (*p == '-' || *p == '+') { negative = *p == '-'; ++p; }
    if (*p < '0' || *p > '9') return 0;
    while (*p >= '0' && *p <= '9') {
        whole = whole * 10u + (unsigned)(*p++ - '0');
        if (whole > UINT32_MAX) return 0;
    }
    if (*p == '.' && fractional) {
        ++p;
        while (*p >= '0' && *p <= '9') {
            if (++digits > 2u) return 0;
            fraction = fraction * 10u + (unsigned)(*p++ - '0');
        }
        if (!digits) return 0;
    }
    if (*p) return 0;
    if (digits == 1u) fraction *= 10u;
    *result = (int64_t)(whole * 100u + fraction);
    if (negative) *result = -*result;
    return 1;
}

static int valid_time(const char *value, int duration)
{
    size_t length = strlen(value), colon;
    unsigned first = 0, second;
    if (duration) {
        if (length < 5u || length > 7u) return 0;
        colon = length - 3u;
    } else {
        if (length != 5u && length != 8u) return 0;
        colon = 2u;
    }
    for (size_t i = 0; i < colon; ++i) {
        if (value[i] < '0' || value[i] > '9') return 0;
        first = first * 10u + (unsigned)(value[i] - '0');
    }
    if (value[colon] != ':' || value[colon + 1u] < '0' || value[colon + 1u] > '9' ||
        value[colon + 2u] < '0' || value[colon + 2u] > '9') return 0;
    second = (unsigned)(value[colon + 1u] - '0') * 10u + (unsigned)(value[colon + 2u] - '0');
    if (second > 59u || (!duration && first > 23u)) return 0;
    if (!duration && length == 8u) {
        if (value[5] != ':' || value[6] < '0' || value[6] > '5' ||
            value[7] < '0' || value[7] > '9') return 0;
    }
    return 1;
}

static int valid_ipv4(const char *value)
{
    const unsigned char *p = (const unsigned char *)value;
    unsigned octet;
    for (octet = 0; octet < 4u; ++octet) {
        unsigned n = 0, digits = 0;
        while (*p >= '0' && *p <= '9') {
            if (++digits > 3u) return 0;
            n = n * 10u + (unsigned)(*p++ - '0');
        }
        if (!digits || n > 255u) return 0;
        if (octet < 3u) { if (*p++ != '.') return 0; }
        else if (*p) return 0;
    }
    return 1;
}

static int hex_digit(unsigned char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int valid_mac(const char *value)
{
    size_t length = strlen(value), i;
    if (length == 12u) {
        for (i = 0; i < length; ++i) if (hex_digit((unsigned char)value[i]) < 0) return 0;
        return 1;
    }
    if (length != 17u || (value[2] != ':' && value[2] != '-')) return 0;
    for (i = 0; i < length; ++i) {
        if (i % 3u == 2u) { if (value[i] != value[2]) return 0; }
        else if (hex_digit((unsigned char)value[i]) < 0) return 0;
    }
    return 1;
}

/* /czas uses the STM32 RTC's two-digit year (2000..2099). Checking each
 * component alone would still accept 31 February and non-leap-year 29 February.
 */
static int valid_calendar(const unsigned char *blob, uint32_t count)
{
    static const unsigned char days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    static const char *const names[] = {
        "n_s_Czas_rok_000", "n_s_Czas_mies_000", "n_s_Czas_dzien_000"
    };
    uint16_t instance;
    const HTTP_SettingPage *page = HTTP_SettingsCatalogFind("/czas", &instance);
    int p = page_index(page);
    unsigned component[3], c;
    if (p < 0) return 1;
    for (c = 0; c < 3u; ++c) {
        int f = field_by_name(page, instance, names[c]);
        const char *value;
        int64_t number;
        uint32_t r;
        if (f < 0) return 0;
        value = page->fields[f].default_value;
        for (r = 0; r < count; ++r) {
            const unsigned char *record = record_at(blob, r);
            if (get16(record) == (unsigned)p && get16(record + 2) == instance &&
                get16(record + 4) == (unsigned)f) {
                value = (const char *)record + 8;
                break;
            }
        }
        if (!value || !decimal_value(value, 0, &number) || number < 0) return 0;
        component[c] = (unsigned)(number / 100);
    }
    if (component[0] > 99u || component[1] < 1u || component[1] > 12u || component[2] < 1u)
        return 0;
    return component[2] <= (unsigned)days[component[1] - 1u] +
           (component[1] == 2u && component[0] % 4u == 0u ? 1u : 0u);
}

static int valid_value(const HTTP_SettingField *field, const char *value)
{
    size_t length = strlen(value), i;
    int64_t number;
    if (length > HTTP_SETTINGS_VALUE_MAX || length > field->max_length) return 0;
    for (i = 0; i < length; ++i)
        if ((unsigned char)value[i] < 32u || (unsigned char)value[i] == 127u) return 0;
    if (!length && (field->flags & HTTP_SETTING_OPTIONAL)) return 1;
    switch (field->kind) {
    case HTTP_SETTING_TEXT:
    case HTTP_SETTING_SECRET: return 1;
    case HTTP_SETTING_READONLY:
        return strcmp(value, field->default_value ? field->default_value : "") == 0;
    case HTTP_SETTING_UINT:
    case HTTP_SETTING_DECIMAL:
        if (!decimal_value(value, field->kind == HTTP_SETTING_DECIMAL, &number)) return 0;
        return number >= (int64_t)field->minimum * 100 &&
               number <= (int64_t)field->maximum * 100;
    case HTTP_SETTING_DURATION: return valid_time(value, 1);
    case HTTP_SETTING_TIME: return valid_time(value, 0);
    case HTTP_SETTING_IPV4: return valid_ipv4(value);
    case HTTP_SETTING_MAC: return valid_mac(value);
    case HTTP_SETTING_SELECT: return allowed_choice(field->choices, value);
    case HTTP_SETTING_CHECKBOX:
        return strcmp(value, "0") == 0 ||
               (field->choices && field->choices[0] ? allowed_choice(field->choices, value) : strcmp(value, "1") == 0);
    default: return 0;
    }
}

static int record_compare(const void *a, const void *b)
{
    const unsigned char *left = a, *right = b;
    unsigned offset;
    for (offset = 0; offset < 6u; offset += 2u) {
        unsigned l = get16(left + offset), r = get16(right + offset);
        if (l != r) return l < r ? -1 : 1;
    }
    return 0;
}

int HTTP_SettingsModelValidateBlob(const void *buffer, size_t length)
{
    const unsigned char *data = buffer;
    uint32_t count, i;
    size_t tail;
    if (!buffer || length != HTTP_SETTINGS_BLOB_SIZE || memcmp(data, "H7CF", 4u) != 0 ||
        get16(data + 4) != SETTINGS_VERSION || get16(data + 6) != HTTP_SETTINGS_RECORD_SIZE ||
        get32(data + 8) != HTTP_SETTINGS_ENTRY_MAX || get32(data + 16) != schema_checksum() ||
        get32(data + 20) != checksum(data + SETTINGS_HEADER, length - SETTINGS_HEADER)) return 0;
    count = get32(data + 12);
    if (count > HTTP_SETTINGS_ENTRY_MAX) return 0;
    for (i = 0; i < count; ++i) {
        const unsigned char *record = record_at(data, i);
        unsigned p = get16(record), instance = get16(record + 2), f = get16(record + 4);
        unsigned n = get16(record + 6), j;
        const HTTP_SettingPage *page;
        unsigned canonical_p, canonical_f;
        if (p >= HTTP_SettingsCatalogCount() || n > HTTP_SETTINGS_VALUE_MAX) return 0;
        page = HTTP_SettingsCatalogAt(p);
        if (!valid_instance(page, (uint16_t)instance) || f >= page->field_count ||
            page->fields[f].kind == HTTP_SETTING_READONLY || record[8u + n] != 0u ||
            strlen((const char *)record + 8u) != n ||
            !valid_value(&page->fields[f], (const char *)record + 8u)) return 0;
        canonical_p = p; canonical_f = f;
        canonical_key(&canonical_p, instance, &canonical_f);
        if (canonical_p != p || canonical_f != f) return 0;
        for (j = n + 1u; j <= HTTP_SETTINGS_VALUE_MAX; ++j)
            if (record[8u + j] != 0u) return 0;
        if (i && record_compare(record_at(data, i - 1u), record) >= 0) return 0;
    }
    tail = SETTINGS_HEADER + (size_t)count * HTTP_SETTINGS_RECORD_SIZE;
    for (; tail < length; ++tail) if (data[tail]) return 0;
    return valid_calendar(data, count);
}

static void finish_blob(unsigned char *data, uint32_t count)
{
    memcpy(data, "H7CF", 4u);
    put16(data + 4, SETTINGS_VERSION);
    put16(data + 6, HTTP_SETTINGS_RECORD_SIZE);
    put32(data + 8, HTTP_SETTINGS_ENTRY_MAX);
    put32(data + 12, count);
    put32(data + 16, schema_checksum());
    put32(data + 20, checksum(data + SETTINGS_HEADER, HTTP_SETTINGS_BLOB_SIZE - SETTINGS_HEADER));
}

int HTTP_SettingsModelInit(HTTP_SettingsLoad load, HTTP_SettingsSave save, void *context)
{
    int result;
    result = load ? load(work_blob, sizeof work_blob, context) : 0;
    if (result != 0 && (result != 1 || !HTTP_SettingsModelValidateBlob(work_blob, sizeof work_blob))) {
        memset(work_blob, 0, sizeof work_blob);
        return -1;
    }
    save_settings = save;
    storage_context = context;
    initialized = 1;
    saved_count = result == 1 ? get32(work_blob + 12) : 0u;
    if (result == 1) memcpy(current_blob, work_blob, sizeof current_blob);
    else memset(current_blob, 0, sizeof current_blob);
    memset(work_blob, 0, sizeof work_blob);
    return result;
}

static int decode(const char *input, size_t length, char *output, size_t capacity)
{
    size_t i, used = 0;
    for (i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)input[i];
        if (c == '%') {
            int high, low;
            if (i + 2u >= length) return 0;
            high = hex_digit((unsigned char)input[++i]);
            low = hex_digit((unsigned char)input[++i]);
            if (high < 0 || low < 0) return 0;
            c = (unsigned char)((high << 4) | low);
        } else if (c == '+') c = ' ';
        if (c < 32u || c == 127u || used + 1u >= capacity) return 0;
        output[used++] = (char)c;
    }
    output[used] = '\0';
    return 1;
}

/* Returns one decoded pair, 0=end, -1=invalid. No read relies on NUL termination. */
static int pair(const char *query, size_t length, size_t *position,
                char *name, size_t name_size, char *value, size_t value_size)
{
    size_t start = *position, end = start, equal;
    if (start == length) return 0;
    while (end < length && query[end] != '&') ++end;
    if (end == start || (end + 1u == length && query[end] == '&')) return -1;
    equal = start;
    while (equal < end && query[equal] != '=') ++equal;
    if (equal == start || equal == end ||
        !decode(query + start, equal - start, name, name_size) ||
        !decode(query + equal + 1u, end - equal - 1u, value, value_size)) return -1;
    *position = end < length ? end + 1u : end;
    return 1;
}

static int append(unsigned p, unsigned instance, unsigned f, const char *value, uint32_t *count)
{
    unsigned char *record;
    size_t length = strlen(value);
    if (*count >= HTTP_SETTINGS_ENTRY_MAX) return 0;
    record = work_blob + SETTINGS_HEADER + (size_t)(*count)++ * HTTP_SETTINGS_RECORD_SIZE;
    put16(record, (uint16_t)p);
    put16(record + 2, (uint16_t)instance);
    put16(record + 4, (uint16_t)f);
    put16(record + 6, (uint16_t)length);
    memcpy(record + 8, value, length + 1u);
    return 1;
}

int HTTP_SettingsApplyTarget(const char *target, size_t length,
                             const HTTP_SettingPage **result_page, uint16_t *result_instance)
{
    const char *query;
    size_t query_length, position, i;
    char name[HTTP_SETTINGS_FIELD_NAME_MAX + 1u], value[HTTP_SETTINGS_VALUE_MAX + 1u];
    char route[HTTP_SETTINGS_VALUE_MAX + 1u] = "";
    const HTTP_SettingPage *page = NULL;
    uint16_t instance = 0;
    unsigned form = 0, page_seen = 0, form_seen = 0, submit_seen = 0, field_seen = 0;
    uint32_t count = 0;
    int p, status, malformed = 0;
    if (result_page) *result_page = NULL;
    if (result_instance) *result_instance = 0;
    if (!target || !length) return HTTP_SETTINGS_NOT_HANDLED;
    if (length > HTTP_SETTINGS_TARGET_MAX) return HTTP_SETTINGS_INVALID;
    query = memchr(target, '?', length);
    if (!query) return HTTP_SETTINGS_NOT_HANDLED;
    query_length = length - (size_t)(++query - target);
    position = 0;
    while ((status = pair(query, query_length, &position, name, sizeof name, value, sizeof value)) > 0) {
        if (strcmp(name, "h7_page") == 0) {
            if (++page_seen > 1u) { malformed = 1; continue; }
            memcpy(route, value, strlen(value) + 1u);
        } else if (strcmp(name, "h7_form") == 0) {
            int64_t number;
            if (++form_seen > 1u) { malformed = 1; continue; }
            if (!value[0] || strspn(value, "0123456789") != strlen(value) ||
                !decimal_value(value, 0, &number) || number < 0 || number > 25500) {
                malformed = 1;
                continue;
            }
            form = (unsigned)(number / 100);
        }
    }
    if (status < 0) malformed = 1;
    if (!page_seen) return HTTP_SETTINGS_NOT_HANDLED;
    if (strcspn(route, "? #") != strlen(route)) return HTTP_SETTINGS_INVALID;
    page = HTTP_SettingsCatalogFind(route, &instance);
    if (!page) return HTTP_SETTINGS_INVALID;
    if (result_page) *result_page = page;
    if (result_instance) *result_instance = instance;
    if (malformed || !form_seen || page->field_count > HTTP_SETTINGS_FIELDS_MAX) return HTTP_SETTINGS_INVALID;
    p = page_index(page);
    if (p < 0 || !valid_instance(page, instance)) return HTTP_SETTINGS_INVALID;
    memset(seen, 0, sizeof seen);
    position = 0;
    while ((status = pair(query, query_length, &position, name, sizeof name, value, sizeof value)) > 0) {
        int f;
        const HTTP_SettingField *field;
        if (strcmp(name, "h7_page") == 0 || strcmp(name, "h7_form") == 0) continue;
        if (strcmp(name, "n_109_zapisz") == 0 || strcmp(name, "n_czas_zapisz") == 0) {
            if (++submit_seen > 1u || strlen(value) > 32u) return HTTP_SETTINGS_INVALID;
            continue;
        }
        f = field_by_name(page, instance, name);
        if (f < 0 || seen[f]) return HTTP_SETTINGS_INVALID;
        field = &page->fields[f];
        if (field->form_id != form) return HTTP_SETTINGS_INVALID;
        if (field->kind == HTTP_SETTING_READONLY) {
            if (strcmp(value, HTTP_SettingsGet(page, instance, field)) != 0) return HTTP_SETTINGS_INVALID;
        } else if (!valid_value(field, value)) return HTTP_SETTINGS_INVALID;
        /* Passwords are never rendered into the form. A blank submission means
         * retain the prior value; a second occurrence is still a duplicate.
         */
        seen[f] = field->kind == HTTP_SETTING_SECRET && !value[0] ? 2u : 1u;
        ++field_seen;
    }
    if (status < 0 || !submit_seen) return HTTP_SETTINGS_INVALID;
    for (i = 0; i < page->field_count; ++i) {
        const HTTP_SettingField *field = &page->fields[i];
        if (field->form_id != form) continue;
        if (field->kind == HTTP_SETTING_CHECKBOX) {
            /* A form containing only checkboxes is valid with every box clear. */
            ++field_seen;
        } else if (!seen[i] && field->kind != HTTP_SETTING_READONLY &&
                   !(field->flags & HTTP_SETTING_OPTIONAL)) return HTTP_SETTINGS_INVALID;
    }
    if (!field_seen) return HTTP_SETTINGS_INVALID;
    if (!save_settings) return HTTP_SETTINGS_STORAGE_ERROR;
    for (i = 0; i < page->field_count; ++i) {
        const HTTP_SettingField *field = &page->fields[i];
        unsigned canonical_p = (unsigned)p, canonical_f = (unsigned)i;
        replacement_keys[i] = UINT32_MAX;
        if ((seen[i] == 1u && field->kind != HTTP_SETTING_READONLY) ||
            (field->form_id == form && field->kind == HTTP_SETTING_CHECKBOX)) {
            canonical_key(&canonical_p, instance, &canonical_f);
            replacement_keys[i] = ((uint32_t)canonical_p << 16) | canonical_f;
        }
    }
    memset(work_blob, 0, sizeof work_blob);
    for (uint32_t r = 0; r < saved_count; ++r) {
        const unsigned char *record = record_at(current_blob, r);
        unsigned old_p = get16(record), old_instance = get16(record + 2), f = get16(record + 4);
        int replace_record = 0;
        if (old_instance == instance) {
            uint32_t old_key = ((uint32_t)old_p << 16) | f;
            for (i = 0; i < page->field_count; ++i)
                if (replacement_keys[i] == old_key) { replace_record = 1; break; }
        }
        if (replace_record) continue;
        if (!append(old_p, old_instance, f, (const char *)record + 8, &count)) return HTTP_SETTINGS_CAPACITY_ERROR;
    }
    position = 0;
    while (pair(query, query_length, &position, name, sizeof name, value, sizeof value) > 0) {
        int f = field_by_name(page, instance, name);
        if (f >= 0 && page->fields[f].kind != HTTP_SETTING_READONLY &&
            !(page->fields[f].kind == HTTP_SETTING_SECRET && !value[0])) {
            unsigned canonical_p = (unsigned)p, canonical_f = (unsigned)f;
            canonical_key(&canonical_p, instance, &canonical_f);
            if (!valid_value(&HTTP_SettingsCatalogAt(canonical_p)->fields[canonical_f], value))
                return HTTP_SETTINGS_INVALID;
            if (!append(canonical_p, instance, canonical_f, value, &count)) return HTTP_SETTINGS_CAPACITY_ERROR;
        }
    }
    if (!valid_calendar(work_blob, count)) return HTTP_SETTINGS_INVALID;
    for (i = 0; i < page->field_count; ++i) {
        const HTTP_SettingField *field = &page->fields[i];
        if (!seen[i] && field->form_id == form && field->kind == HTTP_SETTING_CHECKBOX) {
            unsigned canonical_p = (unsigned)p, canonical_f = (unsigned)i;
            canonical_key(&canonical_p, instance, &canonical_f);
            if (!append(canonical_p, instance, canonical_f, "0", &count)) return HTTP_SETTINGS_CAPACITY_ERROR;
        }
    }
    qsort(work_blob + SETTINGS_HEADER, count, HTTP_SETTINGS_RECORD_SIZE, record_compare);
    finish_blob(work_blob, count);
    if (save_settings(work_blob, sizeof work_blob, storage_context) != 1) {
        memset(work_blob, 0, sizeof work_blob);
        return HTTP_SETTINGS_STORAGE_ERROR;
    }
    memcpy(current_blob, work_blob, sizeof current_blob);
    saved_count = count;
    initialized = 1;
    return HTTP_SETTINGS_SAVED;
}
