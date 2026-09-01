#ifndef WEB_UI_H_
#define WEB_UI_H_
#include <Arduino.h>

// SPA del portal Web Tools. Se sirve con send_P desde web_tools_task y habla con
// las rutas /api/* del propio firmware (login por token en cookie, /api/config
// GET/POST). Diseño "consola de instrumento": monoespaciada + acento ámbar,
// tema claro/oscuro por prefers-color-scheme. Files/OTA llegan en fases 2/3.
static const char WEB_UI_HTML[] PROGMEM = R"HTMLDOC(<!DOCTYPE html>
<html lang="en"><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Hackbat Console</title>
<style>
:root{--bg:#0E1116;--panel:#161B22;--panel2:#12161C;--raised:#1C232D;--bd:#29313C;--bd2:#384250;
--tx:#E6EAF0;--mut:#8B95A3;--fnt:#626C7A;--ac:#E9A23B;--ac2:#FFB84D;--on:#170F02;--gd:#34D399;--dg:#F0616D;}
@media(prefers-color-scheme:light){:root{--bg:#EEF1F4;--panel:#FBFCFD;--panel2:#F2F5F8;--raised:#FFF;
--bd:#D6DCE3;--bd2:#C3CBD4;--tx:#161B22;--mut:#5A6472;--fnt:#8B95A3;--ac:#C77E14;--ac2:#E9A23B;--on:#1A1206;--gd:#0E9F6E;--dg:#C93E4A;}}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--tx);font-family:"IBM Plex Mono",ui-monospace,Menlo,Consolas,monospace;font-size:14px;line-height:1.5}
a{color:var(--ac)}
.bar{position:sticky;top:0;z-index:5;background:var(--panel);border-bottom:1px solid var(--bd);display:flex;align-items:center;gap:12px;padding:11px 16px}
.mk{width:24px;height:24px;color:var(--ac);flex:none}
.wm{font-weight:700;letter-spacing:.12em;text-transform:uppercase;font-size:15px}.wm b{color:var(--ac)}
.sp{flex:1}
.pill{display:flex;align-items:center;gap:7px;font-size:12px;color:var(--mut);border:1px solid var(--bd);background:var(--panel2);border-radius:6px;padding:5px 9px}
.pill .d{width:8px;height:8px;border-radius:50%;background:var(--gd)}
.pill b{color:var(--tx);font-weight:500}
.tabs{display:flex;gap:2px;padding:0 10px;background:var(--bg);border-bottom:1px solid var(--bd);position:sticky;top:47px;z-index:4;overflow-x:auto}
.tab{background:none;border:none;color:var(--mut);font:inherit;font-size:12.5px;letter-spacing:.06em;text-transform:uppercase;padding:12px 14px 10px;cursor:pointer;border-bottom:2px solid transparent;margin-bottom:-1px;white-space:nowrap}
.tab[aria-selected=true]{color:var(--tx);border-bottom-color:var(--ac)}
main{max-width:720px;margin:0 auto;padding:18px 16px 60px}
.card{background:var(--panel);border:1px solid var(--bd);border-radius:10px;margin-bottom:16px;overflow:hidden}
.hd{background:var(--panel2);border-bottom:1px solid var(--bd);padding:11px 15px}
.eb{color:var(--ac);font-size:10.5px;letter-spacing:.14em;text-transform:uppercase}
.hd h2{margin:2px 0 0;font-size:14px;font-weight:600}
.bd{padding:14px 15px}
.f{display:grid;grid-template-columns:150px 1fr;gap:12px;align-items:center;padding:10px 0;border-top:1px solid var(--bd)}
.f:first-child{border-top:none;padding-top:2px}
.f label{color:var(--mut);font-size:13px}
@media(max-width:520px){.f{grid-template-columns:1fr;gap:5px}}
select,input[type=text],input[type=password]{width:100%;font:inherit;font-size:14px;color:var(--tx);background:var(--raised);border:1px solid var(--bd2);border-radius:7px;padding:9px 11px}
select:focus,input:focus{outline:2px solid var(--ac);outline-offset:1px;border-color:var(--ac)}
input::placeholder{color:var(--fnt)}
.seg{display:inline-flex;border:1px solid var(--bd2);border-radius:8px;overflow:hidden;background:var(--raised)}
.seg button{border:none;background:none;font:inherit;font-size:13px;color:var(--mut);padding:8px 18px;cursor:pointer}
.seg button[aria-pressed=true]{background:var(--ac);color:var(--on);font-weight:600}
.brt{display:flex;align-items:center;gap:12px;flex-wrap:wrap}
input[type=range]{-webkit-appearance:none;appearance:none;height:4px;border-radius:3px;background:var(--bd2);flex:1 1 140px}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:16px;height:16px;border-radius:50%;background:var(--ac);border:2px solid var(--raised);cursor:pointer}
.leds{display:flex;gap:5px}.led{width:12px;height:12px;border-radius:50%;border:1px solid var(--bd2);background:var(--panel2)}
.led.on{background:var(--ac2);border-color:var(--ac);box-shadow:0 0 6px var(--ac)}
.bv{color:var(--mut);font-size:13px;min-width:44px}
.btn{cursor:pointer;font:inherit;font-size:13px;font-weight:500;border-radius:7px;padding:10px 16px;border:1px solid var(--bd2);background:var(--raised);color:var(--tx);display:inline-flex;align-items:center;gap:8px}
.btn.pri{background:var(--ac);border-color:var(--ac);color:var(--on);font-weight:600}
.btn:disabled{opacity:.5;cursor:not-allowed}
.act{display:flex;align-items:center;gap:10px}.act .n{color:var(--fnt);font-size:12px;margin-left:auto}
.soon{color:var(--mut);text-align:center;padding:30px 16px;font-size:13px}
.soon b{color:var(--ac);display:block;margin-bottom:6px;letter-spacing:.1em;text-transform:uppercase;font-size:11px}
.storage{display:flex;align-items:center;gap:12px;flex-wrap:wrap}
.fbar{flex:1 1 180px;height:8px;border-radius:5px;background:var(--panel2);border:1px solid var(--bd);overflow:hidden}
.fbar>span{display:block;height:100%;width:0;background:linear-gradient(90deg,var(--ac),var(--ac2))}
.storage .lbl{font-size:12.5px;color:var(--mut)}.storage .lbl b{color:var(--tx);font-weight:500}
.fdir{font-size:11px;letter-spacing:.06em;text-transform:uppercase;color:var(--fnt);padding:14px 2px 5px;display:flex;align-items:center;gap:8px}
.fdir:first-child{padding-top:2px}.fdir::after{content:"";flex:1;height:1px;background:var(--bd)}
.frow{display:flex;align-items:center;gap:10px;padding:9px 11px;border:1px solid var(--bd);border-radius:8px;background:var(--panel2);margin-top:8px}
.frow .fn{overflow:hidden;text-overflow:ellipsis;white-space:nowrap;font-size:13.5px}
.frow .fs{margin-left:auto;color:var(--fnt);font-size:12.5px;flex:none}
.frow .fb{display:flex;gap:6px;flex:none}
.rb{width:30px;height:30px;display:grid;place-items:center;border-radius:6px;border:1px solid var(--bd);background:var(--raised);color:var(--mut);cursor:pointer;text-decoration:none}
.rb:hover{color:var(--tx);border-color:var(--bd2)}.rb.del:hover{color:var(--dg);border-color:var(--dg)}
.rb svg{width:15px;height:15px}
input[type=file]{width:100%;font:inherit;font-size:13px;color:var(--mut)}
.hide[hidden]{display:none}
#gate{position:fixed;inset:0;z-index:20;background:var(--bg);display:grid;place-items:center;padding:20px}
#gate[hidden]{display:none}
.gc{width:100%;max-width:340px;background:var(--panel);border:1px solid var(--bd);border-radius:12px;padding:26px 22px;text-align:center}
.gc .mk{width:38px;height:38px;margin:0 auto 12px}
.gc h1{margin:0;font-size:17px;letter-spacing:.1em;text-transform:uppercase}.gc h1 b{color:var(--ac)}
.gc p{color:var(--mut);font-size:12.5px;margin:8px 0 18px}
#tok{width:100%;text-align:center;font-size:24px;letter-spacing:.35em;padding:12px 8px 12px 18px;text-transform:uppercase}
.ge{color:var(--dg);font-size:12px;min-height:16px;margin:9px 0}
#toast{position:fixed;left:50%;bottom:24px;transform:translate(-50%,20px);background:var(--tx);color:var(--bg);font-size:13px;padding:10px 18px;border-radius:8px;z-index:30;opacity:0;pointer-events:none;transition:.2s}
#toast.show{opacity:1;transform:translate(-50%,0)}
@media(prefers-reduced-motion:reduce){*{transition:none!important}}
</style></head><body>

<div id="gate"><form class="gc" id="gf">
<svg class="mk" viewBox="0 0 32 32" fill="none"><path d="M16 20c-1.6-3.4-3.9-5-6.4-5-1.7 0-2.6.9-3.6 2 .3-2.2.2-3.8-.6-5.2 1.9 1 3.2 1.1 4.6.5C13 6.8 14.6 6 16 6s3 .8 6 6.3c1.4.6 2.7.5 4.6-.5-.8 1.4-.9 3-.6 5.2-1-1.1-1.9-2-3.6-2-2.5 0-4.8 1.6-6.4 5Z" fill="currentColor"/></svg>
<h1>Hack<b>bat</b></h1><p>Enter the access token shown on the badge screen.</p>
<input id="tok" type="text" maxlength="6" autocomplete="off" placeholder="••••••" aria-label="token">
<div class="ge" id="ge"></div>
<button class="btn pri" type="submit" style="width:100%;justify-content:center">Unlock console</button>
</form></div>

<div id="app" hidden>
<div class="bar">
<svg class="mk" viewBox="0 0 32 32" fill="none"><path d="M16 20c-1.6-3.4-3.9-5-6.4-5-1.7 0-2.6.9-3.6 2 .3-2.2.2-3.8-.6-5.2 1.9 1 3.2 1.1 4.6.5C13 6.8 14.6 6 16 6s3 .8 6 6.3c1.4.6 2.7.5 4.6-.5-.8 1.4-.9 3-.6 5.2-1-1.1-1.9-2-3.6-2-2.5 0-4.8 1.6-6.4 5Z" fill="currentColor"/></svg>
<div><div class="wm">Hack<b>bat</b></div></div>
<div class="sp"></div>
<div class="pill"><span class="d"></span><span id="pMode">AP</span>·<b id="pAddr">—</b></div>
</div>
<div class="tabs" role="tablist">
<button class="tab" role="tab" aria-selected="true" data-t="config">Config</button>
<button class="tab" role="tab" aria-selected="false" data-t="files">Files</button>
<button class="tab" role="tab" aria-selected="false" data-t="ota">OTA</button>
</div>
<main>
<section data-p="config">
<div class="card"><div class="hd"><div class="eb">Radio</div><h2>Sub-GHz defaults</h2></div><div class="bd">
<div class="f"><label>Frequency</label><select id="freq">
<option value="0">315.00 MHz</option><option value="1">433.92 MHz</option><option value="2">868.00 MHz</option><option value="3">915.00 MHz</option></select></div>
<div class="f"><label>Preset</label><select id="preset">
<option value="0">AM270 (OOK)</option><option value="1">AM650 (OOK)</option><option value="2">FM238 (2-FSK)</option><option value="3">FM476 (2-FSK)</option></select></div>
</div></div>
<div class="card"><div class="hd"><div class="eb">NeoPixels</div><h2>LED brightness</h2></div><div class="bd">
<div class="f"><label>Brightness</label><div class="brt">
<input type="range" id="bright" min="0" max="10" value="5"><span class="bv" id="bval">5 / 10</span><div class="leds" id="leds"></div></div></div>
</div></div>
<div class="card"><div class="hd"><div class="eb">Profile</div><h2>Identity</h2></div><div class="bd">
<div class="f"><label>Name</label><input type="text" id="uname"></div>
<div class="f"><label>Nick</label><input type="text" id="unick"></div>
</div></div>
<div class="card"><div class="hd"><div class="eb">Network</div><h2>How the console is reached</h2></div><div class="bd">
<div class="f"><label>Mode</label><div class="seg" id="seg" role="group">
<button type="button" data-m="0" aria-pressed="true">AP</button><button type="button" data-m="1" aria-pressed="false">STA</button></div></div>
<div id="apf" class="hide">
<div class="f"><label>AP name</label><input type="text" id="apssid"></div>
<div class="f"><label>AP password</label><input type="password" id="appass" placeholder="unchanged"></div></div>
<div id="staf" class="hide" hidden>
<div class="f"><label>WiFi SSID</label><input type="text" id="stassid" placeholder="MyHomeWiFi (2.4GHz)"></div>
<div class="f"><label>WiFi password</label><input type="password" id="stapass" placeholder="unchanged"></div></div>
<div class="f"><label>Hostname</label><input type="text" id="mdns" readonly style="color:var(--mut)"></div>
</div></div>
<div class="act"><button class="btn pri" id="save">Save to device</button><span class="n">network changes apply on next start</span></div>
</section>
<section data-p="files" hidden>
<div class="card"><div class="hd"><div class="eb">LittleFS</div><h2>Storage</h2></div><div class="bd">
<div class="storage"><span class="lbl"><b id="stUsed">—</b> used</span>
<div class="fbar"><span id="stBar"></span></div><span class="lbl">of <b id="stTotal">—</b></span></div>
</div></div>
<div class="card"><div class="hd"><div class="eb">Browse</div><h2>Saved files</h2><span class="n" id="fSub"></span></div>
<div class="bd" id="fList"><div class="soon">Loading…</div></div></div>
<div class="card"><div class="hd"><div class="eb">Upload</div><h2>Add a file</h2></div><div class="bd">
<div class="f"><label>Folder</label><select id="upDir">
<option value="/subghz/rc-switch">/subghz/rc-switch</option>
<option value="/subghz/raw">/subghz/raw</option>
<option value="/evilportal">/evilportal</option></select></div>
<div class="f"><label>File</label><input type="file" id="upFile"></div>
<div class="act"><button class="btn pri" id="upBtn">Upload</button><span class="n" id="upNote"></span></div>
</div></div>
</section>
<section data-p="ota" hidden><div class="card"><div class="bd"><div class="soon"><b>OTA Update</b>Over-the-air flashing ships in a later build.</div></div></div></section>
</main>
</div>
<div id="toast"><span id="tmsg"></span></div>

<script>
var $=function(s){return document.querySelector(s)},$$=function(s){return Array.prototype.slice.call(document.querySelectorAll(s))};
// brightness LEDs
var leds=$("#leds");for(var i=0;i<10;i++){var d=document.createElement("div");d.className="led";leds.appendChild(d);}
var ledEls=$$("#leds .led");
function rb(){var v=+$("#bright").value;$("#bval").textContent=v+" / 10";ledEls.forEach(function(e,i){e.classList.toggle("on",i<v);});}
$("#bright").addEventListener("input",rb);rb();
// network mode segmented
function setMode(m){$$("#seg button").forEach(function(b){b.setAttribute("aria-pressed",b.dataset.m==m);});
$("#apf").hidden=(m!=0);$("#staf").hidden=(m!=1);$("#pMode").textContent=(m==0?"AP":"STA");}
$$("#seg button").forEach(function(b){b.addEventListener("click",function(){setMode(b.dataset.m);});});
// tabs
$$(".tab").forEach(function(t){t.addEventListener("click",function(){
$$(".tab").forEach(function(x){x.setAttribute("aria-selected",x===t);});
$$("section").forEach(function(p){p.hidden=(p.dataset.p!==t.dataset.t);});
if(t.dataset.t=="files")loadFiles();});});
// files
function fmtSize(b){if(b<1024)return b+" B";if(b<1048576)return (b/1024).toFixed(1)+" KB";return (b/1048576).toFixed(2)+" MB";}
function loadFiles(){$("#fList").innerHTML='<div class="soon">Loading…</div>';
fetch("/api/files").then(function(r){return r.json();}).then(function(d){
$("#stUsed").textContent=fmtSize(d.used);$("#stTotal").textContent=fmtSize(d.total);
$("#stBar").style.width=Math.max(1,d.used/d.total*100).toFixed(1)+"%";
var l=$("#fList");l.innerHTML="";
if(!d.files.length){l.innerHTML='<div class="soon">No files stored.</div>';$("#fSub").textContent="0 files";return;}
$("#fSub").textContent=d.files.length+(d.files.length==1?" file":" files");
var dirs=[];d.files.forEach(function(f){var dir=f.path.substring(0,f.path.lastIndexOf("/"));if(dirs.indexOf(dir)<0)dirs.push(dir);});
dirs.forEach(function(dir){var h=document.createElement("div");h.className="fdir";h.textContent=dir;l.appendChild(h);
d.files.filter(function(f){return f.path.substring(0,f.path.lastIndexOf("/"))==dir;}).forEach(function(f){
var row=document.createElement("div");row.className="frow";
row.innerHTML='<span class="fn">'+f.name+'</span><span class="fs">'+fmtSize(f.size)+'</span><span class="fb">'
+'<a class="rb" href="/api/file?path='+encodeURIComponent(f.path)+'" title="Download"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7"><path d="M12 4v10m0 0 4-4m-4 4-4-4" stroke-linecap="round" stroke-linejoin="round"/><path d="M5 18h14" stroke-linecap="round"/></svg></a>'
+'<button class="rb del" title="Delete"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.7"><path d="M5 7h14M9 7V5h6v2M7 7l1 13h8l1-13" stroke-linecap="round" stroke-linejoin="round"/></svg></button></span>';
row.querySelector(".del").addEventListener("click",function(){delFile(f.path,f.name);});l.appendChild(row);});});
}).catch(function(){$("#fList").innerHTML='<div class="soon">Failed to load.</div>';});}
function delFile(path,name){if(!confirm("Delete "+name+"?"))return;
fetch("/api/file/delete?path="+encodeURIComponent(path),{method:"POST"})
.then(function(r){if(!r.ok)throw 0;toast("Deleted "+name);loadFiles();}).catch(function(){toast("Delete failed");});}
$("#upBtn").addEventListener("click",function(){var f=$("#upFile").files[0];if(!f){toast("Choose a file");return;}
var fd=new FormData();fd.append("file",f,f.name);$("#upBtn").disabled=true;$("#upNote").textContent="Uploading…";
fetch("/api/file?dir="+encodeURIComponent($("#upDir").value),{method:"POST",body:fd})
.then(function(r){if(!r.ok)throw 0;toast("Uploaded "+f.name);$("#upFile").value="";loadFiles();})
.catch(function(){toast("Upload failed");}).then(function(){$("#upBtn").disabled=false;$("#upNote").textContent="";});});
// toast
var tt;function toast(m){$("#tmsg").textContent=m;$("#toast").classList.add("show");clearTimeout(tt);tt=setTimeout(function(){$("#toast").classList.remove("show");},2200);}
// login
$("#tok").addEventListener("input",function(e){e.target.value=e.target.value.toUpperCase().replace(/[^0-9A-Z]/g,"");$("#ge").textContent="";});
$("#gf").addEventListener("submit",function(e){e.preventDefault();var v=$("#tok").value.trim();if(v.length<6){$("#ge").textContent="Enter the 6-char token";return;}
fetch("/api/login",{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:"token="+encodeURIComponent(v)})
.then(function(r){if(!r.ok)throw 0;$("#gate").hidden=true;$("#app").hidden=false;return load();})
.catch(function(){$("#ge").textContent="Wrong token — check the badge screen";});});
setTimeout(function(){$("#tok").focus();},60);
// load config
function load(){return fetch("/api/config").then(function(r){return r.json();}).then(function(c){
$("#freq").value=c.frequency;$("#preset").value=c.preset;$("#bright").value=c.brightness;rb();
$("#uname").value=c.user_name;$("#unick").value=c.user_nick;
$("#apssid").value=c.ap_ssid;$("#stassid").value=c.sta_ssid;$("#mdns").value=c.mdns;
setMode(c.mode);$("#pAddr").textContent=c.ip;$("#pMode").textContent=(c.mode==0?"AP":"STA");});}
// save config
$("#save").addEventListener("click",function(){
var mode=$$("#seg button").filter(function(b){return b.getAttribute("aria-pressed")=="true";})[0].dataset.m;
var p=["frequency="+$("#freq").value,"preset="+$("#preset").value,"brightness="+$("#bright").value,
"user_name="+encodeURIComponent($("#uname").value),"user_nick="+encodeURIComponent($("#unick").value),
"mode="+mode,"ap_ssid="+encodeURIComponent($("#apssid").value),"sta_ssid="+encodeURIComponent($("#stassid").value)];
if($("#appass").value)p.push("ap_pass="+encodeURIComponent($("#appass").value));
if($("#stapass").value)p.push("sta_pass="+encodeURIComponent($("#stapass").value));
$("#save").disabled=true;
fetch("/api/config",{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:p.join("&")})
.then(function(r){if(!r.ok)throw 0;toast("Saved to device");}).catch(function(){toast("Save failed");})
.then(function(){$("#save").disabled=false;$("#appass").value="";$("#stapass").value="";});});
</script></body></html>)HTMLDOC";

#endif
