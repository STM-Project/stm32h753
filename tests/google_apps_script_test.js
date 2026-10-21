/* Offline contract tests; Google Sheets and service APIs are mocked. */
const fs = require('node:fs');
const vm = require('node:vm');
const assert = require('node:assert/strict');
const properties = new Map([
  ['DEVICE_TOKEN', 'a'.repeat(64)], ['DEVICE_ID', 'stm32f769-demo'], ['SHEET_ID', 'test']
]);
let desired = false, rows = [], status, sheetReads = 0, released = 0, busy = false;
const sheet = {
  getRange: range => ({
    getValue: () => range === 'A2' ? 'stm32f769-demo' : desired,
    setValues: values => { status = values[0]; }
  })
};
const log = {
  getLastRow: () => rows.length + 1,
  deleteRows: (start, count) => rows.splice(start - 2, count),
  appendRow: row => rows.push(row)
};
const context = vm.createContext({
  PropertiesService: { getScriptProperties: () => ({
    getProperty: name => properties.get(name) || null,
    setProperties: values => Object.entries(values).forEach(([k,v]) => properties.set(k,v))
  }) },
  ContentService: { MimeType: {TEXT:'text'}, createTextOutput: text => ({setMimeType: () => text}) },
  LockService: {getScriptLock: () => ({ tryLock: () => !busy, releaseLock: () => released++ })},
  SpreadsheetApp: {openById: id => {
    assert.equal(id, 'test'); sheetReads++;
    return {getSheetByName: name => name === 'Sterowanie' ? sheet : log};
  }}
});
vm.runInContext(fs.readFileSync('cloud/google-apps-script/Code.gs', 'utf8'), context);
const post = overrides => context.doPost({ parameter: {
  token:'a'.repeat(64), device:'stm32f769-demo', temperature10:'255', relayAck:'0',
  nonce:'0'.repeat(32), ...overrides
}});
assert.equal(post({token:'wrong'}), 'ERROR:request');
assert.equal(post({device:'other'}), 'ERROR:request');
assert.equal(post({temperature10:'=BAD()'}), 'ERROR:request');
assert.equal(post({temperature10:'1501'}), 'ERROR:temperature');
assert.equal(post({temperature10:'-801'}), 'ERROR:temperature');
assert.equal(post({relayAck:'true'}), 'ERROR:request');
assert.equal(sheetReads, 0);
assert.equal(post({}), 'STM32CLOUD1:'+'0'.repeat(32)+':0');
assert.equal(rows.length, 1);
assert.equal(rows[0][2], 25.5);
desired = true;
assert.equal(post({}), 'STM32CLOUD1:'+'0'.repeat(32)+':0'); // idempotent retry
assert.equal(rows.length, 1);
assert.equal(post({nonce:'1'.repeat(32)}), 'STM32CLOUD1:'+'1'.repeat(32)+':1');
assert.equal(status[0], 0);
assert.equal(status[3], 'DEMO - oczekiwanie na potwierdzenie');
assert.equal(post({nonce:'2'.repeat(32), relayAck:'1'}), 'STM32CLOUD1:'+'2'.repeat(32)+':1');
assert.equal(status[0], 1);
assert.equal(status[3], 'DEMO - potwierdzony');
desired = 'TRUE';
assert.equal(post({nonce:'3'.repeat(32)}), 'ERROR:checkbox');
busy = true;
assert.equal(post({nonce:'4'.repeat(32)}), 'ERROR:busy');
busy = false; desired = false;
rows = Array.from({length:10000}, () => []);
assert.equal(post({nonce:'5'.repeat(32)}), 'STM32CLOUD1:'+'5'.repeat(32)+':0');
assert.equal(rows.length, 9001);
assert.equal(released, 6);
console.log('Apps Script authentication, temperature, checkbox, ACK, deduplication, locking and history limit: PASS');

assert.equal(post({temperature10:'-800', nonce:'8'.repeat(32)}), 'STM32CLOUD1:'+'8'.repeat(32)+':0');
assert.equal(rows[rows.length-1][2], -80);

// Dashboard uses a distinct browser key, keeps device POST contract intact.
properties.set('PANEL_KEY', 'panel-secret');
assert.throws(() => context.getPanelState('wrong'), /klucz/);
assert.throws(() => context.setPanelRelay('a'.repeat(64), true), /klucz/);
busy = false;
let browserDesired = false;
const date = vm.runInContext('new Date(1700000000000)', context);
sheet.getRange = range => ({
  getValues: () => [['stm32f769-demo', browserDesired, 0, date, 25.5, 'DEMO']],
  setValue: value => { assert.equal(range, 'B2'); browserDesired = value; }
});
log.getLastRow = () => 2;
log.getRange = () => ({getValues: () => [[date, 'stm32f769-demo', 25.5]]});
context.SpreadsheetApp.flush = () => {};
assert.throws(() => context.setPanelRelay('panel-secret', 'ON'), /stan/);
let panel = context.getPanelState('panel-secret');
assert.equal(panel.temperature, 25.5);
assert.equal(panel.ack, 0);
assert.equal(panel.history[0][1], 25.5);
panel = context.setPanelRelay('panel-secret', true);
assert.equal(panel.desired, true);
assert.equal(panel.ack, 0); // desired state must never fabricate device confirmation
busy = true;
assert.throws(() => context.setPanelRelay('panel-secret', false), /zajęty/);
assert.equal(browserDesired, true);
assert.ok(!JSON.stringify(panel).includes('panel-secret'));
assert.ok(!JSON.stringify(panel).includes('a'.repeat(64)));
console.log('Dashboard access key, history, relay command, acknowledgement separation and locking: PASS');
