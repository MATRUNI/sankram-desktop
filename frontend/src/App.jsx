import React, { useState, useEffect } from "react";
import Titlebar from "./components/Titlebar";
import CosmicBackground from "./components/CosmicBackground";
import TelemetryHero from "./components/TelemetryHero";
import FsExplorer from "./components/FsExplorer";
import DocsViewer from "./components/DocsViewer";

// Helper to extract clean route from hash or pathname
function getInitialTab() {
  const hash = window.location.hash.replace("#/", "").replace("#", "");
  if (["telemetry", "fs", "docs"].includes(hash)) {
    return hash;
  }
  const path = window.location.pathname.replace(/^\//, "");
  if (["telemetry", "fs", "docs"].includes(path)) {
    return path;
  }
  return "telemetry";
}

export default function App() {
  // 1. Read URL on mount!
  const [activeTab, setActiveTab] = useState(getInitialTab);

  // 2. When user clicks tab, update the URL!
  const handleTabChange = (tab) => {
    setActiveTab(tab);
    window.location.hash = `/${tab}`;
  };

  // 3. Listen to browser Back / Forward buttons & URL changes
  useEffect(() => {
    const handleUrlChange = () => {
      setActiveTab(getInitialTab());
    };

    window.addEventListener("hashchange", handleUrlChange);
    window.addEventListener("popstate", handleUrlChange);
    return () => {
      window.removeEventListener("hashchange", handleUrlChange);
      window.removeEventListener("popstate", handleUrlChange);
    };
  }, []);

  return (
    <div className="app-container">
      <Titlebar activeTab={activeTab} setActiveTab={handleTabChange} />
      <CosmicBackground />

      <main className="app-main">
        {activeTab === "telemetry" && <TelemetryHero />}
        {activeTab === "fs" && <FsExplorer />}
        {activeTab === "docs" && <DocsViewer />}
      </main>

      <footer className="telemetry-footer">
        <div className="footer-data">
          <span>FRONTEND: <strong>React 19 (Vite)</strong></span>
          <span className="dot-sep">&bull;</span>
          <span>RUNTIME: <strong>Vyom C11</strong></span>
          <span className="dot-sep">&bull;</span>
          <span>CORE FOOTPRINT: <strong>&lt; 30 KB</strong></span>
        </div>
      </footer>
    </div>
  );
}
