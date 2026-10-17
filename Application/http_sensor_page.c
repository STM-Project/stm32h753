/*
 * STM32H753 adaptation of the user-provided Sandwitch project:
 * Inc/http.h: HttpCzujki, Http_zapisz and HttpEnd; Src/http.c: /tempNNN page.
 * The card layout, classes, captions and CGI field names follow that source.
 * Shared HttpStyle/HttpMainMenu provide the original CSS and keyboard filter.
 * Stored values come from this project's sensor-settings model, in tenths.
 */
#include "http_sensor_page.h"
#include "http_sensor_settings.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

/* Avoid importing MCU-specific types: these fragments are ordinary strings. */
extern const char HttpStyle[];
extern const char HttpMainMenu[];

typedef struct {
    char *out;
    size_t capacity;
    size_t length;
    int failed;
} SensorPageWriter;

static void PageAppend(SensorPageWriter *writer, const char *format, ...)
{
    int written;
    va_list args;

    if (writer->failed)
        return;
    va_start(args, format);
    written = vsnprintf(writer->out + writer->length,
                        writer->capacity - writer->length, format, args);
    va_end(args);
    if (written < 0 || (size_t)written >= writer->capacity - writer->length) {
        writer->failed = 1;
        return;
    }
    writer->length += (size_t)written;
}

static void PageEscape(SensorPageWriter *writer, const char *text)
{
    while (text != NULL && *text != '\0' && !writer->failed) {
        switch (*text) {
        case '&': PageAppend(writer, "%s", "&amp;"); break;
        case '<': PageAppend(writer, "%s", "&lt;"); break;
        case '>': PageAppend(writer, "%s", "&gt;"); break;
        case '"': PageAppend(writer, "%s", "&quot;"); break;
        case '\'': PageAppend(writer, "%s", "&#39;"); break;
        default: PageAppend(writer, "%c", (unsigned char)*text); break;
        }
        ++text;
    }
}

static void PageNumber(SensorPageWriter *writer, const char *field,
                       unsigned sensor_id, int16_t value)
{
    int magnitude = value < 0 ? -(int)value : (int)value;

    PageAppend(writer,
        "<input type=\"text\" class=\"textbox\" style=\"width:50px;\" "
        "name=\"n_s_Czujki_%s_%03u\" size=\"5\" maxlength=\"7\" "
        "value=\"%c%02d.%d\" "
        "onkeypress=\"return maskujKlawisze(event, '+-.0123456789');\">",
        field, sensor_id, value < 0 ? '-' : '+', magnitude / 10,
        magnitude % 10);
}

static void PageCard(SensorPageWriter *writer, unsigned port, int humidity)
{
    const HttpSensorSettings *settings = HTTP_SensorSettingsGet(port, humidity);
    const char *unit = humidity ? "%" : "&deg;C";
    const unsigned sensor_id = (port - 1U) * 2U + (humidity ? 2U : 1U);

    if (settings == NULL) {
        writer->failed = 1;
        return;
    }
    PageAppend(writer,
        "\r\n<div class=\"corner-radius2\" style=\"margin-left: 0px\">\r\n"
        "<div class=\"corner-radius\" style=\"margin-left: 10px\">\r\n"
        "<font class=\"css-font-style\" style=\"font-family: Courier\">"
        "<b>%s Port %u</b></font><br>\r\n"
        "<font size=\"3\" color=\"#eeeeee\">"
        "-- -- -- -- -- -- - T%u - -- -- -- -- -- </font><br>\r\n"
        "<input type=\"text\" class=\"textbox\" style=\"width:270px;\" "
        "name=\"n_s_Czujki_nazwa_%03u\" size=\"30\" maxlength=\"30\" value=\"",
        humidity ? "Wilgotnosc" : "Temperatura", port, sensor_id, sensor_id);
    PageEscape(writer, settings->name);
    PageAppend(writer,
        "\"><br>\r\n<font class=\"css-font-style2\">Prog max</font>");
    PageNumber(writer, "prog_max1", sensor_id, settings->max1);
    PageAppend(writer, " ");
    PageNumber(writer, "prog_max2", sensor_id, settings->max2);
    PageAppend(writer,
        "<font class=\"css-font-style2\">%s</font><br>\r\n"
        "<font class=\"css-font-style2\">Prog min </font>", unit);
    PageNumber(writer, "prog_min1", sensor_id, settings->min1);
    PageAppend(writer, " ");
    PageNumber(writer, "prog_min2", sensor_id, settings->min2);
    PageAppend(writer,
        "<font class=\"css-font-style2\">%s</font><br>\r\n"
        "<font class=\"css-font-style2\">Histereza &nbsp;</font>", unit);
    PageNumber(writer, "hist", sensor_id, settings->hysteresis);
    PageAppend(writer,
        "<font class=\"css-font-style2\">%s</font><br>\r\n"
        "<font class=\"css-font-style2\">Korekta &nbsp;&nbsp;&nbsp;</font>", unit);
    PageNumber(writer, "korekcja", sensor_id, settings->correction);
    PageAppend(writer,
        "<font class=\"css-font-style2\">%s</font><br>\r\n"
        "<font class=\"css-font-style2\">Czas zwloki</font>"
        "<input type=\"text\" class=\"textbox\" style=\"width:50px;\" "
        "name=\"n_s_Czujki_czas_reak_%03u\" size=\"5\" maxlength=\"5\" "
        "value=\"%02u:%02u\" "
        "onkeypress=\"return maskujKlawisze(event, ':0123456789');\">"
        "<font class=\"css-font-style2\">m:s</font><br>\r\n"
        "</div></div>\r\n",
        unit, sensor_id, (unsigned)settings->delay_seconds / 60U,
        (unsigned)settings->delay_seconds % 60U);
}

