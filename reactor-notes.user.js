// ==UserScript==
// @name         Reactor Notes
// @namespace    reactor-notes
// @version      1.0
// @description  Small movable note that calculates (x*4)*y for every line with two numbers
// @match        https://www.kongregate.com/*/games/cael/reactor-incremental*
// @match        https://www.kongregate.com/games/cael/reactor-incremental*
// @noframes
// @grant        GM_getValue
// @grant        GM_setValue
// ==/UserScript==

(function () {
  'use strict';

  const fmt = n => Number.isInteger(n) ? n.toLocaleString('en-US') : n.toLocaleString('en-US', { maximumFractionDigits: 4 });
  const calc = line => {
    const nums = line.replace(/,/g, '.').match(/-?\d+(\.\d+)?/g);
    if (!nums || nums.length < 2) return null;
    const x = parseFloat(nums[0]), y = parseFloat(nums[1]);
    return (x * 4) * y;
  };

  const css = `
    #rn-box{position:fixed;z-index:2147483647;width:240px;background:#16181d;color:#e6e8ec;border:1px solid #2c313a;border-radius:8px;
      box-shadow:0 6px 20px rgba(0,0,0,.35);font:13px/20px ui-monospace,Consolas,monospace;user-select:none}
    #rn-head{display:flex;align-items:center;justify-content:space-between;padding:4px 8px;cursor:move;border-bottom:1px solid #2c313a;
      font:600 12px/20px system-ui,sans-serif;color:#9aa3b2;letter-spacing:.02em}
    #rn-min{background:none;border:0;color:#9aa3b2;cursor:pointer;font:16px/1 system-ui;padding:0 4px}
    #rn-min:hover{color:#fff}
    #rn-body{display:flex}
    #rn-text{flex:1;min-width:0;height:140px;resize:vertical;margin:0;padding:6px 8px;border:0;outline:0;background:transparent;color:inherit;
      font:inherit;white-space:pre;overflow-x:hidden;user-select:text}
    #rn-out{width:96px;padding:6px 8px;border-left:1px solid #2c313a;text-align:right;color:#7fd1a8;white-space:pre;overflow:hidden}
    #rn-foot{display:flex;justify-content:space-between;padding:4px 8px;border-top:1px solid #2c313a;color:#9aa3b2;font-size:12px}
    #rn-sum{color:#fff;font-weight:600}
    #rn-box.min #rn-body,#rn-box.min #rn-foot{display:none}`;
  document.head.appendChild(Object.assign(document.createElement('style'), { textContent: css }));

  const box = document.createElement('div');
  box.id = 'rn-box';
  box.innerHTML = `
    <div id="rn-head"><span>(x·4)·y</span><button id="rn-min" title="Collapse">–</button></div>
    <div id="rn-body"><textarea id="rn-text" spellcheck="false" placeholder="10 27"></textarea><div id="rn-out"></div></div>
    <div id="rn-foot"><span>total</span><span id="rn-sum">0</span></div>`;
  document.body.appendChild(box);

  const head = box.querySelector('#rn-head'), text = box.querySelector('#rn-text'),
        out = box.querySelector('#rn-out'), sum = box.querySelector('#rn-sum'), minBtn = box.querySelector('#rn-min');

  const update = () => {
    let total = 0;
    out.textContent = text.value.split('\n').map(l => {
      const r = calc(l);
      if (r === null) return '';
      total += r;
      return fmt(r);
    }).join('\n');
    sum.textContent = fmt(total);
    out.style.height = text.offsetHeight + 'px';
    out.scrollTop = text.scrollTop;
  };

  const clamp = () => {
    const r = box.getBoundingClientRect();
    box.style.left = Math.min(Math.max(0, r.left), innerWidth - r.width) + 'px';
    box.style.top = Math.min(Math.max(0, r.top), innerHeight - head.offsetHeight) + 'px';
  };

  const pos = GM_getValue('pos', null);
  box.style.left = (pos ? pos.x : innerWidth - 260) + 'px';
  box.style.top = (pos ? pos.y : innerHeight - 240) + 'px';
  text.value = GM_getValue('text', '');
  if (GM_getValue('min', false)) box.classList.add('min');
  clamp();
  update();

  text.addEventListener('input', () => { update(); GM_setValue('text', text.value); });
  text.addEventListener('scroll', () => { out.scrollTop = text.scrollTop; });
  new ResizeObserver(() => { out.style.height = text.offsetHeight + 'px'; }).observe(text);
  minBtn.addEventListener('click', () => { GM_setValue('min', box.classList.toggle('min')); });

  head.addEventListener('pointerdown', e => {
    if (e.target === minBtn) return;
    const r = box.getBoundingClientRect(), dx = e.clientX - r.left, dy = e.clientY - r.top;
    head.setPointerCapture(e.pointerId);
    const move = ev => { box.style.left = ev.clientX - dx + 'px'; box.style.top = ev.clientY - dy + 'px'; clamp(); };
    const up = () => {
      head.removeEventListener('pointermove', move);
      head.removeEventListener('pointerup', up);
      GM_setValue('pos', { x: parseFloat(box.style.left), y: parseFloat(box.style.top) });
    };
    head.addEventListener('pointermove', move);
    head.addEventListener('pointerup', up);
  });
  addEventListener('resize', clamp);
})();
