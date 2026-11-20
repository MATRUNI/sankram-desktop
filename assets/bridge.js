window.myapp = {
  _listeners: new Map(),

  // JavaScript -> C (Commands)
  invoke: function (cmd, args = {}) 
  {
    return window.webkit.messageHandlers.desktop.postMessage({ cmd, args });
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