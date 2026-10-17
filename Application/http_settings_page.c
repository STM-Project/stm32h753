/* Sandwitch settings HTML adapted to the STM32H753 catalog/model.
 * The card layout, CSS classes, field names and units follow the supplied
 * Sandwitch Inc/http.h, Src/http.c and Src/wejscie_licznikowe.c.
 */
#include "http_settings_page.h"
#include "http_settings_model.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const char HttpStyle[];
extern const char HttpMainMenu[];
extern unsigned HTTP_SettingsPortMode(unsigned port);

typedef struct {
    char *out;
    size_t capacity;
    size_t length;
    int failed;
    const HTTP_SettingPage *page;
    uint16_t instance;
} SettingsWriter;

static void PageAppend(SettingsWriter *writer, const char *format, ...)
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

static void PageEscape(SettingsWriter *writer, const char *text)
{
    while (text != NULL && *text != '\0' && !writer->failed) {
        switch (*text) {
        case '&': PageAppend(writer, "&amp;"); break;
        case '<': PageAppend(writer, "&lt;"); break;
        case '>': PageAppend(writer, "&gt;"); break;
        case '"': PageAppend(writer, "&quot;"); break;
        case '\'': PageAppend(writer, "&#39;"); break;
        default: PageAppend(writer, "%c", (unsigned char)*text); break;
        }
        ++text;
    }
}

static const char *PageValue(SettingsWriter *writer, const char *name)
{
    const char *value = HTTP_SettingsGetByName(writer->page, writer->instance, name);
    return value != NULL ? value : "";
}

static const HTTP_SettingField *PageField(SettingsWriter *writer, const char *name)
{
    unsigned index;
    char expanded[HTTP_SETTINGS_FIELD_NAME_MAX + 1U];
    for (index = 0; index < writer->page->field_count; ++index) {
        const HTTP_SettingField *field = &writer->page->fields[index];
        if (HTTP_SettingsCatalogFieldName(field, writer->instance, expanded, sizeof expanded) &&
            strcmp(expanded, name) == 0)
            return field;
    }
    return NULL;
}

static void PageInput(SettingsWriter *writer, const char *name, const char *type,
                       unsigned width, unsigned max_length)
{
    const HTTP_SettingField *field = PageField(writer, name);
    const int secret = field != NULL && field->kind == HTTP_SETTING_SECRET;
    const char *value = secret ? "" : PageValue(writer, name);
    if (secret) type = "password";
    PageAppend(writer, "<input type=\"%s\" name=\"%s\" id=\"%s\" ", type, name, name);
    if (strcmp(type, "checkbox") == 0) {
        PageAppend(writer, "class=\"regular-checkbox checkbox\" value=\"1\"%s>"
                   "<label for=\"%s\"></label>", strcmp(value, "1") == 0 ? " checked" : "", name);
    } else {
        PageAppend(writer, "class=\"textbox\" style=\"width:%upx;\" maxlength=\"%u\" value=\"",
                   width, max_length);
        PageEscape(writer, value);
        PageAppend(writer, secret ? "\" autocomplete=\"new-password\" placeholder=\"pozostaw puste, aby zachowac\">" : "\">");
    }
}

static void PageSelect(SettingsWriter *writer, const char *name, const char *choices)
{
    const char *value = PageValue(writer, name);
    const char *start = choices;
    PageAppend(writer, "<div id=\"mainselection\"><select name=\"%s\">", name);
    while (start != NULL && *start != '\0') {
        const char *end = strchr(start, '|');
        size_t length = end != NULL ? (size_t)(end - start) : strlen(start);
        PageAppend(writer, "<option value=\"%.*s\"%s>%.*s</option>",
                   (int)length, start,
                   strlen(value) == length && strncmp(value, start, length) == 0 ? " selected" : "",
                   (int)length, start);
        start = end != NULL ? end + 1 : NULL;
    }
    PageAppend(writer, "</select></div>");
}

static void PageCardStart(SettingsWriter *writer, const char *title,
                           unsigned width, unsigned height)
{
    PageAppend(writer,
        "<div class=\"corner-radius2\" style=\"margin-left:0px;width:%upx;height:%upx;\">"
        "<div class=\"corner-radius\" style=\"margin-left:10px;width:%upx;height:%upx;\">"
        "<font class=\"css-font-style\" style=\"font-family:Courier;\"><b>",
        width, height, width > 30U ? width - 30U : width, height > 30U ? height - 30U : height);
    PageEscape(writer, title);
    PageAppend(writer, "</b></font><br>\r\n");
}

