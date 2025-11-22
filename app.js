/* Config & State */
const state = {
  baseUrl: '',
  streaming: false,
  driveDebounce: null,
  armDebounce: {},
  pollMode: false,
  pollIntervalId: null,
  pollFps: 2,
};

/* DOM refs */
const $ = (s) => document.querySelector(s);
const baseUrlInput = $('#baseUrl');
const saveBaseBtn = $('#saveBase');
const connStatus = $('#connStatus');
const throttleSlider = $('#throttle');
const throttleVal = $('#throttleVal');
const turnLeftBtn = $('#turnLeft');
const turnRightBtn = $('#turnRight');
const stopBtn = $('#stop');
const shoulder = $('#shoulder');
const elbow = $('#elbow');
const grip = $('#grip');
const shoulderVal = $('#shoulderVal');
const elbowVal = $('#elbowVal');
const gripVal = $('#gripVal');
const homePoseBtn = $('#homePose');
const streamUrlInput = $('#streamUrl');
const toggleStreamBtn = $('#toggleStream');
const streamImg = $('#streamImg');
const streamPlaceholder = $('#streamPlaceholder');
const streamError = $('#streamError');
const streamEmbedWrapper = document.getElementById('streamEmbedWrapper');
const streamIframe = document.getElementById('streamIframe');
const pollControls = document.getElementById('pollControls');
const pollFpsSlider = document.getElementById('pollFps');
const pollFpsVal = document.getElementById('pollFpsVal');

/* Helpers */
const clamp = (n, min, max) => Math.min(Math.max(n, min), max);

function setOnline(on) {
  connStatus.textContent = on ? 'Online' : 'Offline';
  connStatus.classList.toggle('online', on);
  connStatus.classList.toggle('offline', !on);
}

