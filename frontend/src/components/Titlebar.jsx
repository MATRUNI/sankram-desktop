import React from "react";

export default function Titlebar({ activeTab, setActiveTab }) {
  const handleDrag = (e) => {
    // Only drag on left click and ignore button/tab clicks
    if (e.button === 0 && e.target.closest("button") === null) {
      window.vyom?.window?.startDrag?.();
    }
  };

  return (
    <header className="titlebar" onMouseDown={handleDrag}>
      <div className="titlebar-drag">
        <div className="brand">
          <svg className="brand-glyph" viewBox="0 0 24 24" fill="none" stroke="currentColor">
            <circle cx="12" cy="12" r="9" strokeWidth="1.5" opacity="0.4" />
            <path d="M12 4 L12 20 M4 12 L20 12" strokeWidth="1.2" opacity="0.3" />
            <path d="M7 8 L12 17 L17 8" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round" />
            <circle cx="12" cy="12" r="2" fill="currentColor" />
          </svg>
          <span className="brand-name">VYOM</span>
          <span className="brand-divider">/</span>
          <span className="brand-spec">REACT SPA</span>
        </div>

        {/* Navigation Tabs */}
        <nav className="titlebar-nav">
          <button
            className={`titlebar-tab ${activeTab === "telemetry" ? "active" : ""}`}
            onClick={() => setActiveTab("telemetry")}
          >
            Telemetry
          </button>
          <button
            className={`titlebar-tab ${activeTab === "fs" ? "active" : ""}`}
            onClick={() => setActiveTab("fs")}
          >
            File System
          </button>
          <button
            className={`titlebar-tab ${activeTab === "docs" ? "active" : ""}`}
            onClick={() => setActiveTab("docs")}
          >
            API Docs
          </button>
        </nav>
      </div>

      <div className="titlebar-controls">
        <button
          className="win-btn"
          onClick={() => window.vyom?.window?.minimize?.()}
          title="Minimize"
          aria-label="Minimize"
        >
          <svg viewBox="0 0 10 10">
            <line x1="1" y1="5" x2="9" y2="5" stroke="currentColor" strokeWidth="1.2" />
          </svg>
        </button>
        <button
          className="win-btn"
          onClick={() => window.vyom?.window?.toggleMaximize?.()}
          title="Maximize"
          aria-label="Maximize"
        >
          <svg viewBox="0 0 10 10">
            <rect x="1.5" y="1.5" width="7" height="7" fill="none" stroke="currentColor" strokeWidth="1.2" />
          </svg>
        </button>
        <button
          className="win-btn win-close"
          onClick={() => window.vyom?.window?.close?.() || window.close()}
          title="Close"
          aria-label="Close"
        >
          <svg viewBox="0 0 10 10">
            <path d="M2,2 L8,8 M8,2 L2,8" stroke="currentColor" strokeWidth="1.2" />
          </svg>
        </button>
      </div>
    </header>
  );
}