static void PageCardEnd(SettingsWriter *writer)
{
    PageAppend(writer, "</div></div>\r\n");
}

static void SettingsMasksRender(SettingsWriter *writer)
{
    const int sms = strcmp(writer->page->route, "/maskisms") == 0;
    unsigned contact, port;
    char name[64];
    PageCardStart(writer, sms ? "Numery Uzytkownikow" : "E-mail'e Uzytkownikow",
                  sms ? 450U : 390U, 660U);
    PageAppend(writer,
        "<font class=\"css-font-style2\">Powiadamianie uzytkownikow<br>"
        "o zaistnialych zdarzeniach</font><table>"
        "<tr><th>Numer / adres</th>%s<th>Raport</th></tr>", sms ? "<th>Dzwon</th>" : "");
    for (contact = 1; contact <= 8; ++contact) {
        PageAppend(writer, "<tr><td><font class=\"css_list\">%u.</font>", contact);
        snprintf(name, sizeof name, sms ? "n_s_nrtel%u_000" : "n_s_email_odb%u_000", contact);
        PageInput(writer, name, "text", 240, sms ? 17 : 40);
        if (sms) {
            PageAppend(writer, "</td><td>");
            snprintf(name, sizeof name, "n_s_xerrr%03u_000", contact);
            PageInput(writer, name, "checkbox", 0, 1);
        }
        PageAppend(writer, "</td><td>");
        snprintf(name, sizeof name, sms ? "n_s_derrr%03u_000" : "n_s_merrr%03u_000", contact);
        PageInput(writer, name, "checkbox", 0, 1);
        PageAppend(writer, "</td></tr>");
    }
    PageAppend(writer, "</table>");
    PageCardEnd(writer);
    PageCardStart(writer, sms ? "Wybor portow dla powiadamiania SMS" : "Wybor portow dla powiadamiania E-mail",
                  1050, 660);
    PageAppend(writer, "<div style=\"overflow:auto;\"><table><thead><tr><th>Port:</th>");
    for (port = 1; port <= 20; ++port)
        PageAppend(writer, "<th style=\"min-width:40px;\">%u</th>", port);
    PageAppend(writer, "</tr><tr><th>Typ:</th>");
    for (port = 1; port <= 20; ++port) {
        unsigned mode = HTTP_SettingsPortMode(port);
        PageAppend(writer, "<td>%s</td>", (mode == 4U || mode == 13U) ? "We" : "Te");
    }
    PageAppend(writer, "</tr></thead><tbody>");
    for (contact = 1; contact <= 8; ++contact) {
        PageAppend(writer, "<tr><td><font class=\"css_list\">%u.</font></td>", contact);
        for (port = 1; port <= 20; ++port) {
            PageAppend(writer, "<td>");
            snprintf(name, sizeof name, "n_s_Ma%c_akt%03u_%03u", sms ? 'S' : 'E', port, contact);
            PageInput(writer, name, "checkbox", 0, 1);
            PageAppend(writer, "</td>");
        }
        PageAppend(writer, "</tr>");
    }
    PageAppend(writer, "</tbody></table></div><font class=\"css-font-style2\">"
               "Te - temperatura, wilgotnosc; We - wejscie cyfrowe</font>");
    PageCardEnd(writer);
    PageCardStart(writer, "Ustawianie Raportow", 350, 660);
    PageAppend(writer, "<font class=\"css-font-style2\">Raport wysylany codziennie<br>o godz. </font>");
    PageInput(writer, "n_s_RR_godzON1_000", "text", 50, 5);
    PageCardEnd(writer);
}

