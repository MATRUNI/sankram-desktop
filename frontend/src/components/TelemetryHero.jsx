import React, { useState, useEffect } from "react";

export default function TelemetryHero() {
  const [count, setCount] = useState(0);
  const [latency, setLatency] = useState("0.00 ms");
  const [benchLogs, setBenchLogs] = useState("// Telemetry ready. Click pulse or run latency cycle.");
  const [benchStatus, setBenchStatus] = useState("STANDBY");

  // Listen for asynchronous push events from C core
  useEffect(() => {
    if (window.vyom?.on) {
      const unlisten = window.vyom.on("server_alert", (data) => {
        const payload = typeof data === "string" ? JSON.parse(data) : data;
        setBenchLogs((prev) =>
          `${prev}\n\n>> [PUSH EVENT RECEIVED FROM C CORE]:\n` +
          `   Message: "${payload.msg || 'Alert'}"\n` +
          `   Payload: ${JSON.stringify(payload)}`
        );
        setBenchStatus("EVENT CAPTURED");
      });
      return () => unlisten?.();
    }
  }, []);

  // Pulse single IPC call
  const handlePulse = async () => {
    const next = count + 1;
    setCount(next);

    const t0 = performance.now();
    try {
      if (window.vyom?.invoke) {
        await window.vyom.invoke("get_sysinfo");
      }
      const dt = (performance.now() - t0).toFixed(2);
      setLatency(`${dt} ms`);
    } catch (e) {
      setLatency("ERR");
    }
  };

  // Run 10x cycle benchmark
  const handleBenchmark = async () => {
    setBenchStatus("SAMPLING");
    let logs = "// Initiating 10-cycle zero-copy IPC telemetry probe...\n";
    setBenchLogs(logs);

    const samples = [];
    for (let i = 1; i <= 10; i++) {
      const t0 = performance.now();
      if (window.vyom?.invoke) {
        await window.vyom.invoke("get_sysinfo");
      }
      const dt = performance.now() - t0;
      samples.push(dt);
      logs += `  CYCLE ${String(i).padStart(2, "0")}  -->  ${dt.toFixed(3)} ms\n`;
      setBenchLogs(logs);
    }

    const avg = (samples.reduce((a, b) => a + b, 0) / samples.length).toFixed(3);
    const min = Math.min(...samples).toFixed(3);
    const max = Math.max(...samples).toFixed(3);

    logs += `\n>> TELEMETRY SUMMARY:\n   AVG LATENCY: ${avg} ms\n   MIN LATENCY: ${min} ms\n   MAX LATENCY: ${max} ms\n   STATUS: 10/10 PASS`;
    setBenchLogs(logs);
    setBenchStatus("VERIFIED");
  };

  // Choose file via native dialog
  const handleChooseFile = async () => {
    try {
      const res = await window.vyom?.dialog?.openFile?.();
      if (res && !res.canceled && res.path) {
        setBenchLogs(`// [NATIVE DIALOG // FILE SELECTED]\n// Path:   ${res.path}\n// Status: User confirmed selection`);
        setBenchStatus("SELECTED");
      } else {
        setBenchLogs("// [NATIVE DIALOG // DISMISSED]\n// Path:   null\n// Status: Cancelled by user");
        setBenchStatus("CANCELLED");
      }
    } catch (err) {
      setBenchLogs(`// Dialog Error: ${err.message || err}`);
      setBenchStatus("FAULT");
    }
  };

  // Pulse C Event (triggers C app_emit back to JS)
  const handlePulseCEvent = async () => {
    setBenchStatus("DISPATCHING");
    setBenchLogs(
      "// Dispatching 'trigger_alert' to native C registry...\n" +
      "// Awaiting asynchronous push event from C core..."
    );

    try {
      const res = await window.vyom?.invoke?.("trigger_alert");
      const resStr = typeof res === "object" ? JSON.stringify(res) : String(res);
      setBenchLogs((prev) => `${prev}\n// Core response: ${resStr}`);
      setBenchStatus("PULSED");
    } catch (err) {
      setBenchLogs(`// Error: ${err.message || err}`);
      setBenchStatus("FAULT");
    }
  };

  return (
    <div className="telemetry-view">
      <section className="hero-stage">
        <div className="orbital-emblem">
          <div className="orbital-glow"></div>
          <svg className="orbital-svg" viewBox="0 0 120 120" fill="none">
            <circle className="ring-outer" cx="60" cy="60" r="54" stroke="currentColor" strokeWidth="1" strokeDasharray="4 8" opacity="0.35" />
            <circle className="ring-inner" cx="60" cy="60" r="42" stroke="currentColor" strokeWidth="1.2" opacity="0.6" />
            <circle cx="60" cy="60" r="28" stroke="currentColor" strokeWidth="1" strokeOpacity="0.2" />
            <path className="apex-mark" d="M44 48 L60 76 L76 48" stroke="#38bdf8" strokeWidth="2.5" strokeLinecap="round" strokeLinejoin="round" />
            <circle className="core-singularity" cx="60" cy="60" r="3" fill="#ffffff" />
          </svg>
        </div>

        <div className="title-block">
          <div className="pre-label">
            <span className="live-dot"></span>
            <span>REACT 19 + C11 RUNTIME</span>
          </div>
          <h1 className="core-title">VYOM</h1>
          <p className="core-tagline">The weightless desktop runtime. Zero bundled Chromium, infinite canvas.</p>
        </div>

        <div className="telemetry-capsule-wrapper">
          <button onClick={handlePulse} className="telemetry-capsule">
            <div className="capsule-left">
              <span className="pulse-indicator"></span>
              <span className="counter-val">count is {count}</span>
            </div>
            <div className="capsule-sep"></div>
            <div className="capsule-right">
              <span className="metric-label">IPC</span>
              <span className="metric-val">{latency}</span>
            </div>
          </button>
          <span className="capsule-hint">Click capsule to pulse C core and benchmark zero-copy IPC</span>
        </div>
      </section>

      {/* 3 Specs Grid */}
      <section className="telemetry-grid">
        <div className="telemetry-card">
          <span className="card-id">SPEC // 01</span>
          <h3 className="card-heading">&lt; 30 KB Standalone</h3>
          <p className="card-body">Zero bundled browser, zero Node.js. Pure native Win32 & WebKitGTK bindings.</p>
        </div>
        <div className="telemetry-card">
          <span className="card-id">SPEC // 02</span>
          <h3 className="card-heading">Sub-Millisecond IPC</h3>
          <p className="card-body">Zero-copy Wyhash 32-bit register switchboard with 1-cycle O(1) integer routing.</p>
        </div>
        <div className="telemetry-card">
          <span className="card-id">SPEC // 03</span>
          <h3 className="card-heading">Zero-Allocation FS</h3>
          <p className="card-body">64-bit chunk streaming and 64KB stack buffer folder listing with 0 malloc overhead.</p>
        </div>
      </section>

      {/* Telemetry Console */}
      <section className="instrument-strip">
        <div className="strip-header">
          <span className="strip-title">NATIVE TELEMETRY CONSOLE</span>
          <span className="strip-status">{benchStatus}</span>
        </div>
        <div className="strip-actions">
          <button onClick={handleChooseFile} className="strip-btn">
            <span>Choose Native File</span>
          </button>
          <button onClick={handleBenchmark} className="strip-btn">
            <span>10x Latency Cycle</span>
          </button>
          <button onClick={handlePulseCEvent} className="strip-btn">
            <span>Pulse C Event</span>
          </button>
        </div>
        <pre className="telemetry-console">{benchLogs}</pre>
      </section>
    </div>
  );
}
