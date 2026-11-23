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
  }
};