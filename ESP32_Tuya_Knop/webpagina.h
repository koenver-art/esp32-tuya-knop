#ifndef WEBPAGINA_H
#define WEBPAGINA_H

const char WEBPAGINA[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="nl">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<meta name="apple-mobile-web-app-capable" content="yes">
<title>Lamp Controller</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;
background:#1a1a2e;color:#e0e0e0;max-width:420px;margin:0 auto;padding:16px;
min-height:100vh}
h1{font-size:1.3em;text-align:center;padding:12px 0;color:#fff}
h2{font-size:0.95em;color:#888;margin:16px 0 8px;text-transform:uppercase;letter-spacing:1px}
.card{background:#16213e;border-radius:12px;padding:16px;margin-bottom:12px}
.lamp-row{display:flex;align-items:center;justify-content:space-between;padding:8px 0}
.lamp-naam{font-size:1.1em;font-weight:600}
.lamp-status{font-size:0.85em;color:#888}
.btn{border:none;border-radius:8px;padding:10px 18px;font-size:0.95em;cursor:pointer;
font-weight:600;transition:all 0.2s;touch-action:manipulation}
.btn:active{transform:scale(0.95)}
.btn-aan{background:#4ecca3;color:#1a1a2e}
.btn-uit{background:#e74c3c;color:#fff}
.btn-scene{background:#2d3a5c;color:#e0e0e0;margin:4px;flex:1;min-width:80px}
.btn-scene:active,.btn-scene.active{background:#4ecca3;color:#1a1a2e}
.btn-timer{background:#2d3a5c;color:#e0e0e0;margin:4px;padding:10px 14px}
.btn-timer.active{background:#e67e22;color:#fff}
.power-btn{width:100%;padding:14px;font-size:1.1em;margin:8px 0}
.slider-wrap{margin:12px 0}
.slider-wrap label{display:flex;justify-content:space-between;margin-bottom:6px;font-size:0.9em}
input[type=range]{-webkit-appearance:none;width:100%;height:6px;border-radius:3px;
background:#2d3a5c;outline:none}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:24px;height:24px;
border-radius:50%;background:#4ecca3;cursor:pointer}
.kleuren{display:flex;flex-wrap:wrap;gap:8px;justify-content:center}
.kleur-btn{width:44px;height:44px;border-radius:50%;border:3px solid transparent;
cursor:pointer;transition:all 0.2s}
.kleur-btn:active,.kleur-btn.active{border-color:#fff;transform:scale(1.15)}
.scenes{display:flex;flex-wrap:wrap;gap:4px}
.timers{display:flex;gap:4px;flex-wrap:wrap}
.status-grid{display:grid;grid-template-columns:1fr 1fr;gap:8px;font-size:0.85em}
.status-item{display:flex;justify-content:space-between}
.status-label{color:#888}
.status-val{color:#4ecca3;font-weight:600}
.timer-info{text-align:center;color:#e67e22;font-size:0.9em;margin-top:8px;min-height:1.2em}
#verbinding{text-align:center;font-size:0.8em;color:#e74c3c;min-height:1em;margin-top:8px}
</style>
</head>
<body>

<h1>Woonkamer Lamp</h1>

<div class="card">
  <div class="lamp-row">
    <div>
      <div class="lamp-naam" id="lampNaam">Woonkamer</div>
      <div class="lamp-status" id="lampStatus">Laden...</div>
    </div>
    <button class="btn btn-uit power-btn" id="powerBtn" onclick="togglePower()" style="width:auto;padding:10px 24px">UIT</button>
  </div>
</div>

<div class="card">
  <div class="slider-wrap">
    <label><span>Helderheid</span><span id="dimVal">100%</span></label>
    <input type="range" id="dimSlider" min="5" max="100" value="100" oninput="dimPreview(this.value)" onchange="stuurDim(this.value)">
  </div>
  <div class="slider-wrap">
    <label><span>Kleurtemperatuur</span><span id="tempLabel">neutraal</span></label>
    <input type="range" id="tempSlider" min="0" max="1000" value="500" oninput="tempPreview(this.value)" onchange="stuurTemp(this.value)">
  </div>
</div>

<div class="card">
  <h2>Kleuren</h2>
  <div class="kleuren">
    <div class="kleur-btn" style="background:#fff" onclick="stuurKleur('wit')" title="Wit"></div>
    <div class="kleur-btn" style="background:#ff3333" onclick="stuurKleur('rood')" title="Rood"></div>
    <div class="kleur-btn" style="background:#33ff33" onclick="stuurKleur('groen')" title="Groen"></div>
    <div class="kleur-btn" style="background:#3333ff" onclick="stuurKleur('blauw')" title="Blauw"></div>
    <div class="kleur-btn" style="background:#ffff33" onclick="stuurKleur('geel')" title="Geel"></div>
    <div class="kleur-btn" style="background:#aa33ff" onclick="stuurKleur('paars')" title="Paars"></div>
    <div class="kleur-btn" style="background:#ff8800" onclick="stuurKleur('oranje')" title="Oranje"></div>
    <div class="kleur-btn" style="background:#ff66aa" onclick="stuurKleur('roze')" title="Roze"></div>
    <div class="kleur-btn" style="background:#33ffff" onclick="stuurKleur('cyaan')" title="Cyaan"></div>
  </div>
</div>

<div class="card">
  <h2>Scenes</h2>
  <div class="scenes">
    <button class="btn btn-scene" onclick="stuurScene('film')">Film</button>
    <button class="btn btn-scene" onclick="stuurScene('lezen')">Lezen</button>
    <button class="btn btn-scene" onclick="stuurScene('feest')">Feest</button>
    <button class="btn btn-scene" onclick="stuurScene('ontspannen')">Ontspannen</button>
    <button class="btn btn-scene" onclick="stuurScene('nacht')">Nachtlamp</button>
    <button class="btn btn-scene" onclick="stuurScene('energiek')">Energiek</button>
  </div>
</div>

<div class="card">
  <h2>Timer</h2>
  <div class="timers">
    <button class="btn btn-timer" onclick="stuurTimer(5)">5 min</button>
    <button class="btn btn-timer" onclick="stuurTimer(15)">15 min</button>
    <button class="btn btn-timer" onclick="stuurTimer(30)">30 min</button>
    <button class="btn btn-timer" onclick="stuurTimer(60)">60 min</button>
    <button class="btn btn-timer" onclick="stuurTimer(0)">Annuleer</button>
  </div>
  <div class="timer-info" id="timerInfo"></div>
</div>

<div class="card">
  <h2>Status</h2>
  <div class="status-grid">
    <div class="status-item"><span class="status-label">WiFi</span><span class="status-val" id="sWifi">-</span></div>
    <div class="status-item"><span class="status-label">BLE</span><span class="status-val" id="sBle">-</span></div>
    <div class="status-item"><span class="status-label">Aanwezig</span><span class="status-val" id="sAanwezig">-</span></div>
    <div class="status-item"><span class="status-label">Buiten</span><span class="status-val" id="sBuiten">-</span></div>
    <div class="status-item"><span class="status-label">Uptime</span><span class="status-val" id="sUptime">-</span></div>
    <div class="status-item"><span class="status-label">Geheugen</span><span class="status-val" id="sGeheugen">-</span></div>
  </div>
</div>

<div id="verbinding"></div>

<script>
var pollInterval;
var timerEinde = 0;

function api(pad, cb) {
  fetch('/api/' + pad)
    .then(r => r.json())
    .then(d => { if(cb) cb(d); document.getElementById('verbinding').textContent=''; })
    .catch(e => { document.getElementById('verbinding').textContent='Verbinding verloren...'; });
}

function togglePower() {
  var btn = document.getElementById('powerBtn');
  var aan = btn.classList.contains('btn-uit');
  api(aan ? 'aan' : 'uit', updateUI);
}

function stuurDim(val) {
  api('dim/' + val, updateUI);
}

function dimPreview(val) {
  document.getElementById('dimVal').textContent = val + '%';
}

function stuurTemp(val) {
  api('temp/' + val, updateUI);
}

function tempPreview(val) {
  var v = parseInt(val);
  var label = v < 300 ? 'warm' : v > 700 ? 'koud' : 'neutraal';
  document.getElementById('tempLabel').textContent = label;
}

function stuurKleur(kleur) {
  api('kleur/' + kleur, updateUI);
  // Highlight
  document.querySelectorAll('.kleur-btn').forEach(b => b.classList.remove('active'));
  event.target.classList.add('active');
}

function stuurScene(scene) {
  api('scene/' + scene, updateUI);
  document.querySelectorAll('.btn-scene').forEach(b => b.classList.remove('active'));
  event.target.classList.add('active');
}

function stuurTimer(min) {
  api('timer/' + min, function(d) {
    updateUI(d);
    if (min > 0) {
      timerEinde = Date.now() + min * 60000;
    } else {
      timerEinde = 0;
    }
    updateTimerInfo();
  });
  document.querySelectorAll('.btn-timer').forEach(b => b.classList.remove('active'));
  if (min > 0) event.target.classList.add('active');
}

function updateTimerInfo() {
  var el = document.getElementById('timerInfo');
  if (timerEinde <= 0) { el.textContent = ''; return; }
  var rest = Math.max(0, Math.round((timerEinde - Date.now()) / 60000));
  if (rest <= 0) { el.textContent = ''; timerEinde = 0; return; }
  el.textContent = 'Lamp gaat uit over ' + rest + ' min';
}

function updateUI(d) {
  if (!d) return;
  var btn = document.getElementById('powerBtn');
  if (d.aan) {
    btn.textContent = 'AAN';
    btn.className = 'btn btn-aan power-btn';
    btn.style.width = 'auto';
    btn.style.padding = '10px 24px';
  } else {
    btn.textContent = 'UIT';
    btn.className = 'btn btn-uit power-btn';
    btn.style.width = 'auto';
    btn.style.padding = '10px 24px';
  }
  if (d.lamp) document.getElementById('lampNaam').textContent = d.lamp;
  document.getElementById('lampStatus').textContent = d.aan ? 'Aan — ' + d.helderheid + '%' : 'Uit';
  document.getElementById('dimSlider').value = d.helderheid || 100;
  document.getElementById('dimVal').textContent = (d.helderheid || 100) + '%';
  if (d.wifi) document.getElementById('sWifi').textContent = d.wifi;
  if (d.ble !== undefined) document.getElementById('sBle').textContent = d.ble ? 'verbonden' : 'wacht';
  if (d.aanwezig) document.getElementById('sAanwezig').textContent = d.aanwezig;
  if (d.donker !== undefined) document.getElementById('sBuiten').textContent = d.donker ? 'donker' : 'licht';
  if (d.uptime) document.getElementById('sUptime').textContent = d.uptime;
  if (d.geheugen) document.getElementById('sGeheugen').textContent = d.geheugen;
  if (d.timer > 0) {
    timerEinde = Date.now() + d.timer * 1000;
  }
}

function poll() {
  api('status', updateUI);
  updateTimerInfo();
}

// Poll elke 3 seconden
poll();
pollInterval = setInterval(poll, 3000);
setInterval(updateTimerInfo, 10000);
</script>
</body>
</html>
)rawliteral";

#endif // WEBPAGINA_H
