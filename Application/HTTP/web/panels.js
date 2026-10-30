/* Only /main and /set use this loader. The HTML templates contain no saved
 * settings; only these small, uncached snapshots come from the shared model. */
(function () {
	'use strict';
	var settings = document.body.getAttribute('data-h7-panel') === 'set';
	var notice = document.getElementById('h7-notice');
	var dirty = false, lastConfig = '', pointerDown = false;
	var types = [8, 11, 1, 3, 6, 4, 0, 5, 8, 2, 10, 9, 0, 7];
	var groups = [8, 0, 0, 4, 4, 1, 2, 3, 6, 7, 7, 5];
	function pad(n, width) { return ('000' + n).slice(-width); }
	function request(url, done) {
		var xhr = new XMLHttpRequest(), finished = false;
		function finish(error) {
			if (finished) return;
			finished = true;
			done(error, xhr.responseText);
		}
		xhr.open('GET', url, true);
		xhr.timeout = 15000;
		xhr.onload = function () { finish(xhr.status !== 200); };
		xhr.onerror = xhr.ontimeout = xhr.onabort = function () { finish(true); };
		xhr.send();
	}
	function error() {
		var loading = document.getElementById('h7-loading');
		if (loading) {
			loading.textContent = 'Nie wczytano panelu. ';
			var retry = document.createElement('button');
			retry.textContent = 'Ponow';
			retry.onclick = function () { window.location.reload(); };
			loading.appendChild(retry);
		}
	}
	function addHidden(id) {
		if (document.getElementById(id)) return;
		var node = document.createElement('div');
		node.id = id;
		document.getElementById('h7-hidden').appendChild(node);
	}
	function readTiles(ports) {
		var grid = document.getElementById('h7-read');
		grid.textContent = '';
		document.getElementById('h7-hidden').textContent = '';
		var ordered = ports.map(function (p, i) {
			return {port: i + 1, data: p, type: types[p[0]] || 0};
		}).sort(function (a, b) { return groups[a.type] - groups[b.type] || a.port - b.port; });
		ordered.forEach(function (entry) {
			if (!entry.type) return;
			var nr = pad(entry.port, 3), box = document.createElement('div');
			box.innerHTML = document.getElementById('h7-tile-' + entry.type).innerHTML.split('255').join(nr);
			var labels = box.querySelectorAll('font.b');
			for (var j = 0; j < labels.length; j++) labels[j].textContent = entry.data[1];
			if (entry.type === 8) box.querySelector('a').setAttribute('onclick', 'xpk' + nr + '();return false;');
			var slider = box.querySelector('input[type=range]');
			if (slider) slider.name = 'n_pwm' + (entry.port - 20);
			while (box.firstChild) grid.appendChild(box.firstChild);
		});
		/* The existing TME reader addresses inactive modes too. Preserve all
		 * targets but never shadow the visible tile's IDs with hidden copies. */
		for (var p = 1; p <= 33; p++) {
			var n = pad(p, 3);
			if (p <= 20) {
				['a', 'b', 'c'].forEach(function (suffix) {
					addHidden('tu_temp' + n + suffix);
					addHidden('tu_lora' + n + suffix);
				});
				['tu_we', 'tu_eol', 'tu_adc', 'tu_ptt'].forEach(function (prefix) { addHidden(prefix + n); });
				addHidden('tu_lora' + n + 'we'); addHidden('tu_lora' + n + 'eol');
				for (var k = 0; k < 5; k++) addHidden('tu_imp' + k + '_' + n);
			} else {
				['tu_pk', 'tu_ti', 'tu_te'].forEach(function (prefix) { addHidden(prefix + n); });
				addHidden('tu_lora' + n + 'wy');
			}
		}
	}
	function applyConfig(ports) {
		var config = JSON.stringify(ports.map(function (p) { return [p[0], p[1]]; }));
		if (!settings && config !== lastConfig) {
			readTiles(ports);
			if (typeof window.loadXMLDoc === 'function') { window.wst = 0; window.loadXMLDoc(); }
		}
		if (settings && !dirty && !pointerDown) {
			ports.forEach(function (p, i) {
				var nr = pad(i + 1, 3);
				var input = document.querySelector('[name="n_s_Port_nazwa' + (i + 1) + '_001"]');
				if (input && input !== document.activeElement) input.value = p[1];
				var select = document.querySelector('[name="n_GPIO_val_' + nr + '"]');
				if (select) {
					select.value = pad(p[0], 2) + 'p' + nr;
					for (var mode = 0; mode <= 13; mode++) {
						var block = document.getElementById(pad(mode, 2) + 'p' + nr);
						if (block) block.style.display = mode === p[0] ? 'block' : 'none';
					}
					window['active_id' + nr] = select.value;
				}
			});
		}
		ports.forEach(function (p, i) {
			if (i < 20 || i > 22 || p[2] === null || pointerDown) return;
			var channel = i - 19;
			if (window.pwmBusy && (window.pwmBusy[channel] || window.pwmPending[channel] !== undefined)) return;
			var input = document.querySelector('input[type=range][name="n_pwm' + channel + '"]');
			if (input) input.value = p[2];
			var output = document.getElementById('volume' + pad(i + 1, 3));
			if (output) output.value = p[2];
		});
		lastConfig = config;
	}
	function configRequest(done) {
		request('/api/panel-config', function (failed, text) {
			var data;
			try { data = JSON.parse(text); } catch (ignore) { failed = true; }
			if (!data || !Array.isArray(data.ports) || data.ports.length !== 33) failed = true;
			done(failed, data);
		});
	}
	function pollConfig() {
		configRequest(function (failed, data) {
			if (!failed && !pointerDown) applyConfig(data.ports);
			window.setTimeout(pollConfig, 5000);
		});
	}
	request(settings ? H7_SET_TEMPLATE : H7_READ_TEMPLATE, function (failed, html) {
		if (failed) { error(); return; }
		configRequest(function (failed, data) {
			if (failed) { error(); return; }
			var doc = new DOMParser().parseFromString(html, 'text/html');
			var scripts = Array.prototype.slice.call(doc.querySelectorAll('script'));
			scripts.forEach(function (script) { script.parentNode.removeChild(script); });
			Array.prototype.slice.call(doc.head.querySelectorAll('style,link')).forEach(function (style) {
				document.head.appendChild(document.importNode(style, true));
			});
			document.body.innerHTML = doc.body.innerHTML;
			document.body.insertBefore(notice, document.body.firstChild);
			applyConfig(data.ports);
			Array.prototype.slice.call(document.querySelectorAll('a[onclick]')).forEach(function (a) {
				var handler = a.getAttribute('onclick');
				if (handler.indexOf('xpk') === 0) a.setAttribute('onclick', handler.split(';')[0] + ';return false;');
			});
			scripts.forEach(function (source) {
				var script = document.createElement('script');
				script.text = source.textContent;
				document.body.appendChild(script);
			});
			/* Legacy /set registers window.onload, but its DOM was installed
			 * asynchronously. Apply the visibility handler exactly once here. */
			if (settings && typeof window.Laduj === 'function') {
				window.onload = null;
				window.Laduj();
			}
			if (typeof window.loadXMLDoc === 'function') window.loadXMLDoc();
			document.addEventListener('input', edited);
			document.addEventListener('change', edited);
			function edited(event) {
				if (settings && event.target.matches('select[data-http-mode],input[type=text]')) dirty = true;
			}
			document.addEventListener('pointerdown', function () { pointerDown = true; });
			document.addEventListener('pointerup', function () { pointerDown = false; });
			document.addEventListener('pointercancel', function () { pointerDown = false; });
			window.setTimeout(pollConfig, 5000);
		});
	});
}());
