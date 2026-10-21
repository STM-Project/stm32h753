/* Bound to a private Google Sheet. Deploy as Web app: execute as Me,
 * access Anyone. The device authenticates using a private token in POST body.
 * Do not publish/share the spreadsheet itself. One device per deployment.
 */
function setup() {
  const ss = SpreadsheetApp.getActiveSpreadsheet();
  if (!ss) throw new Error('Otworz Apps Script z menu Rozszerzenia w arkuszu.');
  const props = PropertiesService.getScriptProperties();
  // Do not rotate an existing device token when setup is run again.
  if (!props.getProperty('DEVICE_TOKEN')) {
    props.setProperty('DEVICE_TOKEN',
      (Utilities.getUuid() + Utilities.getUuid()).replace(/-/g, ''));
  }
  if (!props.getProperty('PANEL_KEY')) {
    props.setProperty('PANEL_KEY', (Utilities.getUuid() + Utilities.getUuid()).replace(/-/g, ''));
  }
  props.setProperty('SHEET_ID', ss.getId());
  if (!props.getProperty('DEVICE_ID')) props.setProperty('DEVICE_ID', 'stm32h753');
  let control = ss.getSheetByName('Sterowanie');
  if (!control) {
    control = ss.insertSheet('Sterowanie');
    control.getRange('A1:F1').setValues([[
      'Urzadzenie', 'Przekaznik zadany (DEMO)', 'Potwierdzony (DEMO)',
      'Ostatni kontakt', 'Temperatura [C]', 'Status'
    ]]);
    control.getRange('A2').setValue(props.getProperty('DEVICE_ID'));
    control.getRange('B2').insertCheckboxes().setValue(false);
    control.getRange('C2').setValue('brak potwierdzenia');
    control.getRange('F2').setValue('DEMO - bez GPIO');
    control.getRange('D2').setNumberFormat('yyyy-mm-dd hh:mm:ss');
    control.getRange('E2').setNumberFormat('0.0');
    control.setFrozenRows(1);
    control.autoResizeColumns(1, 6);
  }
  if (!ss.getSheetByName('Pomiary')) {
    const log = ss.insertSheet('Pomiary');
    log.appendRow(['Czas serwera', 'Urzadzenie', 'Temperatura [C]',
      'Zadany DEMO', 'Potwierdzony DEMO', 'Id pomiaru']);
    log.setFrozenRows(1);
  }
}

function text_(value) {
  return ContentService.createTextOutput(value).setMimeType(ContentService.MimeType.TEXT);
}

function doGet() {
  return HtmlService.createHtmlOutputFromFile('Panel')
    .setTitle('STM32 • Temperatura i przekaźnik')
    .addMetaTag('viewport', 'width=device-width, initial-scale=1');
}

// Separate browser key; never expose the firmware DEVICE_TOKEN to the page.
function panelSheet_(key) {
  const props = PropertiesService.getScriptProperties();
  const expected = props.getProperty('PANEL_KEY');
  if (!expected || typeof key !== 'string' || key !== expected)
    throw new Error('Nieprawidłowy klucz panelu.');
  return SpreadsheetApp.openById(props.getProperty('SHEET_ID'));
}

function getPanelState(key) {
  const ss = panelSheet_(key);
  const control = ss.getSheetByName('Sterowanie');
  const log = ss.getSheetByName('Pomiary');
  const v = control.getRange('A2:F2').getValues()[0];
  const count = Math.min(120, Math.max(0, log.getLastRow() - 1));
  const history = count ? log.getRange(log.getLastRow() - count + 1, 1, count, 3).getValues()
    .filter(r => r[0] instanceof Date && typeof r[2] === 'number' && Number.isFinite(r[2]))
    .map(r => [r[0].getTime(), r[2]]) : [];
  return {
    device: String(v[0]), desired: v[1] === true,
    ack: v[2] === 0 || v[2] === 1 ? v[2] : null,
    updated: v[3] instanceof Date ? v[3].getTime() : null,
    temperature: v[3] instanceof Date && typeof v[4] === 'number' && Number.isFinite(v[4]) ? v[4] : null,
    history: history
  };
}

function setPanelRelay(key, enabled) {
  const ss = panelSheet_(key);
  if (typeof enabled !== 'boolean') throw new Error('Nieprawidłowy stan.');
  const lock = LockService.getScriptLock();
  if (!lock.tryLock(5000)) throw new Error('Serwer zajęty. Spróbuj ponownie.');
  try {
    ss.getSheetByName('Sterowanie').getRange('B2').setValue(enabled);
    SpreadsheetApp.flush();
  } finally { lock.releaseLock(); }
  return getPanelState(key);
}

function doPost(e) {
  const p = e && e.parameter;
  const props = PropertiesService.getScriptProperties();
  const token = props.getProperty('DEVICE_TOKEN');
  const device = props.getProperty('DEVICE_ID');
  // All parameter validation precedes sheet access; never log request/token.
  if (!p || !token || !/^[0-9a-f]{64}$/i.test(token) || p.token !== token ||
      p.device !== device || !/^[A-Za-z0-9_-]{1,32}$/.test(p.device) ||
      !/^[0-9a-f]{32}$/.test(p.nonce || '') ||
      !/^-?\d{1,4}$/.test(p.temperature10 || '') ||
      !/^[01]$/.test(p.relayAck || '')) return text_('ERROR:request');
  const temp = Number(p.temperature10);
  if (!Number.isInteger(temp) || temp < -500 || temp > 1500) return text_('ERROR:temperature');
  const lock = LockService.getScriptLock();
  if (!lock.tryLock(5000)) return text_('ERROR:busy');
  try {
    // A repeated POST must not add a second row or change its response.
    if (props.getProperty('LAST_NONCE') === p.nonce) {
      return text_(props.getProperty('LAST_REPLY') || 'ERROR:retry');
    }
    const ss = SpreadsheetApp.openById(props.getProperty('SHEET_ID'));
    const control = ss.getSheetByName('Sterowanie');
    const log = ss.getSheetByName('Pomiary');
    if (!control || !log || control.getRange('A2').getValue() !== device)
      return text_('ERROR:sheet');
    const desiredValue = control.getRange('B2').getValue();
    if (typeof desiredValue !== 'boolean') return text_('ERROR:checkbox');
    const desired = desiredValue ? 1 : 0;
    const ack = Number(p.relayAck);
    const now = new Date();
    control.getRange('C2:F2').setValues([[
      ack, now, temp / 10,
      ack === desired ? 'DEMO - potwierdzony' : 'DEMO - oczekiwanie na potwierdzenie'
    ]]);
    // Bounded history: roughly 25-28 hours at one sample per 10 seconds.
    if (log.getLastRow() >= 10001) log.deleteRows(2, 1000);
    log.appendRow([now, device, temp / 10, desired, ack, p.nonce]);
    const reply = 'STM32CLOUD1:' + p.nonce + ':' + desired;
    props.setProperties({ LAST_NONCE: p.nonce, LAST_REPLY: reply });
    return text_(reply);
  } catch (error) {
    // No exception detail in a public endpoint response.
    return text_('ERROR:server');
  } finally {
    lock.releaseLock();
  }
}
