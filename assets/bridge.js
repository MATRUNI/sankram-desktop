// SPA HTML5 History Route Restoration (restores /route if rewritten by native fallback)
(function () {
  try {
    const url = new URL(window.location.href);
    const route = url.searchParams.get("__route");
    if (route) {
      window.history.replaceState(null, "", route);
    }
  } catch (e) {}
})();

window.vyom = {
  _reqId: 0,
  _listeners: new Map(),

  window:{
    minimize: () => window.vyom.invoke("vyom:window:minimize"),
    toggleMaximize: () => window.vyom.invoke("vyom:window:toggle_maximize"),
    close: () => window.vyom.invoke("vyom:window:close"),
    startDrag: () => window.vyom.invoke("vyom:window:start_drag"),
  },
  dialog:{
    openFile: (options = {}) => window.vyom.invoke("vyom:dialog:open_file", options),
    saveFile: (options = {}) => window.vyom.invoke("vyom:dialog:save_file", options),
  },
  
  // JavaScript -> C (Commands)
  invoke: function (cmd, args = {}) 
  {
    if(window.webkit?.messageHandlers?.desktop)
    {
      return window.webkit.messageHandlers.desktop.postMessage({ cmd, args });
    }
    if(window?.chrome?.webview)
    {
      return new Promise((resolve, reject) => {
        const id = ++this._reqId;
        const replyEvent = `__reply_${id}`;
        
        const handler = (res) => {
          this.off(replyEvent, handler);
          if (res.error) reject(new Error(res.error));
          else resolve(res.result);
        };
        this.on(replyEvent, handler);
        window.chrome.webview.postMessage({ id, cmd, args });
      });
    }

    return Promise.reject(new Error("No native platform detected."));
  },

  // C -> JavaScript (Events)
  on: function (event, callback) 
  {
    if (!this._listeners.has(event)) 
    {
      this._listeners.set(event, new Set());
    }
    this._listeners.get(event).add(callback);
    return ()=>{
      this.off(event,callback);
    }
  },

  off: function (event, callback) 
  {
    this._listeners.get(event)?.delete(callback);
  },

  // Internal trigger called by C
  _emit: function (event, data) 
  {
    const callbacks = this._listeners.get(event);
    if(!callbacks) return;

    for(const callback of callbacks)
    {
      try 
      {
        callback(data);
      } 
      catch (err) 
      {
        console.error("[bridge] Error in event listener:", err);
      }
    }
  },

  // File system commands
  fs: {
    // 1. Get file metadata
    fileInfo: async (path) => {
      const res = await window.vyom.invoke("vyom:fs:file_info", { path });
      return typeof res === "string" ? JSON.parse(res) : res;
    },

    // 2. Read chunk: splits header on line 1, hands you raw data!
    readChunk: async (path, offset = 0, length = 65536) => {
      const raw = await window.vyom.invoke("vyom:fs:read_chunk", { path, offset, length });
      const idx = raw.indexOf("\n");
      if (idx === -1) {
        return { offset, length: 0, total: 0, eof: true, data: "" };
      }

      const header = raw.slice(0, idx);
      const data = raw.slice(idx + 1);

      // Parse "offset=0,len=65536,total=1048576,eof=0"
      const meta = {};
      for (const pair of header.split(",")) {
        const [k, v] = pair.split("=");
        meta[k] = Number(v);
      }

      return {
        offset: meta.offset,
        length: meta.len,
        total: meta.total,
        eof: meta.eof === 1,
        data: data, // 100% pure raw unescaped text!
      };
    },

    // 3. Write text
    writeTextFile: async (path, content, append = false) => {
      const res = await window.vyom.invoke("vyom:fs:write_text", { path, content, append });
      return typeof res === "string" ? JSON.parse(res) : res;
    },

    // 4. List directory contents (with optional pagination)
    readDir: async (path, options = {}) => {
      const offset = options.offset || 0;
      const limit = options.limit || 500;
      const res = await window.vyom.invoke("vyom:fs:read_dir", { path, offset, limit });
      return typeof res === "string" ? JSON.parse(res) : res;
    },

    // 5. Create empty file (with any extension)
    createFile: async (path) => {
      const res = await window.vyom.invoke("vyom:fs:create_file", { path });
      return typeof res === "string" ? JSON.parse(res) : res;
    },

    // 6. Create directory
    createDir: async (path) => {
      const res = await window.vyom.invoke("vyom:fs:create_dir", { path });
      return typeof res === "string" ? JSON.parse(res) : res;
    },

    // 7. Remove file or empty directory
    remove: async (path) => {
      const res = await window.vyom.invoke("vyom:fs:remove", { path });
      return typeof res === "string" ? JSON.parse(res) : res;
    },

    // 8. Rename / Move file or directory
    rename: async (from, to) => {
      const res = await window.vyom.invoke("vyom:fs:rename", { from, to });
      return typeof res === "string" ? JSON.parse(res) : res;
    },

    // 9. Move alias (identical to rename)
    move: async (from, to) => {
      const res = await window.vyom.invoke("vyom:fs:move", { from, to });
      return typeof res === "string" ? JSON.parse(res) : res;
    },
  },
};