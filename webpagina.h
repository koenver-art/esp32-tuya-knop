#ifndef WEBPAGINA_H
#define WEBPAGINA_H

const char WEBPAGINA[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head><meta charset=UTF-8><meta name=viewport content="width=device-width,initial-scale=1"><title>Lamp</title>
<style>*{margin:0;padding:0;box-sizing:border-box}body{font-family:sans-serif;background:#111;color:#eee;max-width:380px;margin:0 auto;padding:10px}
.c{background:#1a1a2e;border-radius:12px;padding:12px;margin:6px 0}.b{width:100%;padding:14px;border:none;border-radius:10px;font-size:1.1em;font-weight:700;cursor:pointer}
.on{background:#4ecca3;color:#111}.off{background:#e74c3c;color:#fff}.r{display:flex;flex-wrap:wrap;gap:6px}
.s{flex:1;min-width:55px;padding:8px;border:none;border-radius:8px;background:#222;color:#aaa;font-size:.8em;cursor:pointer}
.s:active{background:#4ecca3;color:#111}.k{width:36px;height:36px;border-radius:50%;border:2px solid transparent;cursor:pointer;display:inline-block;margin:3px}
.k:active{border-color:#fff}input[type=range]{width:100%;margin:6px 0;height:6px;-webkit-appearance:none;background:#333;border-radius:3px;outline:none}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:22px;height:22px;border-radius:50%;background:#4ecca3;cursor:pointer}
.l{font-size:.75em;color:#666;display:flex;justify-content:space-between}.v{color:#4ecca3}h2{font-size:.7em;color:#555;letter-spacing:1px;margin:8px 0 4px}
</style></head><body>
<div class=c><button class="b off" id=pw onclick="tp()">UIT</button></div>
<div class=c><div class=l><span>Helderheid</span><span class=v id=dv>100%</span></div>
<input type=range id=ds min=5 max=100 value=100 oninput="dv.textContent=this.value+'%'" onchange="f('dim/'+this.value)">
<div class=l><span>Temperatuur</span><span class=v id=tl>midden</span></div>
<input type=range id=ts min=0 max=1000 value=500 oninput="tl.textContent=this.value<300?'warm':this.value>700?'koud':'midden'" onchange="f('temp/'+this.value)"></div>
<div class=c><h2>KLEUREN</h2>
<div class=k style="background:#fff" onclick="f('kleur/wit')"></div>
<div class=k style="background:#f33" onclick="f('kleur/rood')"></div>
<div class=k style="background:#3c3" onclick="f('kleur/groen')"></div>
<div class=k style="background:#36f" onclick="f('kleur/blauw')"></div>
<div class=k style="background:#fd0" onclick="f('kleur/geel')"></div>
<div class=k style="background:#a3f" onclick="f('kleur/paars')"></div>
<div class=k style="background:#f80" onclick="f('kleur/oranje')"></div>
<div class=k style="background:#f6a" onclick="f('kleur/roze')"></div>
<div class=k style="background:#0dd" onclick="f('kleur/cyaan')"></div></div>
<div class=c><h2>SCENES</h2><div class=r>
<button class=s onclick="f('scene/film')">Film</button>
<button class=s onclick="f('scene/lezen')">Lezen</button>
<button class=s onclick="f('scene/feest')">Feest</button>
<button class=s onclick="f('scene/ontspannen')">Relax</button>
<button class=s onclick="f('scene/nacht')">Nacht</button>
<button class=s onclick="f('scene/energiek')">Energie</button></div></div>
<div class=c><h2>TIMER</h2><div class=r>
<button class=s onclick="f('timer/5')">5m</button>
<button class=s onclick="f('timer/15')">15m</button>
<button class=s onclick="f('timer/30')">30m</button>
<button class=s onclick="f('timer/60')">1u</button>
<button class=s onclick="f('timer/0')">&#x2715;</button></div>
<div id=ti style="text-align:center;color:#e67e22;font-size:.8em;margin-top:4px"></div></div>
<div class=c><h2>STATUS</h2><div id=st style="font-size:.75em;color:#888">Laden...</div></div>
<script>
var pw=document.getElementById('pw'),te=0;
function f(p){fetch('/api/'+p).then(r=>r.json()).then(u).catch(()=>{})}
function tp(){f(pw.classList.contains('off')?'aan':'uit')}
function u(d){if(!d)return;if(d.aan){pw.textContent='AAN';pw.className='b on'}else{pw.textContent='UIT';pw.className='b off'}
document.getElementById('ds').value=d.helderheid||100;document.getElementById('dv').textContent=(d.helderheid||100)+'%';
if(d.timer>0)te=Date.now()+d.timer*1000;var s='';if(d.wifi)s+=d.wifi;if(d.aanwezig)s+=' | '+d.aanwezig;if(d.uptime)s+=' | '+d.uptime;document.getElementById('st').textContent=s||'-';
var ti=document.getElementById('ti');if(te>0){var m=Math.round((te-Date.now())/60000);ti.textContent=m>0?'Uit over '+m+'m':''}else ti.textContent=''}
setInterval(function(){f('status')},5000);f('status');
</script></body></html>
)rawliteral";

#endif