static void SettingsTranslationRender(SettingsWriter *writer)
{
    unsigned row;
    char name[64];
    PageAppend(writer,
        "<div class=\"corner-radius4\" style=\"margin-left:0px;width:370px;height:5430px;\">"
        "<div class=\"corner-radius3\" style=\"margin-left:10px;width:340px;height:5400px;\">"
        "<font class=\"css-font-style\" style=\"font-family:Courier;\"><b>Tabela Translacji</b></font><br>"
        "<font class=\"css_list\"><b>&nbsp;Wej[V]&nbsp;&nbsp;&nbsp;&nbsp;Wyjscie &nbsp;&nbsp;j.m.</b></font><br>");
    for (row = 0; row < 100; ++row) {
        PageAppend(writer, "<font class=\"css_list\">%02u.%u</font>&nbsp;&nbsp;", row / 10U, row % 10U);
        snprintf(name, sizeof name, "n_s_ttp%u_%03u", (unsigned)writer->instance - 1U, row);
        PageInput(writer, name, "text", 15, 1);
        snprintf(name, sizeof name, "n_s_ttd%u_%03u", (unsigned)writer->instance - 1U, row);
        PageInput(writer, name, "text", 45, 4);
        PageAppend(writer, "<font class=\"css_list\">.</font>");
        snprintf(name, sizeof name, "n_s_ttm%u_%03u", (unsigned)writer->instance - 1U, row);
        PageInput(writer, name, "text", 25, 2);
        PageAppend(writer, "&nbsp;<font class=\"css_list\">");
        snprintf(name, sizeof name, "n_s_ttj%u_000", (unsigned)writer->instance - 1U);
        PageEscape(writer, PageValue(writer, name));
        PageAppend(writer, "</font><br>%s", row % 10U == 9U ? "<br>" : "");
    }
    PageAppend(writer, "</div></div>");
    PageCardStart(writer, "Jednostka miary", 350, 200);
    snprintf(name, sizeof name, "n_s_ttj%u_000", (unsigned)writer->instance - 1U);
    PageInput(writer, name, "text", 75, 5);
    PageCardEnd(writer);
}

static void SettingsCounterRender(SettingsWriter *writer)
{
    static const char periods[] = "gdtm";
    static const char *const captions[] = {"godzinny", "dobowy", "tygodniowy", "miesieczny"};
    unsigned period, threshold;
    char name[64];
    PageAppend(writer,
        "<div class=\"corner-radius4\" style=\"margin-left:0px;width:550px;height:830px;\">"
        "<div class=\"corner-radius3\" style=\"margin-left:10px;width:520px;height:800px;\">"
        "<font class=\"css-font-style\" style=\"font-family:Courier;\"><b>"
        "Progi Alarmowe dla powiadamiania</b></font><br>");
    for (period = 0; period < 4; ++period) {
        for (threshold = 1; threshold <= 2; ++threshold) {
            PageAppend(writer, "<font class=\"css_list\">Prog %u (%s)</font>&nbsp;", threshold, captions[period]);
            snprintf(name, sizeof name, "n_s_pEtr%cd%u_%03u", periods[period], threshold, writer->instance);
            PageInput(writer, name, "text", 70, 7);
            PageAppend(writer, "<font class=\"css_list\">.</font>");
            snprintf(name, sizeof name, "n_s_pEtr%cu%u_%03u", periods[period], threshold, writer->instance);
            PageInput(writer, name, "text", 50, 5);
            PageAppend(writer, "<br>");
        }
        PageAppend(writer, "<br>");
    }
    PageAppend(writer, "</div></div>");
}

static void SettingsRotationRender(SettingsWriter *writer)
{
    static const unsigned outputs[13] = {28,29,30,31,32,33,24,25,26,27,21,22,23};
    unsigned output, cycle;
    char name[64];
    PageCardStart(writer, "Ustalanie Cyklu Rotacji", 440, 1960);
    PageInput(writer, "n_s_xxROTxx", "checkbox", 0, 1);
    PageAppend(writer, "<font class=\"css-font-style2\">Rotacja aktywna</font>"
               "<table><tr><th>Port</th><th>Typ</th>");
    for (cycle = 1; cycle <= 8; ++cycle)
        PageAppend(writer, "<th>%u</th>", cycle);
    PageAppend(writer, "</tr>");
    for (output = 0; output < 13; ++output) {
        unsigned mode = HTTP_SettingsPortMode(outputs[output]);
        PageAppend(writer, "<tr><td>%02u</td><td>%s</td>", outputs[output], mode == 8U ? "IR" : "PK");
        for (cycle = 1; cycle <= 8; ++cycle) {
            PageAppend(writer, "<td>");
            snprintf(name, sizeof name, "n_s_Rot_akt%03u_%03u", cycle, output + 1U);
            PageInput(writer, name, "checkbox", 0, 1);
            PageAppend(writer, "</td>");
        }
        PageAppend(writer, "</tr>");
    }
    PageAppend(writer, "</table><font size=\"2\" color=\"#eeeeee\">"
               "Wybrane porty ustawic jako 'Przekaznik' lub 'Sterowanie IR'</font><br><br>"
               "<font class=\"css-font-style2\">Interwal czasowy miedzy cyklami</font><br>");
    PageSelect(writer, "n_s_Rot_dob_001", "00|01|02|03|04|05|06|07");
    PageAppend(writer, "<font class=\"css-font-style2\">Liczba dob (00 = bez pelnych dob)</font><br>");
    PageInput(writer, "n_s_Rot_int_001", "text", 50, 5);
    PageAppend(writer, "<font class=\"css-font-style2\">g:m<br>odmierzany od godziny </font>");
    PageInput(writer, "n_s_Rot_sta_001", "text", 50, 5);
    PageAppend(writer, "<br><font class=\"css-font-style2\">Alarmowy prog temperatury </font>");
    PageInput(writer, "n_s_Rot_max_001", "text", 50, 7);
    PageAppend(writer, "<font class=\"css-font-style2\">&deg;C<br>prog zatrzymujacy rotacje </font>");
    PageInput(writer, "n_s_Rot_min_001", "text", 50, 7);
    PageAppend(writer, "<font class=\"css-font-style2\">&deg;C<br>o czasie zwloki </font>");
    PageInput(writer, "n_s_Rot_rea_001", "text", 50, 5);
    PageAppend(writer, "<font class=\"css-font-style2\">m:s<br>i histerezie </font>");
    PageInput(writer, "n_s_Rot_hist_001", "text", 50, 7);
    PageAppend(writer, "<font class=\"css-font-style2\">&deg;C<br>"
               "dla czujnika temperatury z portu nr 1<br>"
               "(port 1 nalezy ustawic jako 'Temperatura')<br><br>"
               "Alarm przekaznikowy z portu nr 33<br>"
               "(port 33 nalezy ustawic jako 'Termostat')</font>");
    PageCardEnd(writer);
    PageCardStart(writer, "Nazwy Portow", 430, 940);
    PageAppend(writer, "<font class=\"css-font-style2\">PK - wyjscie przekaznikowe<br>IR - wyjscie IRDA</font><br>");
    for (output = 0; output < 13; ++output)
        PageAppend(writer, "<font class=\"css-font-style2\">%02u - Port %u</font><br>", outputs[output], outputs[output]);
    PageCardEnd(writer);
}

