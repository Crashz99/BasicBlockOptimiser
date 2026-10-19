#!/usr/bin/env python3
"""
server.py - website front end for the Lex/Yacc optimizer.
Run:   python3 server.py          then open http://localhost:8000
It builds v1, v2 and fe from the .y/.l files in this folder (needs yacc/bison, lex/flex, gcc) the first time,
then runs the real compiled programs for every request. Python standard library only.
"""
import http.server, socketserver, json, subprocess, os, re, shutil, sys, webbrowser

BASE = os.path.dirname(os.path.abspath(__file__))
EXE = '.exe' if os.name == 'nt' else ''
PAGE = r'''<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Basic-Block Optimizer</title>
<style>
:root{--bg:#f4f6f9;--card:#fff;--ink:#1b1f2a;--mute:#5b6577;--line:#dce3eb;--blue:#1f6feb;--navy:#0f3d5e;--orange:#e8703a;--green:#2e9e6b;--red:#c23b3b}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--ink);font:15px/1.5 system-ui,-apple-system,Segoe UI,Roboto,sans-serif}
header{background:var(--navy);color:#fff;padding:18px 24px}header h1{margin:0;font-size:22px}header p{margin:4px 0 0;color:#bfd4ea;font-size:14px}
main{max-width:1200px;margin:0 auto;padding:20px}
.card{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:16px;margin-bottom:16px}
.row{display:flex;gap:16px;flex-wrap:wrap}.col{flex:1 1 420px;min-width:0}
h2{font-size:16px;margin:0 0 10px;color:var(--navy)}h3{font-size:14px;margin:12px 0 6px;color:var(--navy)}
textarea{width:100%;height:230px;font:13px/1.45 Consolas,Menlo,monospace;border:1px solid var(--line);border-radius:8px;padding:10px;resize:vertical}
.seg{display:inline-flex;border:1px solid var(--line);border-radius:8px;overflow:hidden;margin-right:10px}
.seg button{border:0;background:#fff;padding:7px 14px;cursor:pointer;font-size:14px;color:var(--mute)}
.seg button.on{background:var(--blue);color:#fff}
select,.go{font-size:14px;padding:7px 12px;border-radius:8px;border:1px solid var(--line);background:#fff}
.go{background:var(--orange);color:#fff;border:0;font-weight:600;padding:9px 22px;cursor:pointer;font-size:15px}.go:disabled{opacity:.6}
.bar{display:flex;align-items:center;flex-wrap:wrap;gap:10px;margin-top:10px}
.stats{display:flex;gap:12px;flex-wrap:wrap}.stat{flex:1 1 170px;background:#f2f5f8;border-radius:10px;padding:12px}
.stat b{display:block;font-size:24px;color:var(--navy)}.stat span{color:var(--mute);font-size:13px}
.ok{color:var(--green)}.bad{color:var(--red)}
pre{margin:0;font:12.5px/1.5 Consolas,Menlo,monospace;white-space:pre-wrap}
.blk{border:1px solid var(--line);border-left:4px solid var(--blue);border-radius:6px;padding:6px 10px;margin:6px 0;background:#fbfcfd}
.blk.loop{border-left-color:var(--orange)}.blk .h{font-weight:700;color:var(--navy);font-size:12px;margin-bottom:2px}
.notes li{margin:5px 0}.tag{display:inline-block;background:#e8f0fe;color:var(--blue);border-radius:6px;padding:1px 7px;font-size:12px;font-weight:600}
.tag.o{background:#fdeee6;color:var(--orange)}
details{border:1px solid var(--line);border-radius:8px;padding:8px 12px;margin:6px 0;background:#fbfcfd}summary{cursor:pointer;font-weight:600}
.err{background:#fdeaea;border:1px solid #f3b9b9;color:#7a1f1f;border-radius:8px;padding:12px}
svg text{font-family:Consolas,Menlo,monospace}
table{border-collapse:collapse;font-size:13px}td,th{border:1px solid var(--line);padding:4px 10px;text-align:left}
.small{font-size:13px;color:var(--mute)}
</style></head><body>
<header><h1>Basic-Block Optimizer</h1><p>Type three-address code (or simple C-like code). The real Lex/Yacc optimizer runs it and shows what changed, in plain English.</p></header>
<main>
<div class="card">
  <div class="bar" style="margin:0 0 10px">
    <div class="seg" id="verSeg"><button data-v="v1">v1: one block at a time</button><button data-v="v2" class="on">v2: whole program</button></div>
    <div class="seg" id="modeSeg"><button data-m="tac" class="on">Input: TAC</button><button data-m="src">Input: C-like code</button></div>
    <select id="ex"></select>
  </div>
  <textarea id="code" spellcheck="false"></textarea>
  <div class="bar"><button class="go" id="go">Optimize</button><span class="small" id="hint"></span></div>
</div>
<div id="out"></div>
<div class="card"><h2>What do these words mean? (plain English)</h2>
<details><summary>Basic block</summary><p>A chunk of code with no jumps in the middle. You enter at the top and leave at the bottom. Each box in the diagrams is one basic block (B0, B1, ...).</p></details>
<details><summary>CFG (control-flow graph)</summary><p>A map of the blocks. An arrow from one block to another means "the program can go from here to there next" (after an <code>if</code> or <code>goto</code>, or just by running on to the next block).</p></details>
<details><summary>Back edge (the "go around again" arrow)</summary><p>An arrow that jumps <b>back</b> to a block that already ran earlier. That is what a loop is made of: without a back edge there is no loop. The orange arrows in the diagrams are back edges.</p></details>
<details><summary>Loop / loop header</summary><p>The blocks that keep running over and over because of a back edge. The <b>header</b> is the block at the top of the loop where every trip around starts (the loop test, like <code>while (i &lt; 10)</code>).</p></details>
<details><summary>Dominator (a "gatekeeper")</summary><p>Block A <b>dominates</b> block B if you cannot get to B without going through A first. Example: the loop header dominates the loop body, because every trip into the body starts at the header. The compiler uses this to prove an arrow really is a back edge (a loop) and not just a jump forward. The "immediate dominator" is the closest gatekeeper.</p></details>
<details><summary>Constant folding</summary><p>Doing the maths at compile time. <code>b = 2 * 3</code> becomes <code>b = 6</code>.</p></details>
<details><summary>Algebraic simplification</summary><p>Using school-maths rules: <code>x + 0 → x</code>, <code>x * 1 → x</code>, <code>x - x → 0</code>, <code>x * 2 → x + x</code>.</p></details>
<details><summary>Common subexpression elimination (CSE)</summary><p>If <code>a + b</code> was already worked out and neither <code>a</code> nor <code>b</code> changed since, reuse the old answer instead of calculating again. If a variable gets a new value, the remembered answer is forgotten (the "kill" rule).</p></details>
<details><summary>Dead code</summary><p>An instruction whose result is never used (or code that can never run). Safe to delete. v2 only treats the <b>returned value</b> as what matters, so anything that does not affect it is dead.</p></details>
<details><summary>"Verified 400/400"</summary><p>The tool runs your original code and the optimized code on 400 random inputs and checks both give the same answer. 400/400 means no difference was found.</p></details>
</div>
</main>
<script>
const $=s=>document.querySelector(s);
const EX={tac:{
"Loop with constants":"n = 10\nk = 2 * 3\ni = 0\ns = 0\nL1:\nif i >= n goto L2\nt1 = k * 4\ns = s + t1\ni = i + 1\ngoto L1\nL2:\nreturn s",
"Reuse across an if/else":"t1 = a + b\nif c < 0 goto L1\nx = a + b\ngoto L2\nL1:\nx = b + a\nL2:\ny = a + b\nr = x + y\nreturn r",
"Useless assignments":"x = a * b\ny = c + d\nw = x * 0\nif a < 0 goto L1\nz = x + 1\ngoto L2\nL1:\nz = x - 1\nL2:\nreturn z",
"Simple maths":"a = 4\nb = 2 * 3\nt1 = a + b\nt2 = b + a\nc = t1 * 1\nd = c + 0\ne = d - d\nreturn c"},
src:{
"While loop":"n = 10;\nk = 2 * 3;\ni = 0;\ns = 0;\nwhile (i < n) {\n    s = s + k * 4;\n    i = i + 1;\n}\nreturn s;",
"If / else":"a = 4;\nb = (a + 2) * 3;\nif (a < b) {\n    c = a + b;\n} else {\n    c = a - b;\n}\nd = a + b;\nreturn c + d;"}};
let ver='v2',mode='tac';
function fillEx(){const s=$('#ex');s.innerHTML='';Object.keys(EX[mode]).forEach(k=>{const o=document.createElement('option');o.textContent='Example: '+k;o.value=k;s.appendChild(o)});$('#code').value=EX[mode][s.value];
 $('#hint').textContent=mode==='tac'?'One instruction per line: x = a + b, labels (L1:), goto L1, if a < b goto L1, return x':'Use ; after each statement. Supports + - * / %, if/else, while, { }, return.'}
$('#ex').onchange=()=>{$('#code').value=EX[mode][$('#ex').value]};
document.querySelectorAll('#verSeg button').forEach(b=>b.onclick=()=>{ver=b.dataset.v;document.querySelectorAll('#verSeg button').forEach(x=>x.classList.toggle('on',x===b))});
document.querySelectorAll('#modeSeg button').forEach(b=>b.onclick=()=>{mode=b.dataset.m;document.querySelectorAll('#modeSeg button').forEach(x=>x.classList.toggle('on',x===b));fillEx()});
fillEx();
$('#go').onclick=async()=>{const g=$('#go');g.disabled=true;g.textContent='Running...';
 try{const r=await fetch('/api/optimize',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({mode,version:ver,code:$('#code').value})});render(await r.json())}
 catch(e){$('#out').innerHTML='<div class="err">Could not reach the server: '+e+'</div>'}
 g.disabled=false;g.textContent='Optimize'};

function esc(s){return s.replace(/[&<>]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;'}[c]))}
function analyze(bl){
 const n=bl.length,pred=bl.map(()=>[]);bl.forEach(b=>b.succ.forEach(s=>pred[s].push(b.id)));
 const reach=new Set([0]),st=[0];while(st.length){const u=st.pop();bl[u].succ.forEach(v=>{if(!reach.has(v)){reach.add(v);st.push(v)}})}
 const dom=bl.map((_,i)=>i===0?new Set([0]):new Set(reach));let ch=true;
 while(ch){ch=false;for(const i of reach){if(i===0)continue;let nw=null;for(const p of pred[i]){if(!reach.has(p))continue;nw=nw?new Set([...nw].filter(x=>dom[p].has(x))):new Set(dom[p])}
  nw=nw||new Set();nw.add(i);if(nw.size!==dom[i].size||[...nw].some(x=>!dom[i].has(x))){dom[i]=nw;ch=true}}}
 const back=[];for(const t of reach)for(const h of bl[t].succ)if(dom[t].has(h))back.push([t,h]);
 const loops={};for(const [t,h] of back){const body=loops[h]||(loops[h]=new Set([h]));const stack=[t];body.add(t);
  while(stack.length){const m=stack.pop();for(const p of pred[m])if(reach.has(p)&&!body.has(p)){body.add(p);stack.push(p)}}}
 const idom={};for(const i of reach){if(i===0)continue;let best=-1;for(const d of dom[i])if(d!==i&&(best<0||dom[d].size>dom[best].size))best=d;idom[i]=best}
 return{back,loops,idom,reach}}
function drawCFG(bl,an){
 const W=175,GAP=34,LH=15,n=bl.length;if(!n)return'';
 const isBack=(u,v)=>an.back.some(e=>e[0]===u&&e[1]===v);
 const lvl=new Array(n).fill(0);for(let k=0;k<n;k++)bl.forEach(b=>b.succ.forEach(s=>{if(!isBack(b.id,s)&&lvl[s]<lvl[b.id]+1)lvl[s]=lvl[b.id]+1}));
 const lines=bl.map(b=>{const a=b.ins.slice(0,6).map(x=>x.length>24?x.slice(0,23)+'…':x);if(b.ins.length>6)a.push('… +'+(b.ins.length-6)+' more');return a});
 const H=bl.map((b,i)=>26+LH*Math.max(1,lines[i].length));
 const L=Math.max(...lvl)+1,byL=[...Array(L)].map(()=>[]);bl.forEach(b=>byL[lvl[b.id]].push(b.id));
 const maxc=Math.max(...byL.map(a=>a.length)),totW=maxc*(W+GAP)+120;const pos={};let y=16;
 byL.forEach(ids=>{const mh=Math.max(0,...ids.map(i=>H[i]));ids.forEach((i,k)=>{pos[i]={x:(totW-120)/2-(ids.length*(W+GAP)-GAP)/2+k*(W+GAP),y}});y+=mh+46});
 const inLoop=new Set();Object.values(an.loops).forEach(s=>s.forEach(x=>inLoop.add(x)));
 let svg=`<svg viewBox="0 0 ${totW} ${y}" width="100%" style="max-height:${Math.min(y,620)}px"><defs><marker id="ar" markerWidth="8" markerHeight="8" refX="7" refY="4" orient="auto"><path d="M0,0 L8,4 L0,8 z" fill="#8a94a6"/></marker><marker id="ao" markerWidth="8" markerHeight="8" refX="7" refY="4" orient="auto"><path d="M0,0 L8,4 L0,8 z" fill="#e8703a"/></marker></defs>`;
 bl.forEach(b=>b.succ.forEach(s=>{const u=pos[b.id],v=pos[s];if(isBack(b.id,s)){const x1=u.x+W,y1=u.y+H[b.id]/2,x2=v.x+W,y2=v.y+H[s]/2,o=Math.max(x1,x2)+55;
  svg+=`<path d="M${x1},${y1} C${o},${y1} ${o},${y2} ${x2+1},${y2}" fill="none" stroke="#e8703a" stroke-width="2" marker-end="url(#ao)"/><text x="${o-6}" y="${(y1+y2)/2}" font-size="11" fill="#e8703a" font-weight="700">back edge</text>`}
  else{svg+=`<line x1="${u.x+W/2}" y1="${u.y+H[b.id]}" x2="${v.x+W/2}" y2="${v.y-1}" stroke="#8a94a6" stroke-width="1.6" marker-end="url(#ar)"/>`}}));
 bl.forEach((b,i)=>{const p=pos[i],lp=inLoop.has(i),c=lp?'#e8703a':'#1f6feb';
  svg+=`<rect x="${p.x}" y="${p.y}" width="${W}" height="${H[i]}" rx="7" fill="#fff" stroke="${c}" stroke-width="2"/><rect x="${p.x}" y="${p.y}" width="${W}" height="20" rx="7" fill="${c}"/><text x="${p.x+8}" y="${p.y+14}" font-size="12" fill="#fff" font-weight="700">B${i}${lp?'  (in loop)':''}</text>`;
  lines[i].forEach((t,k)=>{svg+=`<text x="${p.x+8}" y="${p.y+36+k*LH}" font-size="11" fill="#1b1f2a">${esc(t)}</text>`})});
 return svg+'</svg>'}
function side(title,bl,color){const an=analyze(bl),inLoop=new Set();Object.values(an.loops).forEach(s=>s.forEach(x=>inLoop.add(x)));
 let h=`<div class="col card"><h2 style="color:${color}">${title}</h2>`;
 bl.forEach(b=>{h+=`<div class="blk ${inLoop.has(b.id)?'loop':''}"><div class="h">B${b.id} → ${b.succ.length?b.succ.map(s=>'B'+s).join(', '):'end of program'}</div><pre>${esc(b.ins.join('\n'))}</pre></div>`});
 h+=`<h3>Control-flow graph</h3>${drawCFG(bl,an)}`;
 h+=`<h3>Loops</h3>`;const hs=Object.keys(an.loops);
 if(!hs.length)h+=`<p class="small">No loops: the code only moves forward (with branches). No back edge exists.</p>`;
 else hs.forEach(k=>{const body=[...an.loops[k]].sort((a,b)=>a-b),be=an.back.filter(e=>e[1]==k).map(e=>`B${e[0]}→B${e[1]}`).join(', ');
  h+=`<p><span class="tag o">loop</span> Blocks <b>${body.map(x=>'B'+x).join(', ')}</b> repeat. <b>B${k}</b> is the loop header (where each trip starts). The back edge <b>${be}</b> is the jump that sends control back to the top for another go.</p>`});
 const ids=Object.keys(an.idom);if(ids.length){h+=`<h3>Dominators (gatekeepers)</h3><table><tr><th>Block</th><th>Must pass through first</th></tr>`;
  ids.forEach(i=>h+=`<tr><td>B${i}</td><td>B${an.idom[i]}</td></tr>`);h+=`</table><p class="small">"Must pass through first" = the immediate dominator: you cannot reach the block without going through that one.</p>`}
 return h+'</div>'}
function notes(d){const s=d.stats,a=[];const L={ 'constant folding':n=>`Did the maths at compile time <b>${n}</b> time(s) (like <code>2 * 3</code> becomes <code>6</code>).`,
 'algebraic simplification':n=>`Simplified <b>${n}</b> expression(s) with maths rules (<code>x + 0</code> → <code>x</code>, <code>x * 1</code> → <code>x</code>, <code>x - x</code> → <code>0</code>).`,
 'local CSE':n=>`Reused <b>${n}</b> value(s) already worked out earlier in the same block instead of calculating again.`,
 'global CSE':n=>`Reused <b>${n}</b> value(s) that were already worked out on <i>every</i> path leading here (even in other blocks).`,
 'global constants propagated':n=>`Swapped variables for their known constant value <b>${n}</b> time(s), even across blocks and into loops.`,
 'branch folded':n=>`<b>${n}</b> <code>if</code> test(s) had a known answer, so the branch was removed.`,
 'unreachable blocks removed':n=>`Deleted <b>${n}</b> block(s) that could never run.`,
 'dead code removed':n=>`Deleted <b>${n}</b> instruction(s) whose result is never used.`,
 'self-copy removed':n=>`Removed <b>${n}</b> pointless <code>x = x</code> instruction(s).`};
 Object.keys(L).forEach(k=>{if(s.counts[k])a.push(L[k](s.counts[k]))});
 if(!a.length)a.push('Nothing could be improved safely: the code was already tight.');
 return a.map(x=>`<li>${x}</li>`).join('')}
function render(d){const o=$('#out');
 if(d.error){o.innerHTML=`<div class="err"><b>Problem:</b><pre>${esc(d.error)}</pre></div>`;return}
 const s=d.stats,vOk=s.good===s.total;
 let h='';
 if(d.tac)h+=`<div class="card"><h2>Step 1: your code turned into three-address code (front end)</h2><pre>${esc(d.tac)}</pre></div>`;
 h+=`<div class="card"><h2>Result (${d.version==='v2'?'v2: whole-program optimizer':'v1: one block at a time'})</h2><div class="stats">
 <div class="stat"><b>${s.before} → ${s.after}</b><span>instructions (${s.pct}% fewer)</span></div>
 <div class="stat"><b>${s.blocks0} → ${s.blocks1}</b><span>basic blocks</span></div>
 ${s.rounds?`<div class="stat"><b>${s.rounds}</b><span>rounds until nothing changed</span></div>`:''}
 <div class="stat"><b class="${vOk?'ok':'bad'}">${vOk?'✓':'✗'} ${s.good}/${s.total}</b><span>random test runs match the original</span></div></div>
 <h3>What it did</h3><ul class="notes">${notes(d)}</ul></div>`;
 h+=`<div class="row">${side('Before',d.before,'#c23b3b')}${side('After',d.after,'#2e9e6b')}</div>`;
 h+=`<details><summary>Raw output from the C program (the real Lex/Yacc optimizer)</summary><pre>${esc(d.raw)}</pre></details>`;
 o.innerHTML=h;o.scrollIntoView({behavior:'smooth'})}
</script></body></html>
'''

