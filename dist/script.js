// Vyom Cosmic Telemetry Controller
(function () {
  let count = 0;
  let totalLatency = 0;

  // DOM Elements
  const winMin = document.getElementById("win-min");
  const winMax = document.getElementById("win-max");
  const winClose = document.getElementById("win-close");

  const counterBtn = document.getElementById("counter-btn");
  const counterText = document.getElementById("counter-text");
  const counterMetric = document.getElementById("counter-metric");

  const btnBenchmark = document.getElementById("btn-benchmark");
  const btnAlert = document.getElementById("btn-alert");
  const streamOut = document.getElementById("stream-out");
  const streamStatus = document.getElementById("stream-status");
  const specOs = document.getElementById("spec-os");
  const specArch = document.getElementById("spec-arch");
  const toastDock = document.getElementById("toast-dock");

  // Helper: Invocation
  async function invoke(cmd, payload = {}) {
    if (window.vyom && typeof window.vyom.invoke === "function") {
      return await window.vyom.invoke(cmd, payload);
    }
    return JSON.stringify({ os: "Windows (x86_64)", arch: "x86_64", mock: true });
  }

  // 1. Frameless Window Controls
  if (winMin) {
    winMin.addEventListener("click", () => window.vyom?.window?.minimize?.());
  }

  if (winMax) {
    winMax.addEventListener("click", () => window.vyom?.window?.toggleMaximize?.());
  }

  if (winClose) {
    winClose.addEventListener("click", () => {
      if (window.vyom?.window?.close) {
        window.vyom.window.close();
      } else {
        window.close();
      }
    });
  }

  // 2. Telemetry Counter Capsule (The Vite Flex)
  if (counterBtn) {
    counterBtn.addEventListener("click", async () => {
      count++;
      counterText.textContent = `count is ${count}`;

      const t0 = performance.now();
      try {
        await invoke("get_sysinfo");
        const dt = (performance.now() - t0).toFixed(2);
        totalLatency += parseFloat(dt);
        counterMetric.textContent = `${dt} ms`;
      } catch (err) {
        counterMetric.textContent = `ERR`;
      }
    });
  }

  // 3. 10x Latency Cycle Benchmark
  if (btnBenchmark) {
    btnBenchmark.addEventListener("click", async () => {
      streamStatus.textContent = "SAMPLING";
      streamStatus.style.color = "var(--starlight-cyan)";
      streamOut.textContent = "// Initiating 10-cycle zero-copy IPC telemetry probe...\n";

      const samples = [];
      for (let i = 1; i <= 10; i++) {
        const start = performance.now();
        await invoke("get_sysinfo");
        const dt = performance.now() - start;
        samples.push(dt);
        streamOut.textContent += `  CYCLE ${i.toString().padStart(2, "0")}  -->  ${dt.toFixed(3)} ms\n`;
      }

      const avg = (samples.reduce((a, b) => a + b, 0) / samples.length).toFixed(3);
      const min = Math.min(...samples).toFixed(3);
      const max = Math.max(...samples).toFixed(3);

      streamOut.textContent += `\n>> TELEMETRY SUMMARY:\n` +
                               `   AVG LATENCY: ${avg} ms\n` +
                               `   MIN LATENCY: ${min} ms\n` +
                               `   MAX LATENCY: ${max} ms\n` +
                               `   DISPATCHER:  FNV-1a Hash Router (O(1) Integer Switch)`;

      streamStatus.textContent = "READY";
      streamStatus.style.color = "var(--aurora-emerald)";
    });
  }

  // 4. Pulse C Event
  if (btnAlert) {
    btnAlert.addEventListener("click", async () => {
      streamStatus.textContent = "DISPATCHING";
      streamStatus.style.color = "var(--starlight-cyan)";
      streamOut.textContent = "// Dispatched 'trigger_alert' to native registry...\n";

      try {
        const res = await invoke("trigger_alert");
        streamOut.textContent += `// Core response: ${res}\n// Awaiting asynchronous push event...`;
        streamStatus.textContent = "PULSED";
      } catch (err) {
        streamOut.textContent += `// Error: ${err.message || err}`;
        streamStatus.textContent = "FAULT";
        streamStatus.style.color = "var(--danger-crimson)";
      }
    });
  }

  // 5. Toast Telemetry Notification
  function showToast(tag, message) {
    if (!toastDock) return;

    const cell = document.createElement("div");
    cell.className = "toast-cell";

    const head = document.createElement("div");
    head.className = "toast-tag";
    head.innerHTML = `<span>${tag}</span><span>${new Date().toLocaleTimeString()}</span>`;

    const body = document.createElement("div");
    body.className = "toast-msg";
    body.textContent = typeof message === "object" ? JSON.stringify(message) : message;

    cell.appendChild(head);
    cell.appendChild(body);
    toastDock.appendChild(cell);

    setTimeout(() => {
      cell.style.transition = "opacity 0.25s, transform 0.25s";
      cell.style.opacity = "0";
      cell.style.transform = "translateY(8px)";
      setTimeout(() => cell.remove(), 250);
    }, 4000);
  }

  // 6. Listen for C Push Events
  if (window.vyom && typeof window.vyom.on === "function") {
    window.vyom.on("server_alert", (payload) => {
      showToast("EVENT // server_alert", payload);
      streamOut.textContent = `// [EVENT DISPATCH RECEIVED @ ${new Date().toLocaleTimeString()}]\n` +
                              `// Payload: ${typeof payload === "object" ? JSON.stringify(payload) : payload}\n\n` +
                              streamOut.textContent;
    });
  }

  // 7. Boot: Query System Info
  async function boot() {
    try {
      const res = await invoke("get_sysinfo");
      const info = typeof res === "string" ? JSON.parse(res) : res;
      if (specOs && info.os) specOs.textContent = info.os.toUpperCase();
      if (specArch && info.arch) specArch.textContent = info.arch.toUpperCase();
    } catch {
      if (specOs) specOs.textContent = "ACTIVE";
    }
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", boot);
  } else {
    boot();
  }
})();
