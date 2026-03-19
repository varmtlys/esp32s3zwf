#ifndef WEB_UI_H
#define WEB_UI_H

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 WorkFlow</title>
<style>
:root {
--bg: #0b0e14;
--panel: rgba(22, 27, 34, 0.8);
--accent: #ff6d00;
--text: #e6edf3;
--node-bg: #161b22;
--node-border: #30363d;
--trigger: #238636;
--action: #1f6feb;
--logic: #8957e5;
--table: #d2a8ff;
}
body {
margin: 0; background: var(--bg); color: var(--text);
font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Helvetica, Arial, sans-serif;
display: flex; height: 100vh; overflow: hidden;
}
.sidebar {
width: 250px; background: var(--panel); border-right: 1px solid var(--node-border);
padding: 20px; display: flex; flex-direction: column; gap: 10px; z-index: 10;
margin-top: 0; height: calc(100vh - 35px);
}
.sidebar h2 { font-size: 1.2rem; margin-top: 0; color: var(--accent); }
#canvas {
flex-grow: 1; position: relative; overflow: hidden;
background-image: radial-gradient(circle at 1px 1px, #30363d 1px, transparent 0);
background-size: 20px 20px; margin-top: 35px;
}
.node {
position: absolute; width: 160px; background: var(--node-bg);
border: 2px solid var(--node-border); border-radius: 10px;
padding: 10px; box-shadow: 0 8px 24px rgba(0,0,0,0.5);
transition: transform 0.1s, border-color 0.2s;
}
.node:hover { border-color: var(--accent); }
.node-header { display: flex; align-items: center; gap: 8px; font-weight: bold; margin-bottom: 5px; font-size: 0.85rem; }
.node-icon { width: 13px; height: 13px; border-radius: 50%; }
.node-label { font-size: 0.65rem; color: #8b949e; margin-top: 5px; text-transform: uppercase; letter-spacing: 0.05em; }
.port {
width: 10px; height: 10px; background: #8b949e; border-radius: 50%;
position: absolute; cursor: crosshair; z-index: 5;
}
.port:hover { background: var(--accent); transform: scale(1.2); }
.port.in { left: -6px; top: 14px; }
.port.out { right: -6px; top: 14px; }
.port-label { position: absolute; font-size: 0.6rem; color: #8b949e; top: -14px; left: 10px; }
svg { position: absolute; top: 0; left: 0; width: 100%; height: 100%; pointer-events: none; overflow: visible; }
path { stroke: #8b949e; stroke-width: 1.3; fill: none; opacity: 0.6; }
path.selected { stroke: var(--accent); opacity: 1; }
.config-overlay {
position: fixed; left: 50%; top: 50%; width: 350px; max-height: 85vh;
background: var(--bg); border: 1px solid var(--node-border); border-radius: 10px;
transform: translate(-50%, -50%) scale(0.9); opacity: 0; pointer-events: none;
transition: 0.2s; padding: 20px; z-index: 100; box-shadow: 0 10px 30px rgba(0,0,0,0.8);
display: flex; flex-direction: column; overflow-y: auto;
}
.config-overlay.open { transform: translate(-50%, -50%) scale(1); opacity: 1; pointer-events: all; }
label { display: block; font-size: 0.75rem; color: #8b949e; margin-bottom: 4px; }
input, select, textarea {
width: 100%; background: #0d1117; border: 1px solid var(--node-border);
color: white; padding: 8px; border-radius: 6px; margin-bottom: 12px; font-size: 0.85rem;
}
.actions { margin-top: auto; display: flex; flex-direction: column; gap: 8px; }
button {
padding: 10px; border-radius: 6px; border: none; cursor: pointer;
font-weight: bold; transition: 0.2s;
}
.btn-primary { background: var(--accent); color: white; }
.btn-secondary { background: #21262d; color: white; }
.btn-danger { background: #da3633; color: white; }
button:hover { filter: brightness(1.2); }
#console {
position: fixed; bottom: 0; left: 291px; right: 0; height: 100px;
background: rgba(0,0,0,0.9); border-top: 1px solid var(--node-border);
padding: 10px; font-family: monospace; font-size: 0.7rem;
overflow-y: auto; color: #7ee787; z-index: 5;
}
#wifi-warning {
position: fixed; top: 0; left: 250px; right: 0; background: #da3633;
color: white; padding: 10px; text-align: center; font-weight: bold;
z-index: 1000; display: none;
}
.node.executing {
border-color: #f1e05a !important;
box-shadow: 0 0 15px rgba(241, 224, 90, 0.5);
transform: scale(1.02);
}
.node.selected { border-color: var(--accent); outline: 2px solid var(--accent); }
.conn-del-btn {
width: 16px; height: 16px; background: #555555ff; color: white;
border-radius: 50%; font-size: 10px; line-height: 16px; text-align: center;
cursor: pointer; border: 1px solid rgba(255,255,255,0.2);
transition: 0.2s; pointer-events: all;
}
.conn-del-btn:hover { transform: scale(1.3); background: #ff7b72; }
#top-bar {
position: fixed; top: 0; left: 291px; right: 0; height: 35px;
background: #161b22; border-bottom: 1px solid var(--node-border);
display: flex; align-items: center; justify-content: flex-end; padding: 0 15px; z-index: 1001;
font-size: 0.65rem; gap: 4px;
}
.stat-group { display: flex; align-items: center; gap: 8px; border-right: 1px solid #30363d; padding: 0 8px; height: 18px; line-height: 18px; }
.stat-group:last-child { border-right: none; }
.stat-badge { background: transparent; padding: 0; color: #8b949e; display: flex; align-items: center; gap: 4px; }
.stat-badge span { color: var(--text); font-weight: 500; }
.node-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 6px; margin-bottom: 15px; }
.sidebar-item-mini {
padding: 10px 0; background: #21262d; border-radius: 6px; cursor: pointer;
border: 1px solid transparent; font-size: 0.72rem;
display: flex; justify-content: center; align-items: center;
width: 100%; box-sizing: border-box; overflow: hidden;
}
.sidebar-item-mini:hover { border-color: var(--accent); background: #30363d; }
.sidebar-item-mini i { font-style: normal; font-size: 1.1rem; display: block; flex-shrink: 0; width: 18px; text-align: center; }
.help-box { background: rgba(255,109,0,0.1); border-left: 3px solid var(--accent); padding: 8px; font-size: 0.75rem; margin-bottom: 12px; }
.flow-mgmt-item {
display: flex; align-items: center; justify-content: space-between;
padding: 8px; background: #21262d; border-radius: 6px; margin-bottom: 5px; font-size: 0.8rem;
}
.flow-mgmt-item span { flex: 1; cursor: pointer; }
.flow-mgmt-item .toggle { cursor: pointer; color: var(--accent); font-weight: bold; }
.flow-mgmt-item .toggle.off { color: #8b949e; }
.flow-mgmt-item.active { border: 1px solid var(--accent); background: #30363d; }
.node-clone-btn {
position: absolute; bottom: 5px; right: 5px;
width: 18px; height: 18px; background: #21262d;
border-radius: 4px; cursor: pointer; color: #8b949e;
font-size: 10px; line-height: 18px; text-align: center;
border: 1px solid #30363d; z-index: 10;
}
.node-clone-btn:hover { background: var(--accent); color: white; border-color: var(--accent); }
.sidebar button { font-family: 'Segoe UI', system-ui, sans-serif; text-transform: none; letter-spacing: 0; font-size: 0.8rem; }
</style>
</head>
<body>
<div class="sidebar">
<h2>FlowEditor</h2>
<div class="node-grid">
<div class="sidebar-item-mini" onclick="addNode('start')" title="Start Trigger"><i>◈</i></div>
<div class="sidebar-item-mini" onclick="addNode('http')" title="HTTP Request"><i>⇄</i></div>
<div class="sidebar-item-mini" onclick="addNode('condition')" title="If Condition"><i>⎇</i></div>
<div class="sidebar-item-mini" onclick="addNode('delay')" title="Delay Timer"><i>⏱</i></div>
<div class="sidebar-item-mini" onclick="addNode('transform')" title="Transform Script"><i>ƒ</i></div>
<div class="sidebar-item-mini" onclick="addNode('loop')" title="Loop Block"><i>⟳</i></div>
<div class="sidebar-item-mini" onclick="addNode('telegram')" title="Telegram Bot"><i>✉</i></div>
<div class="sidebar-item-mini" onclick="addNode('read_table')" title="Read Table" style="color:var(--table)"><i>⇱</i></div>
<div class="sidebar-item-mini" onclick="addNode('write_table')" title="Write Table" style="color:var(--table)"><i>⇲</i></div>
<div class="sidebar-item-mini" onclick="addNode('loop_array')" title="Loop Array Data" style="color:var(--table)"><i>⊟</i></div>
<div class="sidebar-item-mini" onclick="addNode('break_loop')" title="Break Loop" style="color:#e34c26"><i>⎋</i></div>
</div>
<hr style="border:0; border-top:1px solid var(--node-border); margin:10px 0">
<label style="color:var(--accent); font-weight:bold">Flow Control: <span id="current-flow-display" style="color:white"></span></label>
<div style="display:grid; grid-template-columns: repeat(4, 1fr); gap:5px; margin-top:5px">
<button class="btn-primary" style="padding:10px 0" onclick="saveFlow()" title="Save Flow">💾</button>
<button class="btn-secondary" style="background:#238636; padding:10px 0" onclick="runFlow()" title="Run Flow">▶</button>
<button class="btn-danger" style="padding:10px 0; background:#da3633" onclick="stopFlow()" title="Stop Flow">⏹</button>
<button class="btn-danger" style="padding:10px 0" onclick="deleteCurrentFlow()" title="Delete Flow">🗑</button>
</div>
<hr style="border:0; border-top:1px solid var(--node-border); margin:10px 0">
<label style="color:var(--accent); font-weight:bold">Manage Flows</label>
<div style="display:flex; gap:5px; margin-bottom:10px">
<input id="new-flow-name" placeholder="New Flow Name" style="margin:0; font-size:0.7rem; flex:1">
<button class="btn-primary" onclick="createFlow()" style="padding:5px 10px">+</button>
</div>
<div id="flow-mgmt-list" style="overflow-y:auto; max-height:250px"></div>
<hr style="border:0; border-top:1px solid var(--node-border); margin:10px 0">
<button class="btn-secondary" style="background:#4f46e5" onclick="openWifiConfig()">⚙  WiFi</button>
<button class="btn-secondary" style="background:#0e7c42; margin-top:5px" onclick="openOtaPanel()">🜁 Update</button>
<button class="btn-secondary" style="margin-top:5px" onclick="openTunnelPanel()">↩ Webhook Tunnel</button>
<button class="btn-secondary" style="margin-top:5px; background:var(--table); color:#000" onclick="openTablesPanel()">🗃️ Tables</button>
</div>
<div id="top-bar">
<div class="stat-group" title="CPU and Temperature">
<div class="stat-badge" title="CPU Clock Frequency"><span id="stat-cpu">-</span>MHz</div>
<div class="stat-badge" title="ESP32 Internal Temperature"><span id="stat-temp">-</span>°C</div>
</div>
<div class="stat-group" title="WiFi Connection Details">
<div class="stat-badge" title="SSID and Signal Strength"><span id="stat-ssid">-</span> (<span id="stat-rssi">-</span>dBm)</div>
<div class="stat-badge" title="Local IP Address"><span id="stat-ip">-</span></div>
</div>
<div class="stat-group" title="Memory and Storage Status">
<div class="stat-badge" title="Free Internal SRAM"><span id="stat-heap">-</span>KB</div>
<div id="psram-badge" class="stat-badge" title="Free PSRAM" style="display:none"><span id="stat-psram">-</span>KB</div>
<div class="stat-badge" title="FS Free / Total Space"><span id="stat-fs">-/-</span>KB</div>
<div class="stat-badge" title="Total Flash Size"><span id="stat-flash">-</span>MB</div>
</div>
<div class="stat-group" title="System Stability and Info">
<div class="stat-badge" title="ESP-IDF SDK Version"><span id="stat-sdk">-</span></div>
<div class="stat-badge" title="Boot Reset Reason"><span id="stat-reset">-</span></div>
<div class="stat-badge" title="Total System Uptime"><span id="stat-uptime">-</span>s</div>
</div>
</div>
<div id="wifi-warning">⚠️ Running in AP Mode. Please <a href="#" onclick="openWifiConfig()" style="color:white; text-decoration:underline">configure WiFi</a> to enable Internet access.</div>
<div id="canvas">
<div id="canvas-inner" style="position:absolute; top:0; left:0; width:0; height:0; overflow:visible">
<svg id="svg-connections" style="overflow:visible"></svg>
</div>
</div>
<div id="console">System initialized. Ready to flow.</div>
<div id="config-panel" class="config-overlay">
<h3 id="cfg-node-title">Node Config</h3>
<div id="node-help" class="help-box"></div>
<div id="cfg-fields"></div>
<div class="actions">
<button class="btn-primary" onclick="closeConfig()">Apply Changes</button>
<button class="btn-danger" onclick="deleteNode()">Delete Node</button>
<button class="btn-secondary" onclick="document.getElementById('config-panel').classList.remove('open')">Cancel</button>
</div>
</div>
<div id="wifi-panel" class="config-overlay" style="z-index:101">
<h3>WiFi Settings</h3>
<label>SSID</label>
<input id="wifi-ssid" placeholder="Network Name">
<label>Password</label>
<input id="wifi-pass" type="password" placeholder="Password">
<div class="actions">
<button class="btn-primary" onclick="saveWifi()">Save & Reboot</button>
<button class="btn-secondary" onclick="document.getElementById('wifi-panel').classList.remove('open')">Cancel</button>
</div>
</div>
<div id="ota-panel" class="config-overlay" style="z-index:101">
<h3>Firmware Update</h3>
<div class="help-box">
<strong>OTA Update</strong><br>
Current version: <strong id="ota-current-ver">-</strong><br>
Set the URL base where <code>version.json</code> and <code>firmware.bin</code> are hosted.
</div>
<label>Update Server URL</label>
<input id="ota-url" placeholder="http://example.com/firmware/">
<button class="btn-primary" style="margin-bottom:8px" onclick="saveOtaUrl()">Save URL</button>
<button class="btn-secondary" style="background:#0e7c42; margin-bottom:8px" onclick="checkOta()">Check for Updates</button>
<div id="ota-status" style="font-size:0.8rem; color:#8b949e; margin-bottom:10px"></div>
<div id="ota-update-section" style="display:none">
<div style="background:rgba(35,134,54,0.2); border:1px solid #238636; border-radius:6px; padding:10px; margin-bottom:10px; font-size:0.8rem">
New version available: <strong id="ota-remote-ver"></strong>
</div>
<button class="btn-danger" style="background:#238636" id="ota-update-btn" onclick="performOta()">Update Now</button>
</div>
<div class="actions">
<button class="btn-secondary" onclick="document.getElementById('ota-panel').classList.remove('open')">Close</button>
</div>
</div>
</div>
</div>

<div id="tables-panel" class="config-overlay">
<h2 style="margin-top:0">🗃️ Manage Tables</h2>
<div style="display:flex; gap:5px; margin-bottom:15px">
<input id="new-table-name" placeholder="New Table Name" style="margin:0; font-size:0.8rem; flex:1">
<button class="btn-primary" onclick="createTable()" style="padding:8px">+</button>
</div>
<div id="tables-list" style="overflow-y:auto; max-height:200px; margin-bottom:15px; border:1px solid var(--node-border); border-radius:6px; padding:5px"></div>
<h3 style="font-size:0.9rem; margin-top:10px">Table Data (JSON)</h3>
<textarea id="table-data-view" rows="8" readonly style="width:100%; background:#000; color:#7ee787; font-family:monospace; margin-bottom:10px; font-size:0.75rem"></textarea>
<div class="actions">
<button class="btn-secondary" onclick="document.getElementById('tables-panel').classList.remove('open')">Close</button>
</div>
</div>

<div id="tunnel-panel" class="config-overlay" style="z-index:101; width:500px; max-height:80vh; overflow-y:auto">
<h3>Webhook Tunnel</h3>
<div class="help-box">
Proxies incoming HTTP requests from a public relay server.
</div>
<label><input type="checkbox" id="tunnel-enabled" style="width:auto; margin-bottom:10px"> Enable Tunnel Connection</label>
<label>Protocol</label>
<select id="tunnel-protocol">
<option value="http">http://</option>
<option value="https">https://</option>
</select>
<label>Relay Host (e.g. domain.com)</label>
<input id="tunnel-host" placeholder="domain.com">
<label>Relay Port (80, 443, or custom)</label>
<input id="tunnel-port" type="number" placeholder="80">
<div class="actions">
<button class="btn-primary" onclick="saveTunnelConfig()">Save &amp; Apply</button>
<button class="btn-secondary" onclick="document.getElementById('tunnel-panel').classList.remove('open')">Cancel</button>
</div>
</div>
</div>
<script>
function isAnyModalOpen() {
  return document.querySelector('.config-overlay.open') !== null;
}
// --- Tables Management ---
async function openTablesPanel() {
if (isAnyModalOpen()) return;
document.getElementById('tables-panel').classList.add('open');
await loadTablesList();
}
async function loadTablesList() {
try {
const res = await fetch('/api/tables');
const list = await res.json();
let html = '';
list.forEach(t => {
html += `<div class="flow-mgmt-item">
<span onclick="viewTableData('${t.name}')">🗃️ ${t.name} <small>(${(t.size/1024).toFixed(1)}kb)</small></span>
<div style="display:flex; gap:5px">
<button class="btn-danger" style="padding:4px 8px; font-size:0.7rem; background:#da3633" onclick="deleteTable('${t.name}')">Del</button>
</div></div>`;
});
document.getElementById('tables-list').innerHTML = html || '<div style="color:#8b949e; font-size:0.8rem; text-align:center">No tables</div>';
document.getElementById('table-data-view').value = '';
} catch(e) { log("Error loading tables"); }
}
async function createTable() {
const val = document.getElementById('new-table-name').value.trim();
if (!val) return;
await fetch(`/api/table?name=${val}`, { method: 'POST' });
document.getElementById('new-table-name').value = '';
await loadTablesList();
}
async function deleteTable(name) {
if (!confirm(`Delete table ${name}? All data will be lost.`)) return;
await fetch(`/api/table?name=${name}`, { method: 'DELETE' });
await loadTablesList();
}
async function viewTableData(name) {
try {
const res = await fetch(`/api/table/data?name=${name}`);
const data = await res.json();
document.getElementById('table-data-view').value = JSON.stringify(data, null, 2);
} catch(e) { document.getElementById('table-data-view').value = "Error reading data."; }
}
let flowList = [];
let currentFlow = "default";
let wifiData = { ssid: '', pass: '' };
let nodes = [];
let connections = [];
let selectedNodeId = null;
let dragInfo = { active: false, node: null, offset: { x: 0, y: 0 } };
let connInfo = { active: false, from: null, fromPort: null };
let panX = 0, panY = 0;
let isPanning = false, startPanX = 0, startPanY = 0;
function updatePan() {
const inner = document.getElementById('canvas-inner');
if (inner) inner.style.transform = `translate(${panX}px, ${panY}px)`;
document.getElementById('canvas').style.backgroundPosition = `${panX}px ${panY}px`;
}
window.onmousedown = (e) => {
  if (e.target.id === 'canvas' || e.target.id === 'svg-connections' || e.target.id === 'canvas-inner') {
    isPanning = true;
    startPanX = e.clientX - panX;
    startPanY = e.clientY - panY;
    if (!document.getElementById('config-panel').classList.contains('open')) {
      document.querySelectorAll('.node').forEach(n => n.classList.remove('selected'));
      selectedNodeId = null;
    }
  }
};
async function init() {
try {
const wifiStatus = await fetch('/api/status');
const status = await wifiStatus.json();
wifiData = { ssid: status.ssid, pass: status.pass };
if(status.apMode) {
document.getElementById('wifi-warning').style.display = 'block';
}
await refreshFlowList();
await openFlow(currentFlow);
initWebSocket();
} catch(e) { log("Initialization failed."); }
}
let ws;
function initWebSocket() {
ws = new WebSocket(`ws://${window.location.host}:81`);
ws.onmessage = (e) => {
try {
const data = JSON.parse(e.data);
if (data.event === 'stats') {
document.getElementById('stat-heap').innerText = (data.heap/1024).toFixed(1);
if (data.psram > 0) {
document.getElementById('psram-badge').style.display = 'flex';
document.getElementById('stat-psram').innerText = (data.psram/1024).toFixed(0);
}
document.getElementById('stat-uptime').innerText = data.uptime;
document.getElementById('stat-ip').innerText = data.ip;
document.getElementById('stat-ssid').innerText = data.ssid;
document.getElementById('stat-rssi').innerText = data.rssi;
document.getElementById('stat-cpu').innerText = data.cpu;
document.getElementById('stat-flash').innerText = data.flash;
if (data.fs_total) {
  const freeKb = ((data.fs_total - data.fs_used) / 1024).toFixed(0);
  const totalKb = (data.fs_total / 1024).toFixed(0);
  document.getElementById('stat-fs').innerText = `${freeKb}/${totalKb}`;
}
document.getElementById('stat-sdk').innerText = data.sdk;
document.getElementById('stat-reset').innerText = data.reset;
if(data.temp) document.getElementById('stat-temp').innerText = data.temp;
}
if (data.event === 'executing') {
document.querySelectorAll('.node').forEach(n => n.classList.remove('executing'));
const el = document.querySelector(`.node[data-id="${data.nodeId}"]`);
if (el) {
el.classList.add('executing');
if (data.current) {
let badge = el.querySelector('.loop-counter');
if (badge) badge.innerText = `${data.current}/${data.total}`;
}
}
}
if (data.event === 'finished' || data.event === 'stopped') {
if(data.event === 'stopped') log("⚠️ Flow execution stopped by user.");
else log("✅ Flow execution finished.");
setTimeout(() => {
document.querySelectorAll('.node').forEach(n => n.classList.remove('executing'));
document.querySelectorAll('.loop-counter').forEach(el => el.innerText = '');
}, 500);
}
if (data.event === 'ota') {
  const statusEl = document.getElementById('ota-status');
  if (data.status === 'downloading') statusEl.innerText = '⬇️ Downloading firmware...';
  else if (data.status === 'success') statusEl.innerText = '✅ Update successful! Rebooting...';
}
if (data.event === 'log') {
  log(data.msg);
}
} catch(err) { /* ignore malformed messages */ }
};
ws.onerror = () => {};
ws.onclose = () => setTimeout(initWebSocket, 2000);
}
async function refreshFlowList() {
const res = await fetch('/api/flows');
const data = await res.json();
flowList = data;
const display = document.getElementById('current-flow-display');
if(display) display.innerText = currentFlow;
const list = document.getElementById('flow-mgmt-list');
if(list) {
list.innerHTML = flowList.map(f => {
const safe = escapeHtml(f.name);
return `
<div class="flow-mgmt-item ${f.name.trim() === currentFlow.trim() ? 'active' : ''}">
<span onclick="openFlow('${safe}')">${safe}</span>
<div class="toggle ${f.enabled?'':'off'}" onclick="toggleFlowEnabled('${safe}', ${!f.enabled})">
${f.enabled?'ON':'OFF'}
</div>
</div>`;
}).join('');
}
if(!flowList.find(f => f.name === currentFlow) && flowList.length > 0) currentFlow = flowList[0].name;
}
async function toggleFlowEnabled(name, status) {
await fetch(`/api/flow/status?name=${name}&enabled=${status}`, { method: 'POST' });
log(`Flow ${name} ${status ? 'enabled' : 'disabled'}`);
refreshFlowList();
}
async function openFlow(name) {
currentFlow = name;
log(`Opening flow: ${name}`);
refreshFlowList(); // Update UI highlighting immediately
try {
const res = await fetch(`/api/flow?name=${name}`);
const data = await res.json();
nodes = data.nodes || [];
connections = data.connections || [];
if(data.pan) { panX = data.pan.x || 0; panY = data.pan.y || 0; } else { panX = 0; panY = 0; }
updatePan();
nodes.forEach(n => {
  if (n.type === 'start' && n.config) {
    if (!n.config.schedType) n.config.schedType = 'period';
    if (!n.config.cron) n.config.cron = '* * * * *';
  }
});
render();
} catch(e) {
nodes = []; connections = []; panX = 0; panY = 0; updatePan(); render();
log("Flow is empty or new.");
}
}
async function createFlow() {
const name = document.getElementById('new-flow-name').value.trim();
if(!name) return;
currentFlow = name;
nodes = []; connections = [];
document.getElementById('new-flow-name').value = '';
await saveFlow();
await refreshFlowList();
render();
}
function log(msg) {
const c = document.getElementById('console');
c.innerHTML += `<div>[${new Date().toLocaleTimeString()}] ${msg}</div>`;
while (c.children.length > 100) c.removeChild(c.firstChild);
c.scrollTop = c.scrollHeight;
}
function escapeHtml(s) {
return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');
}
function duplicateNode(e, id) {
e.stopPropagation();
const node = nodes.find(n => n.id === id);
if (!node || node.type === 'start') return;
// Generate a unique ID
const newId = node.type + '_' + Math.random().toString(36).substr(2, 5);
// Deep copy the node configuration
const newNode = {
id: newId, type: node.type,
x: Number(node.x) + 40, y: Number(node.y) + 40,
config: JSON.parse(JSON.stringify(node.config))
};
nodes.push(newNode);
selectedNodeId = newId;
saveFlow();
render();
// Small delay to ensure render is complete
setTimeout(() => openConfig(newNode), 50);
log(`Duplicated ${id} to ${newId}`);
}
function addNode(type) {
if (type === 'start' && nodes.some(n => n.type === 'start')) {
log("⚠️ Only one Start node allowed per flow.");
return;
}
const id = type + '_' + Math.random().toString(36).substr(2, 5);
nodes.push({
id, type,
x: Math.round((50 + Math.random()*100) / 20) * 20,
y: Math.round((50 + Math.random()*100) / 20) * 20,
config: getDefaultConfig(type)
});
render();
log(`Added node: ${id}`);
}
function getDefaultConfig(type) {
if (type === 'start') return { mode: 'manual', schedType: 'period', interval: 60, cron: '* * * * *', enabled: false, immediate: false, anchor: 'start', last_run: 0, last_finished: 0, path: '' };
if (type === 'http') return { url: 'http://', method: 'GET', headers: '', body: '' };
if (type === 'delay') return { ms: 1000 };
if (type === 'condition') return { value1: '{{node_id.key}}', op: '==', value2: 'value' };
if (type === 'transform') return { script: 'SET output = {{http_1.status}} + 10' };
if (type === 'telegram') return { token: 'TOKEN', method: 'sendMessage', chat_id: 'ID', text: 'Hello' };
if (type === 'loop_array') return { array: '{{read_table_1.data}}' };
if (type === 'break_loop') return { loop_id: '' };
if (type === 'webhook') return { url: '' };
return {};
}
function getNodeHelp(type) {
const h = {
start: '◈ <b>Start</b> — Manual, Schedule or Webhook. One per flow.',
http: '⇄ <b>HTTP</b> — GET/POST. Use <code>{{id.key}}</code> in URL/Body. Result: <code>data</code>, <code>status</code>.',
condition: '⎇ <b>Condition</b> — Compare values (==, !=, >). Routes to 1/0.',
delay: '⏱ <b>Delay</b> — Pause in ms (1000=1s).',
transform: 'ƒ <b>Transform</b> — <code>SET key = {{var}} + 10</code>. Math: + - * /',
loop: '⟳ <b>Loop</b> — Repeat N times. Index: <code>{{id.index}}</code>. Ports: ⟳/🞪.',
telegram: '✉ <b>Telegram</b> — sendMessage/sendPhoto via Bot API. Use <code>{{id.key}}</code>.',
read_table: '⇱ <b>Read Table</b> — Read a JSON table from local storage. Output: <code>data</code> (array), <code>count</code>.',
write_table: '⇲ <b>Write Table</b> — Append or overwrite a row in a table. Use JSON for <code>data</code>.',
loop_array: '⊟ <b>Loop Array</b> — Iterate over an array (e.g. <code>{{read_table_1.data}}</code>). Output: <code>item</code>, <code>index</code>. Ports: ⟳/🞪.',
break_loop: '⎋ <b>Break Loop</b> — Interrupts the specified loop and continues from its 🞪 port.',
webhook: '↩ <b>Webhook Tunnel</b> — Secure reverse tunnel for receiving external triggers. Supports custom paths, HTTP/HTTPS, and automatic domain mapping.'
};
return h[type] || '';
}

function render() {
const canvas = document.getElementById('canvas-inner');
const svg = document.getElementById('svg-connections');
document.querySelectorAll('.node').forEach(n => n.remove());
svg.innerHTML = '';
nodes.forEach(node => {
const el = document.createElement('div');
el.className = 'node';
el.setAttribute('data-id', node.id);
el.style.left = node.x + 'px';
el.style.top = node.y + 'px';
let color = 'var(--trigger)';
if(['http', 'delay', 'transform'].includes(node.type)) color = 'var(--action)';
if(node.type === 'condition' || node.type === 'loop' || node.type === 'loop_array') color = 'var(--logic)';
if(node.type === 'start') color = 'var(--trigger)';
if(node.type === 'telegram') color = '#0088cc';
if(['read_table', 'write_table'].includes(node.type)) color = 'var(--table)';
if(node.type === 'break_loop') color = '#e34c26';
let icon = '◈';
if(node.type==='http') icon='⇄';
if(node.type==='condition') icon='⎇';
if(node.type==='delay') icon='⏱';
if(node.type==='transform') icon='ƒ';
if(node.type==='loop') icon='⟳';
if(node.type==='telegram') icon='✉';
if(node.type==='read_table') icon='⇲';
if(node.type==='write_table') icon='⇱';
if(node.type==='loop_array') icon='⊟';
if(node.type==='break_loop') icon='⎋';
el.innerHTML = `
<div class="node-header">
<div class="node-icon" style="background:${color}"></div>
<span>${node.id}</span>
</div>
${node.type !== 'start' ? `<div class="node-clone-btn" onmousedown="duplicateNode(event, '${node.id}')" title="Clone Node">⧉</div>` : ''}
`;
if (node.type === 'start') {
let info = node.config.mode.toUpperCase();
if (node.config.mode === 'schedule') info += `: ${node.config.interval}s`;
el.innerHTML += `<div class="node-label">${info}</div>`;
el.innerHTML += `<div class="port out" onmousedown="startConnect(event, '${node.id}', 'out')"></div>`;
}
else if (node.type === 'condition') {
el.innerHTML += `<div class="port in" onmouseup="endConnect('${node.id}', 'in')"></div>`;
el.innerHTML += `
<div class="port out" style="top:14px" onmousedown="startConnect(event, '${node.id}', 'true')">
<span class="port-label">1</span>
</div>
<div class="port out" style="top:33px" onmousedown="startConnect(event, '${node.id}', 'false')">
<span class="port-label">0</span>
</div>
`;
} else if (node.type === 'loop') {
el.innerHTML += `<div class="node-label">COUNT: ${node.config.count}</div>`;
el.innerHTML += `<div class="port in" onmouseup="endConnect('${node.id}', 'in')"></div>`;
el.innerHTML += `
<div class="port out" style="top:14px" onmousedown="startConnect(event, '${node.id}', 'body')">
<span class="port-label">⟳</span>
</div>
<div class="port out" style="top:33px" onmousedown="startConnect(event, '${node.id}', 'done')">
<span class="port-label">🞪</span>
</div>
<div style="font-size:0.6rem; color:var(--accent); text-align:center; font-weight:bold" class="loop-counter"></div>
`;
} else if (node.type === 'loop_array') {
el.innerHTML += `<div class="node-label">ARRAY LOOP</div>`;
el.innerHTML += `<div class="port in" onmouseup="endConnect('${node.id}', 'in')"></div>`;
el.innerHTML += `
<div class="port out" style="top:14px" onmousedown="startConnect(event, '${node.id}', 'body')">
<span class="port-label">⟳</span>
</div>
<div class="port out" style="top:33px" onmousedown="startConnect(event, '${node.id}', 'done')">
<span class="port-label">🞪</span>
</div>
<div style="font-size:0.6rem; color:var(--accent); text-align:center; font-weight:bold" class="loop-counter"></div>
`;
} else if (node.type === 'telegram') {
el.innerHTML += `<div class="node-label">${node.config.method}</div>`;
el.innerHTML += `<div class="port in" onmouseup="endConnect('${node.id}', 'in')"></div>`;
el.innerHTML += `<div class="port out" onmousedown="startConnect(event, '${node.id}', 'out')"></div>`;
} else if (node.type === 'delay') {
el.innerHTML += `<div class="node-label">WAIT: ${node.config.ms}ms</div>`;
el.innerHTML += `<div class="port in" onmouseup="endConnect('${node.id}', 'in')"></div>`;
el.innerHTML += `<div class="port out" onmousedown="startConnect(event, '${node.id}', 'out')"></div>`;
} else {
el.innerHTML += `<div class="port in" onmouseup="endConnect('${node.id}', 'in')"></div>`;
el.innerHTML += `<div class="port out" onmousedown="startConnect(event, '${node.id}', 'out')"></div>`;
}
if (node.id === selectedNodeId) el.classList.add('selected');
el.onmousedown = (e) => {
if(e.target.closest('.port') || e.target.closest('.node-clone-btn')) return;
document.querySelectorAll('.node').forEach(n => n.classList.remove('selected'));
el.classList.add('selected');
selectedNodeId = node.id;
dragInfo = { active: true, node, offset: { x: e.clientX - node.x - panX, y: e.clientY - node.y - panY } };
};
el.onclick = (e) => {
if(e.target.closest('.port') || e.target.closest('.node-clone-btn')) return;
e.stopPropagation();
openConfig(node);
};
canvas.appendChild(el);
});
connections.forEach(conn => drawLine(conn));
}
function drawLine(conn) {
const from = nodes.find(n => n.id === conn.from);
const to = nodes.find(n => n.id === conn.to);
if (!from || !to) return;
const svg = document.getElementById('svg-connections');
const path = document.createElementNS('http://www.w3.org/2000/svg', 'path');
let yOffsetFrom = 14; 
if (from.type === 'condition' || from.type === 'loop' || from.type === 'loop_array') {
yOffsetFrom = (conn.fromPort === 'true' || conn.fromPort === 'body') ? 14 : 33;
}
const x1 = from.x + 185, y1 = from.y + yOffsetFrom + 4 + 3; // adjusted center point based on node width and port dimension
const x2 = to.x + 6 + 6, y2 = to.y + 14 + 4 + 3;
const cp = Math.max(Math.abs(x2 - x1) * 0.7, 50);
path.setAttribute('d', `M ${x1} ${y1} C ${x1 + cp} ${y1}, ${x2 - cp} ${y2}, ${x2} ${y2}`);
svg.appendChild(path);
// Add delete button at midpoint
const mx = (x1 + x2) / 2;
const my = (y1 + y2) / 2;
const fobj = document.createElementNS('http://www.w3.org/2000/svg', 'foreignObject');
fobj.setAttribute('x', mx - 8);
fobj.setAttribute('y', my - 8);
fobj.setAttribute('width', 16);
fobj.setAttribute('height', 16);
const btn = document.createElement('div');
btn.className = 'conn-del-btn';
btn.innerHTML = '×';
btn.onclick = () => deleteConnection(nodes.indexOf(from), nodes.indexOf(to), conn);
fobj.appendChild(btn);
svg.appendChild(fobj);
}
function deleteConnection(fIdx, tIdx, conn) {
connections = connections.filter(c => c !== conn);
render();
log("Connection removed.");
}
window.onmousemove = (e) => {
if (isPanning) {
panX = e.clientX - startPanX;
panY = e.clientY - startPanY;
updatePan();
} else if (dragInfo.active) {
const newX = e.clientX - dragInfo.offset.x - panX;
const newY = e.clientY - dragInfo.offset.y - panY;
dragInfo.node.x = Math.round(newX / 20) * 20;
dragInfo.node.y = Math.round(newY / 20) * 20;
render();
}
};
window.onmouseup = () => { 
  if(isPanning) { isPanning = false; saveFlow(); }
  dragInfo.active = false; 
};
window.onkeydown = (e) => {
if(e.key === "Delete" && selectedNodeId && document.activeElement.tagName !== "INPUT" && document.activeElement.tagName !== "TEXTAREA") {
deleteNode();
}
};
function startConnect(e, id, port) {
e.stopPropagation();
connInfo = { active: true, from: id, fromPort: port };
}
function endConnect(id, port) {
if (connInfo.active && connInfo.from !== id) {
const dup = connections.some(c => c.from === connInfo.from && c.fromPort === connInfo.fromPort && c.to === id);
if (!dup) {
connections.push({ from: connInfo.from, fromPort: connInfo.fromPort, to: id });
render();
}
connInfo.active = false;
}
}
function openConfig(node, force = false) {
if (!force && isAnyModalOpen()) return;
selectedNodeId = node.id;
const panel = document.getElementById('config-panel');
const fields = document.getElementById('cfg-fields');
const help = document.getElementById('node-help');
panel.classList.add('open');
document.getElementById('cfg-node-title').innerText = `Config: ${node.id}`;
help.innerHTML = getNodeHelp(node.type);
let html = '';
for (let key in node.config) {
if (key === 'mode' && node.type === 'start') {
html += `<label>Mode</label><select id="cfg-${key}" onchange="updateStartMode('${node.id}', this.value)">
<option value="manual" ${node.config[key]=='manual'?'selected':''}>Manual</option>
<option value="schedule" ${node.config[key]=='schedule'?'selected':''}>Schedule</option>
<option value="webhook" ${node.config[key]=='webhook'?'selected':''}>Webhook</option>
</select>`;
if (node.config.mode === 'webhook') {
  const p = node.config.path || node.id;
  const pub = `${tunnelProtocolGlobal}://${tunnelHostGlobal}/webhook/${p}`;
  html += `<label>Webhook URL (from tunnel settings)</label><input value="${pub}" readonly style="background:rgba(35,134,54,0.2); color:#7ee787; border-color:#238636; font-size:0.75rem">`;
}
} else if (key === 'path' && node.type === 'start') {
  if (node.config.mode === 'webhook') {
    html += `<label>Webhook Path (optional)</label><input id="cfg-${key}" value="${node.config[key]}" placeholder="e.g. my-trigger (default is node ID)">`;
  } else {
    continue;
  }
} else if (key === 'op') {
html += `<label>Operator</label><select id="cfg-${key}">
<option value="==" ${node.config[key]=='=='?'selected':''}>Equals</option>
<option value="!=" ${node.config[key]=='!='?'selected':''}>Not Equals</option>
<option value=">" ${node.config[key]=='>'?'selected':''}>Greater Than</option>
</select>`;
} else if (key === 'method') {
if (node.type === 'telegram') {
html += `<label>Method</label><select id="cfg-${key}" onchange="updateTelegramFields('${node.id}', this.value)">
<option value="sendMessage" ${node.config[key]=='sendMessage'?'selected':''}>sendMessage</option>
<option value="sendPhoto" ${node.config[key]=='sendPhoto'?'selected':''}>sendPhoto</option>
</select>`;
} else {
html += `<label>Method</label><select id="cfg-${key}">
<option value="GET" ${node.config[key]=='GET'?'selected':''}>GET</option>
<option value="POST" ${node.config[key]=='POST'?'selected':''}>POST</option>
<option value="PUT" ${node.config[key]=='PUT'?'selected':''}>PUT</option>
<option value="PATCH" ${node.config[key]=='PATCH'?'selected':''}>PATCH</option>
<option value="DELETE" ${node.config[key]=='DELETE'?'selected':''}>DELETE</option>
</select>`;
}

} else if (node.type === 'read_table' && key === 'table') {
html += `<label>Table Name</label><input id="cfg-${key}" value="${node.config[key]}" placeholder="e.g. users">`;
} else if (node.type === 'write_table') {
if (key === 'table') {
html += `<label>Table Name</label><input id="cfg-${key}" value="${node.config[key]}" placeholder="e.g. users">`;
} else if (key === 'write_mode') {
html += `<label>Write Mode</label><select id="cfg-${key}">
<option value="append" ${node.config[key]=='append'?'selected':''}>Append Row</option>
<option value="overwrite" ${node.config[key]=='overwrite'?'selected':''}>Overwrite All Data</option>
</select>`;
} else if (key === 'data') {
html += `<label>JSON Data to Write</label><textarea id="cfg-${key}" rows="3" style="width:100%; background:#0d1117; color:white; border:1px solid var(--node-border); border-radius:6px; font-family:monospace">${node.config[key]}</textarea>`;
}
} else if (node.type === 'loop_array' && key === 'array') {
html += `<label>Array Variable (e.g. {{read_table_1.data}})</label><input id="cfg-${key}" value="${node.config[key]}">`;
} else if (node.type === 'break_loop' && key === 'loop_id') {
let loopNodes = nodes.filter(n => n.type === 'loop' || n.type === 'loop_array');
let opts = '<option value="">-- None (Skip) --</option>';
loopNodes.forEach(ln => {
    opts += `<option value="${ln.id}" ${node.config[key] === ln.id ? 'selected' : ''}>${ln.id}</option>`;
});
html += `<label>Target Loop to Break</label><select id="cfg-${key}">${opts}</select>`;
} else if (node.type === 'http' && key === 'headers') {
html += `<label>Headers (Key: Value)</label><textarea id="cfg-${key}" rows="2" style="width:100%; background:#0d1117; color:white; border:1px solid var(--node-border); border-radius:6px; font-family:monospace">${node.config[key]}</textarea>`;
} else if (node.type === 'http' && key === 'body') {
html += `<label>Body</label><textarea id="cfg-${key}" rows="3" style="width:100%; background:#0d1117; color:white; border:1px solid var(--node-border); border-radius:6px; font-family:monospace">${node.config[key]}</textarea>`;
} else if (node.type === 'transform' && key === 'script') {
html += `<label>Script Instructions (e.g. SET temp = 25)</label><textarea id="cfg-${key}" rows="5" style="width:100%; background:#0d1117; color:white; border:1px solid var(--node-border); border-radius:6px; font-family:monospace">${node.config[key]}</textarea>`;
} else if (key === 'schedType' && node.type === 'start') {
let d = (node.config.mode === 'schedule') ? 'block' : 'none';
html += `<div style="display:${d}"><label>Schedule Type</label><select id="cfg-${key}" onchange="updateStartMode('${node.id}', document.getElementById('cfg-mode').value)">
<option value="period" ${node.config[key]=='period'?'selected':''}>Period (Interval)</option>
<option value="cron" ${node.config[key]=='cron'?'selected':''}>Cron (Time Expression)</option>
</select></div>`;
} else if (key === 'anchor' && node.type === 'start') {
let d = (node.config.mode === 'schedule' && node.config.schedType === 'period') ? 'block' : 'none';
html += `<div style="display:${d}"><label>Next Run After</label><select id="cfg-${key}">
<option value="start" ${node.config[key]=='start'?'selected':''}>Start of Flow</option>
<option value="end" ${node.config[key]=='end'?'selected':''}>End of Flow</option>
</select></div>`;
} else if (key === 'immediate' && node.type === 'start') {
let d = (node.config.mode === 'schedule' && node.config.schedType === 'period') ? 'block' : 'none';
html += `<div style="display:${d}"><label><input type="checkbox" id="cfg-${key}" style="width:auto; margin-bottom:10px" ${node.config[key] ? 'checked' : ''}> Run Immediately on Activation</label></div>`;
} else if (key === 'last_run' || key === 'last_finished' || key === 'enabled') {
continue;
} else {
// Decide visibility for Start node fields
let display = 'block';
if (node.type === 'start') {
  if (node.config.mode !== 'schedule') {
    if (['schedType', 'interval', 'cron', 'immediate', 'anchor'].includes(key)) display = 'none';
  } else {
    // We are in Schedule mode
    if (node.config.schedType === 'period') {
       if (key === 'cron') display = 'none';
    } else if (node.config.schedType === 'cron') {
       if (['interval', 'immediate', 'anchor'].includes(key)) display = 'none';
    }
  }
}
if (node.type === 'telegram') {
if (node.config.method === 'sendMessage' && (key === 'url' || key === 'caption')) display = 'none';
if (node.config.method === 'sendPhoto' && key === 'text') display = 'none';
}
html += `<div style="display:${display}"><label>${key}</label><input id="cfg-${key}" value="${node.config[key]}"></div>`;
}
}
fields.innerHTML = html;
}
function updateStartMode(id, mode) {
    const node = nodes.find(n => n.id === id);
    if (!node) return;
    // Sync current values so we don't lose them
    for (let key in node.config) {
        const el = document.getElementById(`cfg-${key}`);
        if (el) {
            if (el.type === 'checkbox') node.config[key] = el.checked;
            else node.config[key] = el.value;
        }
    }
    node.config.mode = mode;
    // Re-render fields with force=true
    openConfig(node, true);
}
function updateTelegramFields(id, method) {
    const node = nodes.find(n => n.id === id);
    if (!node) return;
    for (let key in node.config) {
        const el = document.getElementById(`cfg-${key}`);
        if (el) node.config[key] = el.value;
    }
    node.config.method = method;
    openConfig(node, true);
}
const numericKeys = ['ms', 'interval', 'count', 'last_run', 'last_finished'];
function closeConfig() {
const node = nodes.find(n => n.id === selectedNodeId);
if (!node) return;
for (let key in node.config) {
const el = document.getElementById(`cfg-${key}`);
if (!el) continue;
if (el.type === 'checkbox') node.config[key] = el.checked;
else if (numericKeys.includes(key)) node.config[key] = parseInt(el.value, 10) || 0;
else node.config[key] = el.value;
}
document.getElementById('config-panel').classList.remove('open');
render();
log(`Updated config for ${selectedNodeId}`);
}
function deleteNode() {
nodes = nodes.filter(n => n.id !== selectedNodeId);
connections = connections.filter(c => c.from !== selectedNodeId && c.to !== selectedNodeId);
document.getElementById('config-panel').classList.remove('open');
render();
log(`Deleted node ${selectedNodeId}`);
}
async function saveFlow() {
try {
await fetch(`/api/flow?name=${currentFlow}`, {
method: 'POST',
body: JSON.stringify({ nodes, connections, pan: {x: panX, y: panY} })
});
log(`Flow ${currentFlow} saved.`);
} catch(e) { log("Error saving flow."); }
}
async function deleteCurrentFlow() {
if(!confirm(`Delete flow "${currentFlow}"?`)) return;
await fetch(`/api/flow?name=${currentFlow}`, { method: 'DELETE' });
currentFlow = "default";
await refreshFlowList();
await openFlow(currentFlow);
}
async function runFlow() {
log(`Triggering flow ${currentFlow}...`);
await fetch(`/api/run?name=${currentFlow}`, { method: 'POST' });
}
async function stopFlow() {
log("Sending stop request...");
document.querySelectorAll('.node').forEach(n => n.classList.remove('executing'));
await fetch('/api/stop', { method: 'POST' });
}
function openWifiConfig() {
if (isAnyModalOpen()) return;
document.getElementById('wifi-ssid').value = wifiData.ssid || '';
document.getElementById('wifi-pass').value = wifiData.pass || '';
document.getElementById('wifi-panel').classList.add('open');
}
async function saveWifi() {
const ssid = document.getElementById('wifi-ssid').value;
const pass = document.getElementById('wifi-pass').value;
if(!ssid) return alert("SSID required");
log("Saving WiFi config...");
await fetch('/api/wifi', {
method: 'POST',
body: JSON.stringify({ ssid, pass })
});
alert("Settings saved. Device will reboot now.");
setTimeout(() => location.reload(), 2000);
}
let otaFirmwareUrl = '';
let tunnelHostGlobal = "domain.com";
let tunnelPortGlobal = 9000;
let tunnelProtocolGlobal = "http";
async function initTunnelGlobals() {
try {
const res = await fetch('/api/tunnel/config');
const data = await res.json();
tunnelEnabledGlobal = data.enabled;
tunnelHostGlobal = data.host;
tunnelPortGlobal = data.port;
tunnelProtocolGlobal = data.protocol || "http";
} catch(e) {}
}
async function openTunnelPanel() {
if (isAnyModalOpen()) return;
await initTunnelGlobals();
document.getElementById('tunnel-enabled').checked = tunnelEnabledGlobal;
document.getElementById('tunnel-protocol').value = tunnelProtocolGlobal;
document.getElementById('tunnel-host').value = tunnelHostGlobal;
document.getElementById('tunnel-port').value = tunnelPortGlobal;
document.getElementById('tunnel-panel').classList.add('open');
}
async function saveTunnelConfig() {
const enabled = document.getElementById('tunnel-enabled').checked;
const protocol = document.getElementById('tunnel-protocol').value;
const host = document.getElementById('tunnel-host').value.trim();
const port = parseInt(document.getElementById('tunnel-port').value) || 80;
await fetch('/api/tunnel/config', {
method: 'POST',
body: JSON.stringify({ enabled, protocol, host, port })
});
log(`Tunnel settings saved. Enabled: ${enabled}`);
document.getElementById('tunnel-panel').classList.remove('open');
initTunnelGlobals(); // Refresh globals
}
async function openOtaPanel() {
if (isAnyModalOpen()) return;
document.getElementById('ota-update-section').style.display = 'none';
document.getElementById('ota-status').innerText = '';
try {
const res = await fetch('/api/ota/config');
const data = await res.json();
document.getElementById('ota-url').value = data.url || '';
document.getElementById('ota-current-ver').innerText = data.version || '-';
} catch(e) {}
document.getElementById('ota-panel').classList.add('open');
}
async function saveOtaUrl() {
const url = document.getElementById('ota-url').value.trim();
await fetch('/api/ota/config', {
method: 'POST',
body: JSON.stringify({ url })
});
log('OTA URL saved: ' + url);
document.getElementById('ota-status').innerText = 'URL saved.';
}
async function checkOta() {
const statusEl = document.getElementById('ota-status');
statusEl.innerText = 'Checking...';
document.getElementById('ota-update-section').style.display = 'none';
try {
const res = await fetch('/api/ota/check');
const data = await res.json();
if (data.status === 'error') {
statusEl.innerText = '❌ ' + data.message;
return;
}
statusEl.innerText = `Current: ${data.current} | Remote: ${data.remote}`;
if (data.updateAvailable) {
document.getElementById('ota-remote-ver').innerText = data.remote;
document.getElementById('ota-update-section').style.display = 'block';
otaFirmwareUrl = data.firmwareUrl;
log('Update available: ' + data.remote);
} else {
statusEl.innerText = '✅ Firmware is up to date (' + data.current + ')';
log('Firmware is up to date.');
}
} catch(e) {
statusEl.innerText = '❌ Connection error';
}
}
async function performOta() {
if (!otaFirmwareUrl) return;
const btn = document.getElementById('ota-update-btn');
btn.disabled = true;
btn.innerText = 'Updating...';
document.getElementById('ota-status').innerText = '⬇️ Downloading firmware...';
log('Starting OTA update from: ' + otaFirmwareUrl);
try {
const res = await fetch('/api/ota/update', {
method: 'POST',
body: JSON.stringify({ url: otaFirmwareUrl })
});
const data = await res.json();
if (data.status === 'ok') {
document.getElementById('ota-status').innerText = '✅ ' + data.message;
log('OTA update successful! Rebooting...');
setTimeout(() => location.reload(), 5000);
} else {
document.getElementById('ota-status').innerText = '❌ ' + data.message;
log('OTA failed: ' + data.message);
btn.disabled = false;
btn.innerText = 'Update Now';
}
} catch(e) {
document.getElementById('ota-status').innerText = '❌ Connection lost (device may be rebooting)';
log('Connection lost during OTA — device may be rebooting.');
setTimeout(() => location.reload(), 10000);
}
}
initTunnelGlobals().then(() => init());
</script>
</body>
</html>
)rawliteral";

#endif