def which(*names):
    for n in names:
        if shutil.which(n): return n
    return None

def ensure_built():
    yacc = [['bison', '-y', '-d'], ['win_bison', '-y', '-d'], ['yacc', '-d']]
    lex = ['flex', 'win_flex', 'lex']
    yc = next((c for c in yacc if shutil.which(c[0])), None); lc = which(*lex); gcc = which('gcc', 'cc')
    for name in ('v1', 'v2', 'fe'):
        out = os.path.join(BASE, name + EXE)
        if os.path.exists(out): continue
        y, l = os.path.join(BASE, name + '.y'), os.path.join(BASE, name + '.l')
        if not (os.path.exists(y) and os.path.exists(l)): print(f'[!] missing {name}.y / {name}.l'); continue
        if not (yc and lc and gcc): print('[!] need yacc/bison, lex/flex and gcc installed to build', name); continue
        d = os.path.join(BASE, '.build_' + name); os.makedirs(d, exist_ok=True)
        try:
            subprocess.run(yc + [y], cwd=d, check=True, capture_output=True)
            subprocess.run([lc, l], cwd=d, check=True, capture_output=True)
            subprocess.run([gcc, 'y.tab.c', 'lex.yy.c', '-o', out], cwd=d, check=True, capture_output=True)
            print(f'built {name}')
        except subprocess.CalledProcessError as e:
            print(f'[!] build of {name} failed:', (e.stderr or b'').decode(errors='replace')[:500])

