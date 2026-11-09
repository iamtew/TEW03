// PixiJS backdrop: catalog tewScene, full-viewport, no CDN.
const C = {
  tewYellow: 0xe8c200, tewGreen: 0x22b14c, tewRed: 0xff2a00, cream: 0xf4ead0,
};
const reduceMotion = matchMedia('(prefers-reduced-motion: reduce)').matches;
const rand = (lo, hi) => lo + Math.random() * (hi - lo);

const pointer = { x: 0.5, y: 0.5 };
addEventListener('pointermove', (e) => {
  pointer.x = e.clientX / innerWidth;
  pointer.y = e.clientY / innerHeight;
}, { passive: true });

start().catch((err) => {
  console.info('fx: animated backdrop disabled:', err?.message || err);
  document.getElementById('fx')?.remove();
});

async function start() {
  const PIXI = await import('./vendor/pixi.min.mjs');
  const app = new PIXI.Application();
  await app.init({
    canvas: document.getElementById('fx'),
    resizeTo: window,
    backgroundAlpha: 0,
    antialias: true,
    autoDensity: true,
    resolution: Math.min(devicePixelRatio || 1, 2),
    preference: 'webgl',
  });

  const scene = tewScene(PIXI);
  app.stage.addChild(scene.root);
  bootTicker(app, scene);
}

function bootTicker(app, scene) {
  if (reduceMotion) {
    app.ticker.stop();
    const still = () => {
      const { width: w, height: h } = app.screen;
      scene.update(8, w, h, 0);
      app.render();
    };
    addEventListener('resize', () => requestAnimationFrame(still));
    still();
    return;
  }

  let t = 0;
  app.ticker.add((ticker) => {
    const dt = Math.min(ticker.deltaMS / 1000, 0.05);
    t += dt;
    const { width: w, height: h } = app.screen;
    const ease = Math.min(1, dt * 2.5);
    scene.root.x += ((0.5 - pointer.x) * 40 - scene.root.x) * ease;
    scene.root.y += ((0.5 - pointer.y) * 24 - scene.root.y) * ease;
    scene.update(t, w, h, dt);
  });

  document.addEventListener('visibilitychange', () => {
    if (document.hidden) app.ticker.stop(); else app.ticker.start();
  });
}

const GLOW_PASSES = [[22, 0.05], [10, 0.14], [2.5, 1]];
function glowLine(g, pts, color, scale = 1, passes = GLOW_PASSES) {
  for (const [width, alpha] of passes) {
    g.moveTo(pts[0], pts[1]);
    for (let i = 2; i < pts.length; i += 2) g.lineTo(pts[i], pts[i + 1]);
    g.stroke({ width: width * scale, color, alpha, cap: 'round', join: 'round' });
  }
}

function tewScene(PIXI) {
  const root = new PIXI.Container();
  const grid = new PIXI.Graphics();
  const notes = new PIXI.Graphics();
  const wave = new PIXI.Graphics();
  notes.blendMode = 'add';
  wave.blendMode = 'add';
  root.addChild(grid, notes, wave);

  const ROWS = 12;
  const blocks = Array.from({ length: 26 }, () => ({
    row: Math.floor(rand(0, ROWS)),
    x: rand(0, 2400),
    red: Math.random() < 0.45,
    lit: 0,
  }));

  return {
    root,
    update(t, w, h, dt) {
      const rowH = (h * 0.8) / ROWS, top = h * 0.1, head = w * 0.62;

      grid.clear();
      for (let r = 0; r <= ROWS; r++) grid.moveTo(0, top + r * rowH).lineTo(w, top + r * rowH);
      const step = 64, off = (t * 90) % step;
      for (let x = off - step; x < w; x += step) grid.moveTo(x, top).lineTo(x, top + ROWS * rowH);
      grid.stroke({ width: 1, color: C.tewYellow, alpha: 0.07 });
      grid.moveTo(head, top).lineTo(head, top + ROWS * rowH);
      grid.stroke({ width: 2, color: C.tewRed, alpha: 0.35 });

      notes.clear();
      for (const b of blocks) {
        b.x += 90 * dt;
        if (b.x > w + 120) {
          b.x = -rand(0, w * 0.6);
          b.row = Math.floor(rand(0, ROWS));
          b.red = Math.random() < 0.45;
        }
        if (b.x < head && b.x + 64 > head) b.lit = 1; else b.lit *= Math.exp(-dt * 3);
        const y = top + b.row * rowH + rowH * 0.2, bh = rowH * 0.6;
        const color = b.red ? C.tewRed : C.cream;
        notes.roundRect(b.x - 6, y - 6, 76, bh + 12, 8).fill({ color, alpha: 0.06 + 0.25 * b.lit });
        notes.roundRect(b.x, y, 64, bh, 4).fill({ color, alpha: 0.35 + 0.6 * b.lit });
      }

      wave.clear();
      const cut = 0.45 + 0.12 * Math.sin(t * 0.18);
      const period = 260;
      const amp = h * 0.16, cy = h * 0.55, scroll = t * 140;
      const main = [], echo = [];
      for (let x = -10; x <= w + 10; x += 5) {
        const ph = (((x - scroll) / period) % 1 + 1) % 1;
        const saw = 1 - 2 * ph;
        const ring = Math.exp(-ph * (7 - 5 * cut)) * Math.sin(ph * (20 + 8 * cut) * Math.PI);
        main.push(x, cy + amp * (saw * (0.35 + 0.4 * cut) + ring * 0.7));
        const ph2 = (ph + 0.5) % 1;
        echo.push(x, cy + amp * 0.6 * ((1 - 2 * ph2) * 0.5 + Math.exp(-ph2 * 5) * Math.sin(ph2 * 30 * Math.PI) * 0.5));
      }
      glowLine(wave, echo, C.tewGreen, 0.7);
      glowLine(wave, main, C.tewYellow, 1 + 0.4 * cut);
    },
  };
}
