let eventCount = 0;

// Helper: Format JSON nicely
function formatOutput(data) {
  try {
    const parsed = typeof data === "string" ? JSON.parse(data) : data;
    return JSON.stringify(parsed, null, 2);
  } catch {
    return data;
  }
}

// 1. Initial System Info Load
async function loadSystemStats() {
  const osEl = document.getElementById("val-os");
  const archEl = document.getElementById("val-arch");
  const out = document.getElementById("sysinfo-out");
  const status = document.getElementById("ipc-status");

  try {
    status.textContent = "QUERYING...";
    const start = performance.now();
    const res = await window.myapp.invoke("get_sysinfo");
    const elapsed = (performance.now() - start).toFixed(2);

    const parsed = JSON.parse(res);
    osEl.textContent = parsed.os || "Active";
    archEl.textContent = `Arch: ${parsed.arch || "Native"}`;
    document.getElementById("val-latency").textContent = `${elapsed} ms`;

    out.textContent = formatOutput(res);
    status.textContent = "CONNECTED (200 OK)";
    status.style.color = "var(--success)";
  } catch (err) {
    osEl.textContent = "Connected";
    out.textContent = "Error: " + err;
    status.textContent = "ERROR";
    status.style.color = "#f87171";
  }
}

// 2. Button: Manual Query System Info
document.getElementById("btn-sysinfo").addEventListener("click", async () => {
  const out = document.getElementById("sysinfo-out");
  const status = document.getElementById("ipc-status");
  status.textContent = "DISPATCHING...";

  const t0 = performance.now();
  try {
    const res = await window.myapp.invoke("get_sysinfo");
    const delta = (performance.now() - t0).toFixed(2);
    document.getElementById("val-latency").textContent = `${delta} ms`;
    out.textContent = `// Latency: ${delta} ms\n` + formatOutput(res);
    status.textContent = "OK";
  } catch (err) {
    out.textContent = "Error: " + err;
    status.textContent = "FAIL";
  }
});

// 3. Button: Latency Benchmark
document.getElementById("btn-ping").addEventListener("click", async () => {
  const out = document.getElementById("sysinfo-out");
  const status = document.getElementById("ipc-status");
  status.textContent = "BENCHMARKING...";

  const samples = [];
  for (let i = 0; i < 5; i++) {
    const start = performance.now();
    await window.myapp.invoke("get_sysinfo");
    samples.push(performance.now() - start);
  }

  const avg = (samples.reduce((a, b) => a + b, 0) / samples.length).toFixed(2);
  const min = Math.min(...samples).toFixed(2);
  const max = Math.max(...samples).toFixed(2);

  document.getElementById("val-latency").textContent = `${avg} ms`;
  out.textContent = `// 5-Iteration Native IPC Benchmark:\n` +
                    `  Avg: ${avg} ms\n` +
                    `  Min: ${min} ms\n` +
                    `  Max: ${max} ms\n` +
                    `  Status: Zero-copy Memory Passing`;
  status.textContent = "BENCHMARK COMPLETE";
});

// 4. Button: Trigger C Alert
document.getElementById("btn-alert").addEventListener("click", async () => {
  try {
    await window.myapp.invoke("trigger_alert");
  } catch (err) {
    console.error("Trigger error:", err);
  }
});

// 5. C Push Event Listener
window.myapp.on("server_alert", (data) => {
  eventCount++;
  document.getElementById("val-events-count").textContent = `${eventCount} received`;

  const feed = document.getElementById("event-feed");
  const placeholder = feed.querySelector(".feed-placeholder");
  if (placeholder) placeholder.remove();

  const item = document.createElement("div");
  item.className = "feed-item";

  const timeStr = new Date().toLocaleTimeString();
  const payloadStr = typeof data === "object" ? JSON.stringify(data) : data;

  item.innerHTML = `
    <div class="feed-meta">
      <span>EVENT: server_alert</span>
      <span>${timeStr}</span>
    </div>
    <div class="feed-content">${payloadStr}</div>
  `;

  feed.insertBefore(item, feed.firstChild);
});

// Boot
window.addEventListener("DOMContentLoaded", () => {
  setTimeout(loadSystemStats, 100);
});
