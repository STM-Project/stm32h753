/*
 * Sandwitch logic editor, adapted from HttpScriptLogic_1..4 and HttpLogicEntry.
 * Included by http_settings_page.c after its bounded SettingsWriter helpers.
 * The original 4 x 8 -> 2 x 4 -> 1 x 2 diagram, field names and operators are
 * retained. Each output has a separate /logicNNN settings bank on the H753.
 * This page edits stored configuration; it does not evaluate physical outputs.
 */

static void LogicOption(SettingsWriter *writer, unsigned value,
                        const char *label, unsigned selected)
{
    PageAppend(writer, "<option value=\"%u\"%s>", value,
               value == selected ? " selected" : "");
    PageEscape(writer, label);
    PageAppend(writer, "</option>");
}

/* kind: 0=physical input, 1=initial gate output, 2=intermediate gate output,
 * 3=BUFFER/NOT, 4=gate operator, 5=physical output / editor bank. */
static void LogicSelect(SettingsWriter *writer, const char *name,
                        unsigned kind, unsigned x, unsigned y, unsigned width,
                        const char *caption)
{
    const char *saved = PageValue(writer, name);
    unsigned selected = 0U;
    unsigned index;
    char label[40];
    static const char *const operators[] = {"---", "OR", "AND", "NOR", "NAND", "XOR"};
    static const char *const buffers[] = {"---", "BUFFER", "NOT"};

    if (saved != NULL && saved[0] >= '0' && saved[0] <= '9') {
        selected = (unsigned)(saved[0] - '0');
        if (saved[1] >= '0' && saved[1] <= '9')
            selected = selected * 10U + (unsigned)(saved[1] - '0');
    }
    /* The bank number is authoritative; a stale field cannot select another
     * output's network or make the first visit display the wrong output. */
    if (kind == 5U)
        selected = writer->instance > 0U ? writer->instance - 1U : 0U;
    PageAppend(writer,
        "<div class=\"h7logic-control\" style=\"left:%upx;top:%upx;\">"
        "<select id=\"", x + 5U, y + 5U);
    PageEscape(writer, name);
    PageAppend(writer, "\" name=\"");
    PageEscape(writer, name);
    PageAppend(writer, "\" aria-label=\"");
    PageEscape(writer, caption);
    PageAppend(writer, "\" style=\"width:%upx;%s\"%s>", width,
               kind == 5U ? "font-size:17px;padding:8px 15px;" : "",
               kind == 5U ? " data-logic-bank=\"true\"" : "");
    if (kind == 0U) {
        LogicOption(writer, 0U, "--- --- ---", selected);
        for (index = 1U; index <= 20U; ++index) {
            (void)snprintf(label, sizeof(label), "Port %u", index);
            LogicOption(writer, index, label, selected);
        }
        for (index = 1U; index <= 20U; ++index) {
            (void)snprintf(label, sizeof(label), "W: Port %u", index);
            LogicOption(writer, index + 60U, label, selected);
        }
    } else if (kind == 1U || kind == 2U) {
        LogicOption(writer, 0U, "---", selected);
        for (index = 1U; index <= (kind == 1U ? 4U : 2U); ++index) {
            if (kind == 1U)
                (void)snprintf(label, sizeof(label), "Sygnal %c", 'A' + (int)index - 1);
            else
                (void)snprintf(label, sizeof(label), "Sygnal %u", index);
            LogicOption(writer, index, label, selected);
        }
    } else if (kind == 3U || kind == 4U) {
        for (index = 0U; index < (kind == 3U ? 3U : 6U); ++index)
            LogicOption(writer, index, kind == 3U ? buffers[index] : operators[index], selected);
    } else {
        static const unsigned outputs[13] = {28,29,30,31,32,33,24,25,26,27,21,22,23};
        for (index = 0U; index < 13U; ++index) {
            (void)snprintf(label, sizeof(label), "Wyjscie %u - Port %u", index + 1U, outputs[index]);
            LogicOption(writer, index, label, selected);
        }
    }
    PageAppend(writer, "</select></div>\r\n");
}

