import React, { useState, useEffect } from "react";

const SANDBOX_DIR = "sandbox";

export default function FsExplorer() {
  const [entries, setEntries] = useState([]);
  const [loading, setLoading] = useState(false);
  const [newItemName, setNewItemName] = useState("");
  const [selectedFile, setSelectedFile] = useState(null);
  const [fileContent, setFileContent] = useState("");
  const [statusMessage, setStatusMessage] = useState("Initializing sandbox...");

  // Initialize sandbox with demo files if not present
  const initSandbox = async () => {
    try {
      if (window.vyom?.fs) {
        const info = await window.vyom.fs.fileInfo(SANDBOX_DIR);
        if (!info?.exists) {
          await window.vyom.fs.createDir(SANDBOX_DIR);
          await window.vyom.fs.writeTextFile(
            `${SANDBOX_DIR}/welcome.txt`,
            "Welcome to the Vyom File System Sandbox!\n\n" +
            "This is a dedicated, isolated playground folder.\n" +
            "Feel free to create, edit, chunk-read, and delete files here safely!\n" +
            "Your project source code (src, Makefile, .git) is completely protected.\n"
          );
          await window.vyom.fs.writeTextFile(
            `${SANDBOX_DIR}/config.json`,
            '{\n  "runtime": "Vyom C11",\n  "subsystem": "WebKitGTK / WebView2",\n  "sandboxed": true\n}\n'
          );
        }
      }
    } catch (err) {
      console.error("[sandbox init]", err);
    }
  };

  // Load sandbox directory entries
  const loadDirectory = async () => {
    setLoading(true);
    setStatusMessage(`Scanning ./${SANDBOX_DIR}...`);
    try {
      await initSandbox();
      const list = (await window.vyom?.fs?.readDir?.(SANDBOX_DIR, { offset: 0, limit: 100 })) || [];
      const sorted = [...list].sort((a, b) => {
        if (a.isDir === b.isDir) return a.name.localeCompare(b.name);
        return a.isDir ? -1 : 1;
      });
      setEntries(sorted);
      setStatusMessage(`Sandbox contains ${list.length} item(s)`);
    } catch (err) {
      setStatusMessage(`Scan failed: ${err.message || err}`);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    loadDirectory();
  }, []);

  // Create empty file inside sandbox
  const handleCreateFile = async () => {
    const trimmed = newItemName.trim().replace(/^\/+/, "");
    if (!trimmed) return;
    const targetPath = `${SANDBOX_DIR}/${trimmed}`;
    try {
      await window.vyom?.fs?.createFile?.(targetPath);
      setStatusMessage(`Created file: ${targetPath}`);
      setNewItemName("");
      loadDirectory();
    } catch (err) {
      setStatusMessage(`Create failed: ${err.message || err}`);
    }
  };

  // Create folder inside sandbox
  const handleCreateDir = async () => {
    const trimmed = newItemName.trim().replace(/^\/+/, "");
    if (!trimmed) return;
    const targetPath = `${SANDBOX_DIR}/${trimmed}`;
    try {
      await window.vyom?.fs?.createDir?.(targetPath);
      setStatusMessage(`Created folder: ${targetPath}`);
      setNewItemName("");
      loadDirectory();
    } catch (err) {
      setStatusMessage(`Create failed: ${err.message || err}`);
    }
  };

  // Remove item with safety check
  const handleRemove = async (name) => {
    // Safety guard: only allow deleting inside sandbox
    const targetPath = `${SANDBOX_DIR}/${name}`;
    if (!window.confirm(`Delete "${name}" from sandbox?`)) {
      return;
    }

    try {
      await window.vyom?.fs?.remove?.(targetPath);
      setStatusMessage(`Removed: ${targetPath}`);
      if (selectedFile === name) {
        setSelectedFile(null);
        setFileContent("");
      }
      loadDirectory();
    } catch (err) {
      setStatusMessage(`Remove failed: ${err.message || err}`);
    }
  };

  // Rename / Move item
  const handleRename = async (name) => {
    const newName = window.prompt(`Rename or move "${name}" to:`, name);
    const trimmed = (newName || "").trim().replace(/^\/+/, "");
    if (!trimmed || trimmed === name) return;
    const oldPath = `${SANDBOX_DIR}/${name}`;
    const newPath = `${SANDBOX_DIR}/${trimmed}`;
    try {
      await window.vyom?.fs?.rename?.(oldPath, newPath);
      setStatusMessage(`Renamed: ${name} -> ${trimmed}`);
      if (selectedFile === name) setSelectedFile(trimmed);
      loadDirectory();
    } catch (err) {
      setStatusMessage(`Rename failed: ${err.message || err}`);
    }
  };

  // View file contents via chunk reader
  const handleViewFile = async (name) => {
    const targetPath = `${SANDBOX_DIR}/${name}`;
    setSelectedFile(name);
    setFileContent("Streaming file contents via vyom.fs.readChunk()...");
    try {
      const chunk = await window.vyom?.fs?.readChunk?.(targetPath, 0, 16384);
      setFileContent(chunk.data || "// (Empty file)");
      setStatusMessage(`Read ${chunk.length} bytes from ${name}`);
    } catch (err) {
      setFileContent(`Error reading file: ${err.message || err}`);
    }
  };

  return (
    <div className="fs-view">
      <div className="fs-header">
        <div className="fs-title-area">
          <h2 className="fs-title">File System Sandbox</h2>
          <p className="fs-subtitle">
            Safe isolated playground at <code className="md-inline-code">./sandbox/</code> &bull; Project source code protected
          </p>
        </div>
        <div className="fs-status-pill">{statusMessage}</div>
      </div>

      {/* Action Toolbar */}
      <div className="fs-toolbar">
        <div className="fs-input-group">
          <input
            type="text"
            className="fs-input"
            placeholder="New filename or folder..."
            value={newItemName}
            onChange={(e) => setNewItemName(e.target.value)}
            onKeyDown={(e) => e.key === "Enter" && handleCreateFile()}
          />
          <button onClick={handleCreateFile} className="strip-btn" style={{ padding: "0.5rem 0.9rem" }}>
            + File
          </button>
          <button onClick={handleCreateDir} className="strip-btn" style={{ padding: "0.5rem 0.9rem" }}>
            + Folder
          </button>
        </div>

        <button onClick={loadDirectory} className="strip-btn" style={{ padding: "0.5rem 0.9rem" }}>
          Refresh
        </button>
      </div>

      {/* Explorer Split View */}
      <div className="fs-grid">
        {/* Left: File List */}
        <div className="fs-file-panel">
          <div className="fs-panel-header">
            <span>SANDBOX ITEMS ({entries.length})</span>
            <span style={{ color: "var(--aurora-emerald)" }}>./{SANDBOX_DIR}/</span>
          </div>

          <div className="fs-list">
            {loading ? (
              <div className="fs-empty">Scanning sandbox...</div>
            ) : entries.length === 0 ? (
              <div className="fs-empty">Sandbox is empty. Create a file above!</div>
            ) : (
              entries.map((item) => (
                <div
                  key={item.name}
                  className={`fs-item ${selectedFile === item.name ? "selected" : ""}`}
                  onClick={() => !item.isDir && handleViewFile(item.name)}
                >
                  <div className="fs-item-meta">
                    <span className="fs-item-icon">{item.isDir ? "📁" : "📄"}</span>
                    <span className="fs-item-name">{item.name}</span>
                    <span className="fs-item-size">{item.isDir ? "DIR" : `${item.size} B`}</span>
                  </div>
                  <div style={{ display: "flex", gap: "0.25rem" }}>
                    <button
                      className="fs-del-btn"
                      style={{ color: "var(--nebula-lavender)" }}
                      title="Rename or move item"
                      onClick={(e) => {
                        e.stopPropagation();
                        handleRename(item.name);
                      }}
                    >
                      ✎
                    </button>
                    <button
                      className="fs-del-btn"
                      title="Delete item from sandbox"
                      onClick={(e) => {
                        e.stopPropagation();
                        handleRemove(item.name);
                      }}
                    >
                      ✕
                    </button>
                  </div>
                </div>
              ))
            )}
          </div>
        </div>

        {/* Right: Chunk Previewer */}
        <div className="fs-preview-panel">
          <div className="fs-panel-header">
            <span>LIVE CHUNK PREVIEW</span>
            <span style={{ color: "var(--starlight-cyan)" }}>{selectedFile || "None"}</span>
          </div>
          <pre className="fs-preview-content">
            {fileContent || "// Click any file in the sandbox to stream its chunks live into this buffer."}
          </pre>
        </div>
      </div>
    </div>
  );
}