def run_bin(name, text):
    exe = os.path.join(BASE, name + EXE)
    if not os.path.exists(exe): return None, f'{name} is not built. Install yacc/bison, lex/flex and gcc, then restart server.py.'
    p = subprocess.run([exe], input=text, capture_output=True, text=True, timeout=15)
    return p, None

def parse_blocks(lines):
    blocks, cur = [], None
    for ln in lines:
        m = re.match(r'^B(\d+) ->(.*)$', ln)
        if m:
            cur = {'id': int(m.group(1)), 'succ': [int(x) for x in re.findall(r'B(\d+)', m.group(2))], 'ins': []}; blocks.append(cur)
        elif cur is not None and ln.startswith('    '): cur['ins'].append(ln.strip())
    return blocks

def parse_output(out):
    sec, data = None, {'before': [], 'after': [], 'stats': []}
    for ln in out.splitlines():
        if ln.startswith('=== '):
            t = ln.strip('= ').upper()
            sec = 'before' if t.startswith('BEFORE') else 'after' if t.startswith('AFTER') else 'stats' if t.startswith('STATISTICS') else None
            continue
        if sec: data[sec].append(ln)
    st = ' '.join(data['stats'])
    m = re.search(r'instructions: (\d+) -> (\d+) \(([\d.]+)% fewer\), blocks (\d+) -> (\d+)(?:, rounds to fixed point: (\d+))?', st)
    v = re.search(r'verification: (\d+)/(\d+)', st)
    if not m or not v or not data['before']: return None
    counts = {k.strip(): int(n) for k, n in re.findall(r'^\s+([a-zA-Z\- ]+): (\d+)$', '\n'.join(data['stats']), re.M)}
    return {'before': parse_blocks(data['before']), 'after': parse_blocks(data['after']),
            'stats': {'before': int(m.group(1)), 'after': int(m.group(2)), 'pct': m.group(3), 'blocks0': int(m.group(4)), 'blocks1': int(m.group(5)),
                      'rounds': int(m.group(6)) if m.group(6) else 0, 'good': int(v.group(1)), 'total': int(v.group(2)), 'counts': counts}}

