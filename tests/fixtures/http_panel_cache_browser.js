const {chromium} = require('playwright');
const assert = require('assert');
(async () => {
	const browser = await chromium.launch({executablePath: process.env.CHROMIUM_PATH || '/usr/bin/chromium', headless:true, args:['--no-sandbox']});
	const context = await browser.newContext();
	const page = await context.newPage();
	const errors = [];
	page.on('pageerror', e => errors.push(e.message));
	const base = 'http://127.0.0.1:' + process.argv[2];
	async function open(path, selector) {
		await page.goto(base + path);
		await page.locator(selector).first().waitFor();
	}
	async function config() {return page.evaluate(async () => (await (await fetch('/api/panel-config')).json()).ports);}
	async function post(values) {
		return page.evaluate(async values => {
			const r = await fetch('/lpc.cgi/A117',{method:'POST',body:new URLSearchParams(values)});
			return {status:r.status,text:await r.text()};
		}, values);
	}
	await open('/main', '#h7-read .y');
	assert((await page.locator('#h7-read .y').count()) > 0);
	const before = await config();
	// Every fixed-ID lookup in the actual legacy poller must resolve exactly once.
	const lookups = await page.evaluate(() => {
		const text = Array.from(document.scripts).map(s=>s.textContent).join('\n');
		return [...text.matchAll(/document\.getElementById\("([^"]+)"\)/g)].map(m=>m[1]);
	});
	for (const id of new Set(lookups)) assert.equal(await page.locator('[id="'+id+'"]').count(),1,id);
	await page.reload(); await page.locator('#h7-read .y').first().waitFor();
	await open('/set', 'select[data-http-mode="005"]');
	const name = page.locator('[name=n_s_Port_nazwa5_001]');
	assert.equal(await name.inputValue(), before[4][1]);
	const special = 'Kuchnia <ok> & "Łódź"';
	await name.fill(special);
	await Promise.all([page.waitForNavigation(), page.getByRole('button',{name:'Zapisz Nazwy',exact:true}).click()]);
	await page.locator('select[data-http-mode="005"]').waitFor();
	assert.equal(await name.inputValue(), special);
	assert((await page.locator('#h7-notice').innerText()).includes('Zapisano'));
	assert.equal(new URL(page.url()).pathname,'/set');
	const select = page.locator('select[data-http-mode="005"]');
	await select.selectOption('09p005');
	await Promise.all([page.waitForNavigation(), page.getByRole('link',{name:'Zapisz Tryb',exact:true}).click()]);
	await page.locator('select[data-http-mode="005"]').waitFor();
	assert.equal((await config())[4][0],9);
	assert.equal(await select.inputValue(),'09p005');
	assert(await page.locator('[id="09p005"]').isVisible());
	await open('/main', '#h7-read .y');
	assert(await page.locator('#tu_temp005b').isVisible());
	assert(await page.locator('#tu_temp005c').isVisible());
	assert(!await page.locator('#tu_temp005a').isVisible());
	assert((await page.locator('#h7-read').innerText()).includes(special));
	assert.equal(await page.locator('#h7-read ok').count(),0); // names never become markup
	// Simulate another HTTP/LCD writer: mutate the same committed /set model.
	let ports = await config();
	let modes={h7_page:'/set',h7_form:'1',n_109_zapisz:''};
	ports.forEach((p,i)=>modes['n_GPIO_val_'+String(i+1).padStart(3,'0')]=String(p[0]).padStart(2,'0'));
	modes.n_GPIO_val_005='04';modes.n_GPIO_val_021='01';modes.n_GPIO_val_028='00';
	assert.equal((await post(modes)).status,200);
	await page.waitForFunction(()=>document.getElementById('tu_we005')?.offsetHeight>0,{},{timeout:8000});
	assert(!await page.locator('#tu_temp005b').isVisible());
	// PWM still uses the existing shared output command, with the correct channel.
	const slider=page.locator('input[type=range][name=n_pwm1]');
	await slider.evaluate(n=>{n.value='73';n.dispatchEvent(new Event('input',{bubbles:true}));n.dispatchEvent(new Event('change',{bubbles:true}));});
	await page.waitForFunction(async()=> (await (await fetch('/api/panel-config')).json()).ports[20][2]===73);
	// Relay command is preserved too.
	const relayResponse=page.waitForResponse(r=>r.url().includes('/mobile/pk001'));
	await page.locator('#tu_pk028').click();
	assert.equal((await relayResponse).status(),200);
	await open('/set','select[data-http-mode="005"]');
	await name.fill('Lokalny szkic');
	const remote={h7_page:'/set',h7_form:'0',n_109_zapisz:'',n_s_Port_nazwa5_001:'Nazwa z LCD'};
	assert.equal((await post(remote)).status,200);
	await page.waitForTimeout(5500);
	assert.equal(await name.inputValue(),'Lokalny szkic');
	await page.reload();await page.locator('select[data-http-mode="005"]').waitFor();
	assert.equal(await name.inputValue(),'Nazwa z LCD');
	assert.equal(await select.inputValue(),'04p005');
	// Switching in a clean open form follows remote committed modes.
	modes.n_GPIO_val_005='02';assert.equal((await post(modes)).status,200);
	await page.waitForFunction(()=>document.querySelector('[name=n_GPIO_val_005]').value==='02p005',{},{timeout:8000});
	assert(await page.locator('[id="02p005"]').isVisible());
	assert.deepEqual(errors,[]);
	await browser.close();
})().catch(e=>{console.error(e);process.exit(1)});
