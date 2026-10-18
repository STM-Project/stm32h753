/* LPC2148 czolowka.htm layout/thermometers, STM32H753 live-data adapter. */
(function () {
    'use strict';
    var chart, thermometers = [], busy = false, commands = {}, sequence = 0, applied = 0;
    function id(prefix, n) { return prefix + ('00' + n).slice(-3); }
    function text(target, value) { document.getElementById(target).textContent = value; }
    function failed(xhr, reason) {
        var detail = xhr && xhr.status ? 'HTTP ' + xhr.status : reason === 'timeout' ? 'przekroczony czas' : 'brak odpowiedzi';
        text('connection', 'Błąd /api/nowa: ' + detail + ' — wartości mogą być nieaktualne');
        document.body.classList.add('stale');
    }
    function range(item) {
        var lo = item.min / 10, hi = item.max / 10;
        return hi > lo ? [lo, hi] : [-10, 50];
    }
    function showThermometer(i, value, bounds) {
        var fixture = i ? '#fixture' + (i + 1) : '#fixture', key = bounds.join('/');
        if (thermometers[i] && thermometers[i].key !== key) {
            $(fixture).thermometer('destroy');$(fixture).empty();thermometers[i]=null;
        }
        if (!thermometers[i]) {
            $(fixture).thermometer({startValue:0, height:150, width:'100%', minValue:0,
                maxValue:bounds[1]-bounds[0], bottomText:String(bounds[0]), topText:String(bounds[1]),
                svgMarkup:window.HTTP_LPC_SVG, animationSpeed:300,
                liquidColour:function(v){var p=Math.max(0,Math.min(1,v/(bounds[1]-bounds[0])));return 'rgb('+Math.round(255*p)+','+Math.round(128*(1-p))+','+Math.round(255*(1-p))+')';}});
            thermometers[i] = {key:key, bounds:bounds, element:$(fixture)};
        }
        var widget = thermometers[i];
        /* SVG is embedded in this document; no additional connection. */
        if (widget && $(fixture).find('svg').length) {
            $(fixture).thermometer('setValue', value === null ? 0 : value - bounds[0]);
        }
        $(fixture).toggleClass('no-measurement',value === null);
    }
    function apply(data, revision) {
        if (revision < applied) return;
        applied = revision;
        if (!data || data.temperatures.length !== 3 || data.inputs.length !== 7 || data.outputs.length !== 4) throw new Error('Niepełne dane');
        document.body.classList.remove('stale');
        text('connection', 'Ostatni odczyt: ' + new Date().toLocaleTimeString('pl-PL') + ' · odświeżanie co 1 s');
        var now = Date.now(), graphicsFailed = false;
        data.temperatures.forEach(function (item, i) {
            var value = item.tenths === null ? null : item.tenths / 10;
            var bounds = range(item), fixture = i ? '#fixture' + (i + 1) : '#fixture';
            text('name-temp-' + (i + 1), item.name + ' (port ' + item.port + ')');
            text(id('tu_temp', i + 1), value === null ? 'brak danych' : value.toFixed(1));
            var key = bounds.join('/');
            try { showThermometer(i, value, bounds); } catch(e) { graphicsFailed = true; }
            if (chart) {
            chart.series[i].update({name:item.name+' (port '+item.port+')'},false);
            chart.series[i].addPoint([now,value],false,chart.series[i].data.length >= 300);
            }
        });
        if (chart) chart.redraw();
        data.inputs.forEach(function (item, i) {
            var node = document.getElementById(id('tu_we',i+1));
            node.textContent = item.state === null ? '—' : item.state ? 'ON' : 'OFF';
            node.className = item.state === null ? 'unknown' : item.state ? 'on' : 'off';
            text(id('tu_we_p',i+1), 'Port ' + item.port);
            text('id_txt_we'+(i+1), item.name || 'Wejście ' + (i+1));
        });
        data.outputs.forEach(function (item,i) {
            var label = item.state === null ? 'brak potwierdzenia' : item.state ? 'ON' : 'OFF';
            if (item.requested !== null) label += ' · zadano ' + (item.requested ? 'ON' : 'OFF');
            text(id('tu_pk',i+1),label);
            text('name-output-'+(i+1),(item.name || 'PK'+(i+1))+' (port '+item.port+')');
            document.querySelector('[data-output="'+(i+1)+'"]').disabled = !item.enabled || !!commands[i+1];
        });
        if (graphicsFailed) text('connection','Dane odebrane; błąd wyświetlania termometru.');
        else if (data.temperatures.every(function(t){return t.tenths === null;}) && data.inputs.every(function(i){return i.state === null;}))
            text('connection','Połączenie OK — brak pomiarów. Program STM32 musi przekazać temperatury i stany wejść.');
    }
    function poll() {
        if (busy) return;
        busy = true;
        var revision = ++sequence;
        $.ajax({url:'/api/nowa',dataType:'json',cache:true,timeout:5000})
            .done(function (data) { try { apply(data, revision); } catch(e) { text('connection','Błąd danych /api/nowa: '+e.message); } })
            .fail(failed).always(function(){busy=false;});
    }
    $(function () {
        for (var i=0;i<3;i++) { try { showThermometer(i,null,[-10,50]); } catch(e) {} }
        try {
        Highcharts.setOptions({global:{useUTC:false}});
        chart = new Highcharts.Chart({chart:{renderTo:'container',type:'spline',animation:false},
            title:{text:'Temperatury'},xAxis:{type:'datetime'},yAxis:{title:{text:'°C'}},
            credits:{enabled:false},plotOptions:{series:{marker:{enabled:false},animation:false}},
            series:[{name:'Temperatura 1',data:[]},{name:'Temperatura 2',data:[]},{name:'Temperatura 3',data:[]}]});
        } catch(e) { chart=null; }
        document.querySelectorAll('[data-output]').forEach(function(button){
            button.addEventListener('click',function(event){
                event.preventDefault();var channel=Number(button.getAttribute('data-output'));
                if(commands[channel])return;
                commands[channel]=true;button.disabled=true;
                $.ajax({url:'/mobile/pk'+('00'+channel).slice(-3),timeout:5000})
                    .done(function(){text('connection','Polecenie wysłane; oczekiwanie na potwierdzenie wyjścia');})
                    .fail(function(){text('connection','Nie wysłano polecenia. Sprawdź tryb portu w /set.');})
                    .always(function(){delete commands[channel];poll();});
            });
        });
        poll();window.setInterval(poll,1000);
    });
}());