def optimize(req):
    code, mode, ver = req.get('code', ''), req.get('mode', 'tac'), req.get('version', 'v2')
    if ver not in ('v1', 'v2'): return {'error': 'unknown version'}
    if len(code) > 20000 or not code.strip(): return {'error': 'Please enter some code (max 20000 characters).'}
    tac = None
    if mode == 'src':
        p, err = run_bin('fe', code)
        if err: return {'error': err}
        if p.returncode != 0 or not p.stdout.strip(): return {'error': 'Front end (source to TAC) failed:\n' + (p.stderr or 'no output')}
        tac = p.stdout; code = tac
    p, err = run_bin(ver, code)
    if err: return {'error': err}
    res = parse_output(p.stdout)
    if not res: return {'error': (p.stderr.strip() or 'The optimizer produced no result.') + '\n\nCheck your input format (one TAC instruction per line, e.g.  x = a + b).'}
    res.update({'tac': tac, 'version': ver, 'raw': p.stdout}); return res

class H(http.server.BaseHTTPRequestHandler):
    def _send(self, code, body, ctype):
        b = body if isinstance(body, bytes) else body.encode()
        self.send_response(code); self.send_header('Content-Type', ctype); self.send_header('Content-Length', str(len(b))); self.end_headers(); self.wfile.write(b)
    def do_GET(self):
        if self.path in ('/', '/index.html'): self._send(200, PAGE, 'text/html; charset=utf-8')
        else: self._send(404, 'not found', 'text/plain')
    def do_POST(self):
        if self.path != '/api/optimize': return self._send(404, 'not found', 'text/plain')
        try:
            n = int(self.headers.get('Content-Length', 0)); res = optimize(json.loads(self.rfile.read(n) or b'{}'))
        except subprocess.TimeoutExpired: res = {'error': 'Timed out (is there an infinite loop in the input?)'}
        except Exception as e: res = {'error': f'Server error: {e}'}
        self._send(200, json.dumps(res), 'application/json')
    def log_message(self, *a): pass

if __name__ == '__main__':
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
    ensure_built()
    socketserver.ThreadingTCPServer.allow_reuse_address = True
    with socketserver.ThreadingTCPServer(('127.0.0.1', port), H) as srv:
        url = f'http://localhost:{port}'; print(f'Open {url} in your browser   (Ctrl+C to stop)')
        try: webbrowser.open(url)
        except Exception: pass
        try: srv.serve_forever()
        except KeyboardInterrupt: print('\nstopped')