size_t HTTP_SensorPageRender(char *out, size_t capacity, unsigned port,
                           int humidity_enabled, const char *notice)
{
    SensorPageWriter writer = {out, capacity, 0U, 0};

    if (out == NULL || capacity == 0U)
        return 0U;
    out[0] = '\0';
    if (port == 0U || port > HTTP_SENSOR_PORT_COUNT)
        return 0U;
    PageAppend(&writer, "%s%s", HttpStyle, HttpMainMenu);
    if (notice != NULL && *notice != '\0') {
        PageAppend(&writer, "<p role=\"status\" style=\"color:#eeeeee;\">");
        PageEscape(&writer, notice);
        PageAppend(&writer, "</p>\r\n");
    }
    PageAppend(&writer,
        "<form method=\"post\" action=\"/lpc.cgi/A117\" data-ajax=\"false\">\r\n");
    PageCard(&writer, port, 0);
    if (humidity_enabled)
        PageCard(&writer, port, 1);
    PageAppend(&writer,
        "<div class=\"corner-radius2\" style=\"margin-left: 0px\">\r\n"
        "<div class=\"corner-radius\" style=\"margin-left: 10px\">\r\n"
        "<br>\r\n&nbsp;<button type=\"submit\" name=\"n_109_zapisz\" "
        "class=\"css_button\">Zapisz</button>\r\n"
        "<br><a href=\"/set\" class=\"n\">Wroc</a>\r\n</div></div>\r\n"
        "</form>\r\n</body>\r\n<script>\r\n"
        "document.getElementById('tu_menu').innerHTML="
        "\"<li><a href='/'>Panel odczytowy</a></li>"
        "<li><a href='/set'>Konfiguracja portow</a></li>"
        "<li><a href='/lan'>LAN / WWW</a></li><li><a href='/wifi'>WiFi</a></li>"
        "<li><a href='/czas'>Czas</a></li><li><a href='/gsm'>GSM</a></li>"
        "<li><a href='/maskisms'>Powiadomienia SMS</a></li><li><a href='/maskimail'>E-mail</a></li>"
        "<li><a href='/nrf'>LoRa</a></li><li><a href='/rej'>Rejestrator</a></li>"
        "<li><a href='/logic'>Operacje logiczne</a></li><li><a href='/Rot'>Rotacja</a></li>"
        "<li><a href='/wykres1'>Wykres pomiarow</a></li><li><a href='/wykres2'>Wykres licznikow</a></li>\";\r\n"
        "</script>\r\n</html>\r\n");
    if (writer.failed) {
        out[0] = '\0';
        return 0U;
    }
    return writer.length;
}