#include "http_settings_logic.inc"

static void PagePlaceholder(SettingsWriter *writer, const char *token)
{
    char *end;
    unsigned long index;
    if (strcmp(token, "N") == 0) {
        PageAppend(writer, "%03u", writer->instance);
        return;
    }
    if (strcmp(token, "I") == 0) {
        PageAppend(writer, "%u", writer->instance > 0U ? writer->instance - 1U : 0U);
        return;
    }
    if (strcmp(token, "OUTPUT_PORT") == 0) {
        static const unsigned outputs[13] = {28,29,30,31,32,33,24,25,26,27,21,22,23};
        if (writer->instance == 0U || writer->instance > 13U) writer->failed = 1;
        else PageAppend(writer, "%02u", outputs[writer->instance - 1U]);
        return;
    }
    if (strcmp(token, "INPUT_PORT") == 0) {
        if (writer->instance == 0U || writer->instance > 18U) writer->failed = 1;
        else PageAppend(writer, "%03u", writer->instance + (writer->instance > 14U ? 2U : 0U));
        return;
    }
    if (strcmp(token, "MASKS") == 0) { SettingsMasksRender(writer); return; }
    if (strcmp(token, "TRANSLATION") == 0) { SettingsTranslationRender(writer); return; }
    if (strcmp(token, "COUNTER") == 0) { SettingsCounterRender(writer); return; }
    if (strcmp(token, "ROTATION") == 0) { SettingsRotationRender(writer); return; }
    if (strcmp(token, "LOGIC") == 0) { SettingsLogicRender(writer); return; }
    if (strcmp(token, "EMAIL_STATUS") == 0) {
        PageAppend(writer, "Ustawienia SMTP zapisane lokalnie. Wysylanie e-mail wymaga obslugi SMTP w firmware.");
        return;
    }
    if (strcmp(token, "RECORDER") == 0 || strcmp(token, "SMS_COMMANDS") == 0) {
        PageAppend(writer, "<p style=\"color:#eee;\">Konfiguracja jest zapisywana na karcie SD. "
                   "Wykonywanie funkcji wymaga odpowiedniego modulu i jego obslugi w firmware.</p>");
        return;
    }
    if ((token[0] == 'V' || token[0] == 'S' || token[0] == 'C') && token[1] == ':') {
        index = strtoul(token + 2, &end, 10);
        if (end == token + 2 || index >= writer->page->field_count) {
            writer->failed = 1;
            return;
        }
        {
            const HTTP_SettingField *field = &writer->page->fields[index];
            const char *value = field->kind == HTTP_SETTING_SECRET ? "" :
                                HTTP_SettingsGet(writer->page, writer->instance, field);
            if (value == NULL) value = "";
            if (token[0] == 'V' && *end == '\0') PageEscape(writer, value);
            else if (token[0] == 'C' && *end == '\0') PageAppend(writer, "%s", strcmp(value, "1") == 0 ? "checked" : "");
            else if (token[0] == 'S' && *end == ':') PageAppend(writer, "%s", strcmp(value, end + 1) == 0 ? "selected" : "");
            else writer->failed = 1;
        }
        return;
    }
    /* Unknown markers are a catalog/renderer integration error, never output. */
    writer->failed = 1;
}