static const char SettingsLogicScript[] =
    "<script>(function(){'use strict';"
    "var canvas=document.getElementById('myCanvas');"
    "var ctx=canvas&&canvas.getContext?canvas.getContext('2d'):null;"
    "function text(x,y,s,color,font){ctx.fillStyle=color;ctx.font=font;ctx.fillText(s,x,y);}"
    "function round(x,y,w,h,r){ctx.beginPath();ctx.lineWidth=4;ctx.strokeStyle='#D0D0D0';"
    "ctx.moveTo(x,y+r);ctx.lineTo(x,y+h-r);ctx.arcTo(x,y+h,x+r,y+h,r);"
    "ctx.lineTo(x+w-r,y+h);ctx.arcTo(x+w,y+h,x+w,y+h-r,r);ctx.lineTo(x+w,y+r);"
    "ctx.arcTo(x+w,y,x+w-r,y,r);ctx.lineTo(x+r,y);ctx.arcTo(x,y,x,y+r,r);ctx.stroke();}"
    "function arrow(x,y,w,h,len){ctx.beginPath();ctx.fillStyle='#D0D0D0';ctx.moveTo(x,y);"
    "ctx.lineTo(x,y+h);ctx.lineTo(x+w,y+h/2);ctx.fill();ctx.fillRect(x-len,y+h/3,len,h/3);}"
    "function rotated(x,y,w,h,len,deg){ctx.save();ctx.rotate(deg*Math.PI/180);"
    "arrow(x,y,w,h,len);ctx.restore();}"
    "function gate(x,y,a,b,label){text(x,y,'Operator','#FFFFFF','15px Arial');"
    "text(x+2,y+20,'logiczny','#FFFFFF','15px Arial');round(x-113,y-29,193,225,20);"
    "for(var i=a;i<b;i++){arrow(x-137,y-9+i*24,20,15,10);}"
    "if(x===463&&y===111){text(x+129,y+117,label,'#FFFFFF','15px Arial');"
    "rotated(x+129,y-250,20,60,15,30);}"
    "else if(x===463&&y===952){text(x+129,y+57,label,'#FFFFFF','15px Arial');"
    "rotated(x-485,y+190,20,60,15,-30);}"
    "else if(x===963&&y===390){text(x+129,y+118,label,'#FFFFFF','15px Arial');"
    "rotated(x+203,y-538,20,60,15,30);}"
    "else if(x===963&&y===671){text(x+129,y+57,label,'#FFFFFF','15px Arial');"
    "rotated(x-410,y+478,20,60,15,-30);}"
    "else{text(x+129,y+87,label,'#FFFFFF','15px Arial');arrow(x+99,y+52,20,60,15);}}"
    "if(ctx){text(850,50,'Wybor sygnalu wyjsciowego','#9edf0f','bold 20px Arial');"
    "text(90,70,'Sygnaly wejsciowe','#9edf0f','bold 18px Arial');"
    "gate(463,111,0,8,'Sygnal A');gate(463,391,0,8,'Sygnal B');"
    "gate(463,671,0,8,'Sygnal C');gate(463,952,0,8,'Sygnal D');"
    "text(710,405,'Sygnaly wtorne','#9edf0f','bold 18px Arial');"
    "gate(963,390,2,6,'Sygnal 1');gate(963,671,2,6,'Sygnal 2');"
    "text(1210,565,'Sygnaly wtorne','#9edf0f','bold 18px Arial');"
    "gate(1463,531,3,5,'Zdarzenie zaistniale SMS');"
    "text(1593,575,'Sygnal wyjsciowy','#9edf0f','bold 18px Arial');}"
    "var output=document.querySelector('[data-logic-bank]');"
    "if(output){output.addEventListener('change',function(){"
    "var bank=Number(output.value)+1;"
    "if(Number.isInteger(bank)&&bank>=1&&bank<=13){"
    "window.location.assign('/logic'+String(bank).padStart(3,'0'));}});}"
    "})();</script>\r\n";

static void SettingsLogicRender(SettingsWriter *writer)
{
    unsigned gate;
    unsigned input;
    char name[24];
    char caption[64];

    PageAppend(writer, "%s",
        "<style>.h7logic-scroll{overflow-x:auto;max-width:100%;}"
        ".h7logic{position:relative;width:1890px;height:1240px;color:#fff;}"
        ".h7logic canvas{display:block;border:5px solid #909090;background:#505050;}"
        ".h7logic-control{position:absolute;transform:translate(-50%,-50%);}"
        ".h7logic-control select{border:0;background:#707070;color:#fff;font-size:13px;"
        "padding:2px 5px;box-sizing:border-box;}"
        "</style><div class=\"h7logic-scroll\"><div class=\"h7logic\">"
        "<canvas id=\"myCanvas\" width=\"1880\" height=\"1230\">"
        "Your browser does not support the HTML canvas tag.</canvas>\r\n");
    LogicSelect(writer, "n_s_SigWy", 5U, 1010U, 95U, 300U, "Wybor sygnalu wyjsciowego");
    for (gate = 0U; gate < 7U; ++gate) {
        unsigned count = gate < 4U ? 8U : gate < 6U ? 4U : 2U;
        unsigned operator_x = gate < 4U ? 510U : gate < 6U ? 1010U : 1510U;
        unsigned operator_y = gate < 4U ? 195U + gate * 280U :
                              gate < 6U ? 475U + (gate - 4U) * 280U : 615U;
        unsigned source_x = gate < 4U ? 190U : gate < 6U ? 778U : 1278U;
        unsigned buffer_x = gate < 4U ? 420U : gate < 6U ? 920U : 1420U;
        unsigned source_y = gate < 4U ? 111U + gate * 280U :
                            gate < 6U ? 439U + (gate - 4U) * 280U : 603U;
        (void)snprintf(name, sizeof(name), "n_s_OL0%c", 'a' + (int)gate);
        (void)snprintf(caption, sizeof(caption), "Operator logiczny %c", 'A' + (int)gate);
        LogicSelect(writer, name, 4U, operator_x, operator_y, 70U, caption);
        for (input = 1U; input <= count; ++input) {
            (void)snprintf(name, sizeof(name), "n_s_%s%u%c", gate < 4U ? "SW" : "SP",
                           input, 'a' + (int)gate);
            (void)snprintf(caption, sizeof(caption), "Sygnal wejsciowy %c %u", 'A' + (int)gate, input);
            LogicSelect(writer, name, gate < 4U ? 0U : gate < 6U ? 1U : 2U,
                        source_x, source_y + (input - 1U) * 24U,
                        gate < 4U ? 252U : 90U, caption);
            (void)snprintf(name, sizeof(name), "n_s_OL%u%c", input, 'a' + (int)gate);
            (void)snprintf(caption, sizeof(caption), "BUFFER NOT %c %u", 'A' + (int)gate, input);
            LogicSelect(writer, name, 3U, buffer_x, source_y + (input - 1U) * 24U, 85U, caption);
        }
    }
    PageAppend(writer, "%s",
        "<div class=\"h7logic-control\" style=\"top:682px;left:1695px;\">"
        "<button type=\"submit\" name=\"n_109_zapisz\" class=\"css_button\">Zapisz</button>"
        "</div></div></div>\r\n");
    PageAppend(writer, "%s", SettingsLogicScript);
}
