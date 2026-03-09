#ifndef WEBPAGINA_H
#define WEBPAGINA_H

const char WEBPAGINA[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="nl">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<meta name="apple-mobile-web-app-capable" content="yes">
<title>Lamp</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,sans-serif;background:#0f0f1a;color:#e0e0e0;max-width:400px;margin:0 auto;padding:12px}
.h{text-align:center;padding:10px 0;font-size:1.2em;color:#fff;letter-spacing:1px}
.c{background:linear-gradient(135deg,#1a1a2e,#16213e);border-radius:16px;padding:14px;margin:8px 0;box-shadow:0 4px 15px rgba(0,0,0,0.3)}
.pb{width:100%;padding:16px;border:none;border-radius:12px;font-size:1.2em;font-weight:700;cursor:pointer;transition:all .2s;letter-spacing:1px}
.pb:active{transform:scale(0.96)}
.on{background:linear-gradient(135deg,#4ecca3,#38b2ac);color:#0f0f1a}
.off{background:linear-gradient(135deg,#e74c3c,#c0392b);color:#fff}
.sl{margin:10px 0}
.sl label{display:flex;justify-content:space-between;font-size:.85em;color:#888;margin-bottom:4px}
.sl label span:last-child{color:#4ecca3;font-weight:600}
input[type=range]{-webkit-appearance:none;width:100%;height:8px;border-radius:4px;background:#2d3a5c;outline:none}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:28px;height:28px;border-radius:50%;background:#4ecca3;cursor:pointer;box-shadow:0 2px 8px rgba(78,204,163,0.4)}
.t{font-size:.8em;color:#666;text-transform:uppercase;letter-spacing:2px;margin:12px 0 6px}
.kr{display:flex;flex-wrap:wrap;gap:10px;justify-content:center;padding:4px 0}
.kb{width:42px;height:42px;border-radius:50%;border:3px solid transparent;cursor:pointer;transition:all .15s;box-shadow:0 2px 8px rgba(0,0,0,0.3)}
.kb:active,.kb.a{border-color:#fff;transform:scale(1.2);box-shadow:0 2px 12px rgba(255,255,255,0.3)}
.sr{display:flex;flex-wrap:wrap;gap:6px}
.sb{flex:1;min-width:70px;padding:10px 6px;border:none;border-radius:10px;background:#1e2a4a;color:#aaa;font-size:.85em;font-weight:600;cursor:pointer;transition:all .15s}
.sb:active,.sb.a{background:#4ecca3;color:#0f0f1a}
.tr{display:flex;gap:6px}
.tb{flex:1;padding:10px 4px;border:none;border-radius:10px;background:#1e2a4a;color:#aaa;font-size:.85em;font-weight:600;cursor:pointer;transition:all .15s}
.tb:active,.tb.a{background:#e67e22;color:#fff}
.ti{text-align:center;color:#e67e22;font-size:.85em;margin-top:6px;min-height:1em}
.sg{display:grid;grid-template-columns:1fr 1fr;gap:6px;font-size:.8em}
.si{display:flex;justify-content:space-between;padding:4px 8px;background:#0f0f1a;border-radius:6px}
.si span:first-child{color:#666}
.si span:last-child{color:#4ecca3;font-weight:600}
#cx{text-align:center;font-size:.75em;min-height:1em;margin-top:6px}
.err{color:#e74c3c}
</style>
</head>
<body>
<div class="h">WOONKAMER LAMP</div>

<div class="c">
<button class="pb off" id="pw" onclick="tp()">UIT</button>
</div>

<div class="c">
<div class="sl"><label><span>Helderheid</span><span id="dv">100%</span></label>
<input type="range" id="ds" min="5" max="100" value="100" oninput="dp(this.value)" onchange="sd(this.value)"></div>
<div class="sl"><label><span>Temperatuur</span><span id="tl">neutraal</span></label>
<input type="range" id="ts" min="0" max="1000" value="500" oninput="tv(this.value)" onchange="st(this.value)"></div>
</div>

<div class="c">
<div class="t">Kleuren</div>
<div class="kr">
<div class="kb" style="background:#fff" onclick="sk('wit')"></div>
<div class="kb" style="background:#ff3333" onclick="sk('rood')"></div>
<div class="kb" style="background:#33cc33" onclick="sk('groen')"></div>
<div class="kb" style="background:#3366ff" onclick="sk('blauw')"></div>
<div class="kb" style="background:#ffdd00" onclick="sk('geel')"></div>
<div class="kb" style="background:#aa33ff" onclick="sk('paars')"></div>
<div class="kb" style="background:#ff8800" onclick="sk('oranje')"></div>
<div class="kb" style="background:#ff66aa" onclick="sk('roze')"></div>
<div class="kb" style="background:#00dddd" onclick="sk('cyaan')"></div>
</div>
</div>

<div class="c">
<div class="t">Scenes</div>
<div class="sr">
<button class="sb" onclick="ss('film')">Film</button>
<button class="sb" onclick="ss('lezen')">Lezen</button>
<button class="sb" onclick="ss('feest')">Feest</button>
<button class="sb" onclick="ss('ontspannen')">Relax</button>
<button class="sb" onclick="ss('nacht')">Nacht</button>
<button class="sb" onclick="ss('energiek')">Energie</button>
</div>
</div>

<div class="c">
<div class="t">Timer</div>
<div class="tr">
<button class="tb" onclick="stt(5)">5m</button>
<button class="tb" onclick="stt(15)">15m</button>
<button class="tb" onclick="stt(30)">30m</button>
<button class="tb" onclick="stt(60)">1u</button>
<button class="tb" onclick="stt(0)">&#x2715;</button>
</div>
<div class="ti" id="ti"></div>
</div>

<div class="c">
<div class="t">Status</div>
<div class="sg">
<div class="si"><span>WiFi</span><span id="sw">-</span></div>
<div class="si"><span>BLE</span><span id="sb">-</span></div>
<div class="si"><span>Thuis</span><span id="sa">-</span></div>
<div class="si"><span>Buiten</span><span id="sd">-</span></div>
<div class="si"><span>Uptime</span><span id="su">-</span></div>
<div class="si"><span>RAM</span><span id="sm">-</span></div>
</div>
</div>
<div id="cx"></div>

<script>
var te=0;
function q(p,f){fetch('/api/'+p).then(r=>r.json()).then(d=>{if(f)f(d);document.getElementById('cx').textContent='';}).catch(()=>{document.getElementById('cx').className='err';document.getElementById('cx').textContent='Geen verbinding';})}
function tp(){var b=document.getElementById('pw');q(b.classList.contains('off')?'aan':'uit',u)}
function sd(v){q('dim/'+v,u)}
function dp(v){document.getElementById('dv').textContent=v+'%'}
function st(v){q('temp/'+v,u)}
function tv(v){var n=parseInt(v);document.getElementById('tl').textContent=n<300?'warm':n>700?'koud':'neutraal'}
function sk(k){q('kleur/'+k,u);document.querySelectorAll('.kb').forEach(b=>b.classList.remove('a'));if(event&&event.target)event.target.classList.add('a')}
function ss(s){q('scene/'+s,u);document.querySelectorAll('.sb').forEach(b=>b.classList.remove('a'));if(event&&event.target)event.target.classList.add('a')}
function stt(m){q('timer/'+m,function(d){u(d);if(m>0){te=Date.now()+m*60000}else{te=0}ut()});document.querySelectorAll('.tb').forEach(b=>b.classList.remove('a'));if(m>0&&event&&event.target)event.target.classList.add('a')}
function ut(){var e=document.getElementById('ti');if(te<=0){e.textContent='';return}var r=Math.max(0,Math.round((te-Date.now())/60000));if(r<=0){e.textContent='';te=0;return}e.textContent='Uit over '+r+' min'}
function u(d){if(!d)return;var b=document.getElementById('pw');if(d.aan){b.textContent='AAN';b.className='pb on'}else{b.textContent='UIT';b.className='pb off'}document.getElementById('ds').value=d.helderheid||100;document.getElementById('dv').textContent=(d.helderheid||100)+'%';if(d.wifi)document.getElementById('sw').textContent=d.wifi;if(d.ble!==undefined)document.getElementById('sb').textContent=d.ble?'ja':'nee';if(d.aanwezig)document.getElementById('sa').textContent=d.aanwezig;if(d.donker!==undefined)document.getElementById('sd').textContent=d.donker?'donker':'licht';if(d.uptime)document.getElementById('su').textContent=d.uptime;if(d.geheugen)document.getElementById('sm').textContent=d.geheugen;if(d.timer>0)te=Date.now()+d.timer*1000}
function pl(){q('status',u);ut()}
pl();setInterval(pl,3000);setInterval(ut,10000);
</script>
</body>
</html>
)rawliteral";

#endif