function normalizeBase(url) {
  if (!url) return '';
  if (!/^https?:\/\//i.test(url)) url = 'http://' + url;
  return url.replace(/\/$/, '');
}

function buildUrl(path) { return state.baseUrl + path; }

async function ping() {
  if (!state.baseUrl) return setOnline(false);
  try {
    const ctrl = new AbortController();
    const t = setTimeout(() => ctrl.abort(), 1500);
    const res = await fetch(buildUrl('/'), { signal: ctrl.signal });
    clearTimeout(t);
    setOnline(res.ok);
  } catch (_) {
    setOnline(false);
  }
}

function loadSettings() {
  const savedBase = localStorage.getItem('baseUrl') || '';
  const savedStream = localStorage.getItem('streamUrl') || '';
  state.baseUrl = normalizeBase(savedBase);
  baseUrlInput.value = state.baseUrl;
  streamUrlInput.value = savedStream;
  ping();
}

function saveSettings() {
  state.baseUrl = normalizeBase(baseUrlInput.value.trim());
  localStorage.setItem('baseUrl', state.baseUrl);
  ping();
}

/* Drive logic */
async function sendDrive(cmd, speed) {
  if (!state.baseUrl) return;
  try {
    const url = speed != null ? buildUrl(`/drive?cmd=${cmd}&speed=${speed}`) : buildUrl(`/drive?cmd=${cmd}`);
    await fetch(url);
  } catch (_) {}
}

function handleThrottleInput() {
  const val = parseInt(throttleSlider.value, 10);
  throttleVal.textContent = val;
  clearTimeout(state.driveDebounce);
  state.driveDebounce = setTimeout(() => {
    const magnitude = Math.abs(val);
    if (magnitude === 0) {
      sendDrive('S');
    } else if (val > 0) {
      sendDrive('F', magnitude);
    } else {
      sendDrive('B', magnitude);
    }
  }, 60);
}

function bindTurnHold(btn, dir) {
  const start = () => sendDrive(dir);
  const end = () => sendDrive('S');
  btn.addEventListener('mousedown', start);
  btn.addEventListener('mouseup', end);
  btn.addEventListener('mouseleave', end);
  btn.addEventListener('touchstart', (e) => { e.preventDefault(); start(); }, { passive: false });
  btn.addEventListener('touchend', (e) => { e.preventDefault(); end(); });
  btn.addEventListener('touchcancel', (e) => { e.preventDefault(); end(); });
}

/* Arm logic */
async function sendServo(joint, angle) {
  if (!state.baseUrl) return;
  try { await fetch(buildUrl(`/setServo?joint=${joint}&angle=${angle}`)); } catch (_) {}
}

function bindArm(slider, valEl, joint) {
  const updateLabel = () => valEl.textContent = `${slider.value}°`;
  slider.addEventListener('input', () => {
    updateLabel();
    clearTimeout(state.armDebounce[joint]);
    state.armDebounce[joint] = setTimeout(() => sendServo(joint, clamp(parseInt(slider.value, 10), 0, 180)), 80);
  });
  updateLabel();
}

/* Stream logic (MJPEG + snapshot fallback + iframe) */
async function startStream() {
  let url = streamUrlInput.value.trim();
  if (!url) return;
  if (!/^https?:\/\//i.test(url)) url = 'http://' + url;
  streamError.style.display = 'none';
  streamImg.style.display = 'none';
  streamEmbedWrapper.style.display = 'none';
  if (pollControls) pollControls.style.display = 'none';
  state.pollMode = false;
  if (state.pollIntervalId) { clearInterval(state.pollIntervalId); state.pollIntervalId = null; }

  let parsed;
  try { parsed = new URL(url); } catch (_) { parsed = null; }
  const pathOnly = parsed && (parsed.pathname === '/' || parsed.pathname === '');
  const candidates = pathOnly ? (() => {
    const origin = parsed.origin;
    const origin81 = parsed.port === '81' ? origin : `${parsed.protocol}//${parsed.hostname}:81`;
    return [
      `${origin81}/stream`,
      `${origin}/stream`,
      `${origin}/mjpeg`,
      `${origin}/video`,
      `${origin}/?action=stream`
    ];
  })() : [url];

  const controller = new AbortController();
  async function probe(cand) {
    try {
      const timeout = setTimeout(() => controller.abort(), 1500);
      const res = await fetch(cand, { signal: controller.signal });
      clearTimeout(timeout);
      if (!res.ok) return false;
      const ct = res.headers.get('content-type') || '';
      if (/image\/(jpeg|jpg|png)|multipart\//i.test(ct)) return true;
      if (ct === '') return true;
    } catch (_) { return false; }
    return false;
  }

  let working = null;
  for (const cand of candidates) {
    // eslint-disable-next-line no-await-in-loop
    if (await probe(cand)) { working = cand; break; }
  }

  if (working) {
    streamImg.src = working + (working.includes('?') ? '&' : '?') + 't=' + Date.now();
    streamImg.style.display = 'block';
    streamPlaceholder.style.display = 'none';
    streamUrlInput.value = working;
    if (pollControls) pollControls.style.display = 'none';
  } else {
    const snapshotCandidates = [
      '/capture','/snapshot','/snapshot.jpg','/snap.jpg','/photo.jpg','/jpg','/image.jpg','/bmp']
      .map(p => (url.endsWith('/') ? url.slice(0,-1) : url) + p);
    let snapshotWorking = null;
    for (const shot of snapshotCandidates) {
      try {
        const r = await fetch(shot, { method: 'GET' });
        if (r.ok) {
          const ct = (r.headers.get('content-type')||'').toLowerCase();
          if (ct.includes('image')) { snapshotWorking = shot; break; }
        }
      } catch(_) {}
    }
    if (snapshotWorking) {
      state.pollMode = true;
      streamImg.style.display = 'block';
      streamPlaceholder.style.display = 'none';
      streamError.textContent = 'Using snapshot polling (no MJPEG stream detected).';
      streamError.style.display = 'block';
      if (pollControls) pollControls.style.display = 'flex';
      state.pollFps = parseInt(pollFpsSlider?.value||'2',10);
      if (pollFpsVal) pollFpsVal.textContent = state.pollFps;
      const refresh = () => {
        const bust = snapshotWorking + (snapshotWorking.includes('?')?'&':'?') + 't=' + Date.now();
        streamImg.src = bust;
      };
      refresh();
      const startPolling = () => {
        if (state.pollIntervalId) clearInterval(state.pollIntervalId);
        const interval = Math.max(1000 / state.pollFps, 100);
        state.pollIntervalId = setInterval(refresh, interval);
      };
      startPolling();
      if (pollFpsSlider) {
        pollFpsSlider.oninput = () => {
          state.pollFps = parseInt(pollFpsSlider.value,10);
          if (pollFpsVal) pollFpsVal.textContent = state.pollFps;
          startPolling();
        };
      }
      streamUrlInput.value = snapshotWorking;
    } else {
      streamIframe.src = url;
      streamEmbedWrapper.style.display = 'block';
      streamPlaceholder.style.display = 'none';
      streamError.textContent = 'Raw MJPEG not detected; embedded device page.';
      streamError.style.display = 'block';
      if (pollControls) pollControls.style.display = 'none';
    }
  }
  state.streaming = true;
  toggleStreamBtn.textContent = 'Stop';
  localStorage.setItem('streamUrl', url);
}

function stopStream() {
  streamImg.src = '';
  streamIframe.src = '';
  streamImg.style.display = 'none';
  streamEmbedWrapper.style.display = 'none';
  streamError.style.display = 'none';
  streamPlaceholder.style.display = 'grid';
  if (state.pollIntervalId) { clearInterval(state.pollIntervalId); state.pollIntervalId = null; }
  state.pollMode = false;
  if (pollControls) pollControls.style.display = 'none';
  state.streaming = false;
  toggleStreamBtn.textContent = 'Start';
}

/* Keyboard shortcuts */
function handleKey(e) {
  if (['INPUT', 'TEXTAREA'].includes(document.activeElement.tagName)) return;
  const k = e.key.toLowerCase();
  if (k === 'w') { throttleSlider.value = String(clamp(parseInt(throttleSlider.value, 10) + 10, -100, 100)); handleThrottleInput(); }
  else if (k === 's') { throttleSlider.value = String(clamp(parseInt(throttleSlider.value, 10) - 10, -100, 100)); handleThrottleInput(); }
  else if (k === 'a') { sendDrive('L'); setTimeout(() => sendDrive('S'), 150); }
  else if (k === 'd') { sendDrive('R'); setTimeout(() => sendDrive('S'), 150); }
  else if (k === ' ') { e.preventDefault(); sendDrive('S'); throttleSlider.value = '0'; throttleVal.textContent = '0'; }
}

/* Wire up */
document.addEventListener('DOMContentLoaded', () => {
  loadSettings();
  saveBaseBtn.addEventListener('click', saveSettings);
  baseUrlInput.addEventListener('keydown', (e) => { if (e.key === 'Enter') saveSettings(); });
  throttleSlider.addEventListener('input', handleThrottleInput);
  bindTurnHold(turnLeftBtn, 'L');
  bindTurnHold(turnRightBtn, 'R');
  stopBtn.addEventListener('click', () => { throttleSlider.value = '0'; throttleVal.textContent = '0'; sendDrive('S'); });
  bindArm(shoulder, shoulderVal, 'shoulder');
  bindArm(elbow, elbowVal, 'elbow');
  bindArm(grip, gripVal, 'grip');
  homePoseBtn.addEventListener('click', async () => {
    shoulder.value = elbow.value = grip.value = '90';
    shoulderVal.textContent = elbowVal.textContent = gripVal.textContent = '90°';
    await Promise.all([
      sendServo('shoulder', 90),
      sendServo('elbow', 90),
      sendServo('grip', 90),
    ]);
  });
  toggleStreamBtn.addEventListener('click', () => state.streaming ? stopStream() : startStream());
  document.addEventListener('keydown', handleKey);
  setInterval(ping, 3000);
});
