#include "http_sensor_settings.h"

#include <string.h>

#define SENSOR_RECORD_SIZE 45u
#define SENSOR_FIELDS_ALL 255u

static HttpSensorSettings settings[HTTP_SENSOR_COUNT];
/* Shared scratch avoids a multi-kilobyte HTTP task stack allocation. */
static unsigned char blob[HTTP_SENSOR_SETTINGS_BLOB_SIZE];
static HttpSensorSettingsSave save_settings;
static void *storage_context;

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

static uint32_t checksum(const unsigned char *p, size_t length)
{
    uint32_t crc = UINT32_C(0xffffffff);
    size_t i;
    unsigned bit;
    for (i = 0; i < length; ++i) {
        crc ^= p[i];
        for (bit = 0; bit < 8u; ++bit)
            crc = (crc >> 1) ^ ((crc & 1u) ? UINT32_C(0xedb88320) : 0u);
    }
    return crc ^ UINT32_C(0xffffffff);
}

static int valid_name(const char *name)
{
    size_t i;
    for (i = 0; i <= HTTP_SENSOR_NAME_LENGTH; ++i) {
        unsigned char ch = (unsigned char)name[i];
        if (ch == 0u) return 1;
        if (ch < 32u || ch == 127u) return 0;
    }
    return 0;
}

static int valid_settings(const HttpSensorSettings *value, unsigned humidity)
{
    if (!valid_name(value->name) || value->hysteresis < 0 ||
        value->delay_seconds > 5999u) return 0;
    if (humidity && (value->max1 < 0 || value->max1 > 1000 ||
                     value->max2 < 0 || value->max2 > 1000 ||
                     value->min1 < 0 || value->min1 > 1000 ||
                     value->min2 < 0 || value->min2 > 1000 ||
                     value->hysteresis > 1000 ||
                     value->correction < -1000 || value->correction > 1000))
        return 0;
    /* The four original alarm thresholds are independent: no ordering implied. */
    return 1;
}

static void decode_record(HttpSensorSettings *value, const unsigned char *p)
{
    memcpy(value->name, p, sizeof value->name);
    value->max1 = (int16_t)get16(p + 31);
    value->max2 = (int16_t)get16(p + 33);
    value->min1 = (int16_t)get16(p + 35);
    value->min2 = (int16_t)get16(p + 37);
    value->hysteresis = (int16_t)get16(p + 39);
    value->correction = (int16_t)get16(p + 41);
    value->delay_seconds = get16(p + 43);
}

static void encode_record(unsigned char *p, const HttpSensorSettings *value)
{
    memcpy(p, value->name, sizeof value->name);
    put16(p + 31, (uint16_t)value->max1);
    put16(p + 33, (uint16_t)value->max2);
    put16(p + 35, (uint16_t)value->min1);
    put16(p + 37, (uint16_t)value->min2);
    put16(p + 39, (uint16_t)value->hysteresis);
    put16(p + 41, (uint16_t)value->correction);
    put16(p + 43, value->delay_seconds);
}

int HTTP_SensorSettingsValidateBlob(const void *buffer, size_t length)
{
    const unsigned char *p = (const unsigned char *)buffer;
    HttpSensorSettings candidate;
    unsigned i;
    if (!p || length != HTTP_SENSOR_SETTINGS_BLOB_SIZE ||
        memcmp(p, "SHT1", 4) != 0 || get16(p + 4) != 1u ||
        get16(p + 6) != HTTP_SENSOR_COUNT ||
        get32(p + 8) != HTTP_SENSOR_COUNT * SENSOR_RECORD_SIZE ||
        get32(p + 12) != checksum(p + 16, length - 16u)) return 0;
    for (i = 0; i < HTTP_SENSOR_COUNT; ++i) {
        decode_record(&candidate, p + 16u + i * SENSOR_RECORD_SIZE);
        if (!valid_settings(&candidate, i & 1u)) return 0;
    }
    return 1;
}

int HTTP_SensorSettingsInit(HttpSensorSettingsLoad load,
                           HttpSensorSettingsSave save, void *context)
{
    unsigned i;
    int result;
    save_settings = save;
    storage_context = context;
    memset(settings, 0, sizeof settings);
    for (i = 0; i < HTTP_SENSOR_COUNT; ++i)
        strcpy(settings[i].name, (i & 1u) ? "Wilgotnosc" : "Temperatura");
    if (!load) return 0;
    result = load(blob, sizeof blob, context);
    if (result == 0) return 0;
    if (result != 1 || !HTTP_SensorSettingsValidateBlob(blob, sizeof blob)) return -1;
    for (i = 0; i < HTTP_SENSOR_COUNT; ++i)
        decode_record(&settings[i], blob + 16u + i * SENSOR_RECORD_SIZE);
    return 1;
}