static void PageTemplate(SettingsWriter *writer, const char *html)
{
    while (html != NULL && *html != '\0' && !writer->failed) {
        const char *marker = strstr(html, "{{");
        const char *close;
        char token[128];
        size_t length;
        if (marker == NULL) { PageAppend(writer, "%s", html); break; }
        PageAppend(writer, "%.*s", (int)(marker - html), html);
        close = strstr(marker + 2, "}}");
        if (close == NULL || (length = (size_t)(close - marker - 2)) >= sizeof token) {
            writer->failed = 1;
            break;
        }
        memcpy(token, marker + 2, length);
        token[length] = '\0';
        PagePlaceholder(writer, token);
        html = close + 2;
    }
}

size_t HTTP_SettingsPageRender(char *out, size_t capacity,
                               const HTTP_SettingPage *page,
                               uint16_t instance, const char *notice)
{
    SettingsWriter writer = {out, capacity, 0U, 0, page, instance};
    char path[64];
    if (out == NULL || capacity == 0U) return 0U;
    out[0] = '\0';
    if (page == NULL || strcmp(page->route, "/set") == 0 ||
        !HTTP_SettingsCatalogPath(page, instance, path, sizeof path)) return 0U;
    PageAppend(&writer, "%s%s", HttpStyle, HttpMainMenu);
    if (notice != NULL && *notice != '\0') {
        PageAppend(&writer, "<p role=\"status\" style=\"color:#eeeeee;\">");
        PageEscape(&writer, notice);
        PageAppend(&writer, "</p>");
    }
    if (page->field_count != 0U) {
        PageAppend(&writer, "<form method=\"get\" action=\"/lpc.cgi/A117\" data-ajax=\"false\">"
               "<input type=\"hidden\" name=\"h7_page\" value=\"");
        PageEscape(&writer, path);
        PageAppend(&writer, "\"><input type=\"hidden\" name=\"h7_form\" value=\"0\">");
    }
    PageTemplate(&writer, page->html);
    if (page->field_count != 0U && strcmp(page->route, "/logic") != 0) {
        PageCardStart(&writer, "Zapis ustawien", 350, 200);
        PageAppend(&writer, "<br><button type=\"submit\" name=\"n_109_zapisz\" "
                   "class=\"css_button\">Zapisz</button><br>"
                   "<a href=\"/set\" class=\"n\">Wroc</a>");
        PageCardEnd(&writer);
    } else {
        PageAppend(&writer, "<a href=\"/set\" class=\"n\">Wroc</a>");
    }
    if (page->field_count != 0U) PageAppend(&writer, "</form>");
    PageAppend(&writer,
        "<script>document.getElementById('tu_menu').innerHTML="
        "\"<li><a href='/'>Panel odczytowy</a></li>"
        "<li><a href='/set'>Konfiguracja portow</a></li>"
        "<li><a href='/lan'>LAN / WWW</a></li>"
        "<li><a href='/wifi'>WiFi</a></li>"
        "<li><a href='/czas'>Czas</a></li>"
        "<li><a href='/rej'>Rejestrator</a></li>"
        "<li><a href='/gsm'>GSM</a></li>"
        "<li><a href='/maskisms'>Powiadomienia SMS</a></li>"
        "<li><a href='/maskimail'>E-mail</a></li>"
        "<li><a href='/nrf'>LoRa</a></li>"
        "<li><a href='/nap001'>Tabela translacji 1</a></li>"
        "<li><a href='/nap002'>Tabela translacji 2</a></li>"
        "<li><a href='/Rot'>Rotacja</a></li>"
        "<li><a href='/logic'>Operacje logiczne</a></li>"
        "<li><a href='/wykres1'>Wykres pomiarow</a></li>"
        "<li><a href='/wykres2'>Wykres licznikow</a></li>"
        "<li><a href='/upload'>Aktualizacja oprogramowania</a></li>\";"
        "</script></body></html>\r\n");
    if (writer.failed) { out[0] = '\0'; return 0U; }
    return writer.length;
}
