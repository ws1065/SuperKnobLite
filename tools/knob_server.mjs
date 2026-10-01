#!/usr/bin/env node
/**
 * SuperKnobLite 电脑资源采集服务（零依赖，仅用 Node 内置模块）
 *
 * 作用：浏览器 JS 拿不到系统级 CPU/内存占用，这个服务采样真实系统指标，
 *      页面每秒拉一次并通过串口推给设备的「电脑监测」页显示。
 *
 * 启动：node tools/knob_server.mjs        （默认 http://127.0.0.1:8123）
 * 接口：GET /api/stats  → {cpu, memory, ...}
 */
import http from 'node:http';
import os from 'node:os';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const PAGE = path.join(__dirname, '..', 'superknob_control.html');
const PORT = Number(process.env.PORT || 8123);
const HOST = process.env.HOST || '127.0.0.1';

/* ---------- CPU：os.cpus() 累计时间差值 ---------- */
let lastCpu = os.cpus(), lastTime = Date.now();
function sampleCpu() {
  const now = os.cpus(), t = Date.now();
  let idle = 0, total = 0;
  for (let i = 0; i < now.length; i++) {
    for (const k of ['user', 'nice', 'sys', 'idle', 'irq']) {
      const d = (now[i].times[k] || 0) - (lastCpu[i].times[k] || 0);
      total += d;
      if (k === 'idle' || k === 'irq') idle += d;
    }
  }
  lastCpu = now; lastTime = t;
  if (total <= 0) return 0;
  return Math.max(0, Math.min(100, (1 - idle / total) * 100));
}
function sampleMemory() {
  const total = os.totalmem(), free = os.freemem();
  return Math.max(0, Math.min(100, (total - free) / total * 100));
}

const state = { cpu: 0, memory: 0, ts: 0 };
setInterval(() => {
  state.cpu = sampleCpu();
  state.memory = sampleMemory();
  state.ts = Date.now();
}, 500);

function stats() {
  const total = os.totalmem(), free = os.freemem();
  return {
    cpu: +state.cpu.toFixed(1),
    memory: +state.memory.toFixed(1),
    memUsedGB: +((total - free) / 1024 ** 3).toFixed(2),
    memTotalGB: +(total / 1024 ** 3).toFixed(2),
    cores: os.cpus().length,
    loadavg: os.loadavg().map(v => +v.toFixed(2)),
    host: os.hostname(),
    uptime: Math.round(process.uptime()),
    ts: state.ts || Date.now(),
  };
}

/* ---------- HTTP ---------- */
http.createServer((req, res) => {
  const url = new URL(req.url, `http://${req.headers.host}`);
  res.setHeader('Access-Control-Allow-Origin', '*');

  if (url.pathname === '/api/stats') {
    res.setHeader('Content-Type', 'application/json; charset=utf-8');
    res.end(JSON.stringify(stats()));
    return;
  }
  if (url.pathname === '/' || url.pathname === '/superknob_control.html') {
    try {
      const html = fs.readFileSync(PAGE);
      res.setHeader('Content-Type', 'text/html; charset=utf-8');
      res.end(html);
      return;
    } catch (e) {
      res.statusCode = 500;
      res.end('page not found: ' + PAGE);
      return;
    }
  }
  res.statusCode = 404;
  res.end('not found');
}).listen(PORT, HOST, () => {
  const s = stats();
  console.log(`SuperKnobLite 资源服务已启动  →  http://${HOST}:${PORT}/`);
  console.log(`  资源接口: http://${HOST}:${PORT}/api/stats`);
  console.log(`  当前采样: CPU ${s.cpu}%  内存 ${s.memory}%  (${s.cores} 核)`);
  console.log('  Ctrl+C 停止');
});