const HttpSensorSettings *HTTP_SensorSettingsGet(unsigned port, int humidity)
{
    if (port < 1u || port > HTTP_SENSOR_PORT_COUNT ||
        (humidity != 0 && humidity != 1)) return NULL;
    return &settings[(port - 1u) * 2u + (unsigned)humidity];
}

static int hex_digit(unsigned char ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

static int url_decode(char *out, size_t capacity, const char *value, size_t length)
{
    size_t i, used = 0;
    for (i = 0; i < length; ++i) {
        unsigned char ch = (unsigned char)value[i];
        if (ch == '%') {
            int a, b;
            if (i + 2u >= length) return 0;
            a = hex_digit((unsigned char)value[i + 1u]);
            b = hex_digit((unsigned char)value[i + 2u]);
            if (a < 0 || b < 0) return 0;
            ch = (unsigned char)(a * 16 + b);
            i += 2u;
        } else if (ch == '+') ch = ' ';
        if (ch == 0u || ch < 32u || ch == 127u || used + 1u >= capacity) return 0;
        out[used++] = (char)ch;
    }
    out[used] = '\0';
    return 1;
}

static int fixed_tenths(const char *text, int16_t *result)
{
    const unsigned char *p = (const unsigned char *)text;
    uint32_t integral = 0, fraction = 0, magnitude;
    unsigned digits = 0;
    int negative = 0;
    if (*p == '-' || *p == '+') { negative = *p == '-'; ++p; }
    while (*p >= '0' && *p <= '9') {
        if (++digits > 4u) return 0;
        integral = integral * 10u + (unsigned)(*p++ - '0');
    }
    if (!digits) return 0;
    if (*p == '.') {
        ++p;
        if (*p < '0' || *p > '9') return 0;
        fraction = (unsigned)(*p++ - '0');
    }
    if (*p != '\0') return 0;
    magnitude = integral * 10u + fraction;
    if (magnitude > (negative ? 32768u : 32767u)) return 0;
    *result = negative ? (int16_t)(-(int32_t)magnitude) : (int16_t)magnitude;
    return 1;
}

static int delay_value(const char *text, uint16_t *seconds)
{
    unsigned m, s;
    if (strlen(text) != 5u || text[2] != ':' ||
        text[0] < '0' || text[0] > '9' || text[1] < '0' || text[1] > '9' ||
        text[3] < '0' || text[3] > '9' || text[4] < '0' || text[4] > '9') return 0;
    m = (unsigned)(text[0] - '0') * 10u + (unsigned)(text[1] - '0');
    s = (unsigned)(text[3] - '0') * 10u + (unsigned)(text[4] - '0');
    if (s >= 60u) return 0;
    *seconds = (uint16_t)(m * 60u + s);
    return 1;
}

static int field_number(const char *name, unsigned *sensor)
{
    static const char *const fields[] = {
        "nazwa", "prog_max1", "prog_max2", "prog_min1", "prog_min2",
        "hist", "korekcja", "czas_reak"
    };
    const char *p;
    size_t length;
    unsigned i;
    if (strncmp(name, "n_s_Czujki_", 11u) != 0) return -1;
    p = name + 11u;
    for (i = 0; i < 8u; ++i) {
        length = strlen(fields[i]);
        if (strncmp(p, fields[i], length) == 0 && p[length] == '_') {
            p += length + 1u;
            if (strlen(p) != 3u || p[0] < '0' || p[0] > '9' ||
                p[1] < '0' || p[1] > '9' || p[2] < '0' || p[2] > '9') return -1;
            *sensor = (unsigned)(p[0] - '0') * 100u +
                      (unsigned)(p[1] - '0') * 10u + (unsigned)(p[2] - '0');
            return (int)i;
        }
    }
    return -1;
}

static int assign_field(HttpSensorSettings *value, unsigned field, const char *text)
{
    int16_t number;
    if (field == 0u) {
        if (strlen(text) > HTTP_SENSOR_NAME_LENGTH) return 0;
        memset(value->name, 0, sizeof value->name);
        strcpy(value->name, text);
        return valid_name(value->name);
    }
    if (field == 7u) return delay_value(text, &value->delay_seconds);
    if (!fixed_tenths(text, &number)) return 0;
    switch (field) {
    case 1: value->max1 = number; break;
    case 2: value->max2 = number; break;
    case 3: value->min1 = number; break;
    case 4: value->min2 = number; break;
    case 5: value->hysteresis = number; break;
    case 6: value->correction = number; break;
    default: return 0;
    }
    return 1;
}

int HTTP_SensorSettingsApplyTarget(const char *target, size_t length, unsigned *port)
{
    static const char route[] = "/lpc.cgi/A117?";
    HttpSensorSettings candidate[2];
    unsigned masks[2] = {0, 0}, selected_port = 0, seen_save = 0;
    size_t offset, end, equal, i;
    int has_sensor = 0, invalid = 0;
    if (port) *port = 0;
    if (!target || length < sizeof route - 1u ||
        memcmp(target, route, sizeof route - 1u) != 0)
        return HTTP_SENSOR_SETTINGS_NOT_HANDLED;
    /* Distinguish sensor forms from legacy A117 forms before strict parsing.
     * Scan bounded raw input too, so malformed sensor names cannot fall through
     * into the legacy CGI handler. Encoded parameter names remain supported.
     */
    for (i = sizeof route - 1u; i + 11u <= length && i < HTTP_SENSOR_TARGET_MAX; ++i) {
        if (memcmp(target + i, "n_s_Czujki_", 11u) == 0) { has_sensor = 1; break; }
    }
    offset = sizeof route - 1u;
    while (!has_sensor && offset < length && offset < HTTP_SENSOR_TARGET_MAX) {
        char name[64];
        end = offset;
        while (end < length && end < HTTP_SENSOR_TARGET_MAX && target[end] != '&') ++end;
        equal = offset;
        while (equal < end && target[equal] != '=') ++equal;
        if (url_decode(name, sizeof name, target + offset, equal - offset) &&
            strncmp(name, "n_s_Czujki_", 11u) == 0) has_sensor = 1;
        offset = end + 1u;
    }
    if (!has_sensor) return HTTP_SENSOR_SETTINGS_NOT_HANDLED;
    if (length > HTTP_SENSOR_TARGET_MAX) return HTTP_SENSOR_SETTINGS_INVALID;
    memset(candidate, 0, sizeof candidate);
    offset = sizeof route - 1u;
    while (offset < length) {
        char name[64], value[HTTP_SENSOR_NAME_LENGTH + 1u];
        unsigned sensor = 0, slot, this_port;
        int field;
        end = offset;
        while (end < length && target[end] != '&') ++end;
        equal = offset;
        while (equal < end && target[equal] != '=') ++equal;
        if (equal == end || equal == offset ||
            !url_decode(name, sizeof name, target + offset, equal - offset) ||
            !url_decode(value, sizeof value, target + equal + 1u, end - equal - 1u)) {
            invalid = 1;
            break;
        }
        if (strcmp(name, "n_109_zapisz") == 0) {
            if (seen_save) { invalid = 1; break; }
            seen_save = 1;
        } else {
            field = field_number(name, &sensor);
            if (field < 0 || sensor < 1u || sensor > HTTP_SENSOR_COUNT) {
                invalid = 1; break;
            }
            this_port = (sensor + 1u) / 2u;
            if (!selected_port) {
                selected_port = this_port;
                if (port) *port = selected_port;
            }
            if (selected_port != this_port) { invalid = 1; break; }
            slot = (sensor - 1u) & 1u;
            if ((masks[slot] & (1u << (unsigned)field)) ||
                !assign_field(&candidate[slot], (unsigned)field, value)) {
                invalid = 1; break;
            }
            masks[slot] |= 1u << (unsigned)field;
        }
        offset = end + 1u;
        if (end < length && offset == length) invalid = 1;
    }
    if (invalid || !seen_save || !selected_port || masks[0] != SENSOR_FIELDS_ALL ||
        (masks[1] != 0u && masks[1] != SENSOR_FIELDS_ALL) ||
        !valid_settings(&candidate[0], 0u) ||
        (masks[1] && !valid_settings(&candidate[1], 1u)))
        return HTTP_SENSOR_SETTINGS_INVALID;
    if (!save_settings) return HTTP_SENSOR_SETTINGS_STORAGE_ERROR;

    memcpy(blob, "SHT1", 4u);
    put16(blob + 4u, 1u);
    put16(blob + 6u, HTTP_SENSOR_COUNT);
    put32(blob + 8u, HTTP_SENSOR_COUNT * SENSOR_RECORD_SIZE);
    for (i = 0; i < HTTP_SENSOR_COUNT; ++i) {
        const HttpSensorSettings *value = &settings[i];
        if (i / 2u == selected_port - 1u && masks[i & 1u])
            value = &candidate[i & 1u];
        encode_record(blob + 16u + i * SENSOR_RECORD_SIZE, value);
    }
    put32(blob + 12u, checksum(blob + 16u, sizeof blob - 16u));
    if (save_settings(blob, sizeof blob, storage_context) != 1)
        return HTTP_SENSOR_SETTINGS_STORAGE_ERROR;
    settings[(selected_port - 1u) * 2u] = candidate[0];
    if (masks[1]) settings[(selected_port - 1u) * 2u + 1u] = candidate[1];
    return HTTP_SENSOR_SETTINGS_SAVED;
}
