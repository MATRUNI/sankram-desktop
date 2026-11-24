# Vyom (व्योम) — Architecture & API Specification

**Vyom** is an ultra-lightweight, high-performance, cross-platform desktop application runtime written in pure **C11**. It serves as an alternative to Electron and Tauri, achieving a standalone binary size of **< 30 KB** with zero bundled browsers and zero external runtime dependencies.

---

## 1. Architectural Overview & Wire Protocol

```
+-----------------------------------------------------------------+
|                        WEBVIEW FRONTEND                         |
|             (React / Vue / Svelte / Vanilla HTML+JS)            |
|                                                                 |
|   window.vyom.invoke(cmd, args)       window.vyom.on(evt, cb)   |
+-----------------------------------------------------------------+
                               |   ^
   JSON Request { cmd, args }  |   |  IPC Reply / Push Event
                               v   |
+-----------------------------------------------------------------+
|                        IPC BRIDGE LAYER                         |
|   Linux: WebKitGTK 6.0 ScriptMessageWithReply                   |
|   Windows: WebView2 WebMessage + ExecuteScript                  |
+-----------------------------------------------------------------+
                               |   ^
                    32-bit     |   |  ipc_reply_send(reply, ...)
                    FNV-1a     |   |
                               v   |
+-----------------------------------------------------------------+
|                  CORE ENGINE REGISTRY (C11)                     |
|                                                                 |
|   switch (fnv1a_32(cmd)) {                                      |
|       case HASH_FS_READ_CHUNK:  handle_fs_read_chunk(...);      |
|       case HASH_FS_READ_DIR:    handle_fs_read_dir(...);        |
|       default: search user cmd_table[...]                       |
|   }                                                             |
+-----------------------------------------------------------------+
                               |
                               v
+-----------------------------------------------------------------+
|                      NATIVE OS KERNEL APIS                      |
|   POSIX Syscalls (Linux)     |     Win32 Core APIs (Windows)    |
+-----------------------------------------------------------------+
```

### IPC Mechanics

1. **JavaScript to C (`invoke`)**:
   - Web frontend invokes `window.vyom.invoke(command_name, args_object)`.
   - On Linux: Passed natively via WebKit's `webkit_user_content_manager_register_script_message_handler_with_reply`.
   - On Windows: Dispatched via `window.chrome.webview.postMessage({ id, cmd, args })`.

2. **$O(1)$ Hash-Based Command Routing**:
   - Instead of expensive string comparisons (`strcmp`), Vyom hashes command strings at compile-time using the **32-bit FNV-1a** algorithm.
   - The engine switchboard executes a single CPU register integer check in **1 clock cycle**.

3. **C to JavaScript (`ipc_reply_send` & `platform_app_emit`)**:
   - Synchronous/Asynchronous replies are delivered directly back to the originating JS Promise.
   - Asynchronous background events are pushed via `platform_app_emit(event_name, json_payload)`.

---

## 2. Master Command Dispatch Table

| JavaScript Invocation | Wire Command Name | FNV-1a Hash (32-bit) | C Handler | Source File |
| :--- | :--- | :--- | :--- | :--- |
| `window.vyom.window.minimize()` | `vyom:window:minimize` | `0xf460cba8u` | `handle_win_minimize` | `src/engine/registry.c` |
| `window.vyom.window.toggleMaximize()` | `vyom:window:toggle_maximize` | `0x17a84d23u` | `handle_win_toggle_max` | `src/engine/registry.c` |
| `window.vyom.window.close()` | `vyom:window:close` | `0x0590e82cu` | `handle_win_close` | `src/engine/registry.c` |
| `window.vyom.window.startDrag()` | `vyom:window:start_drag` | `0x34e32b9fu` | `handle_win_start_drag` | `src/engine/registry.c` |
| `window.vyom.dialog.openFile(options)` | `vyom:dialog:open_file` | `0xadfa146fu` | `handle_dialog_open_file` | `src/engine/registry.c` |
| `window.vyom.dialog.saveFile(options)` | `vyom:dialog:save_file` | `0x7f7e3624u` | `handle_dialog_save_file` | `src/engine/registry.c` |
| `window.vyom.fs.fileInfo(path)` | `vyom:fs:file_info` | `0x1be18f64u` | `handle_fs_file_info` | `src/engine/fs.c` |
| `window.vyom.fs.readChunk(path, off, len)` | `vyom:fs:read_chunk` | `0x2e9676efu` | `handle_fs_read_chunk` | `src/engine/fs.c` |
| `window.vyom.fs.writeTextFile(path, txt, app)`| `vyom:fs:write_text` | `0x4072a17eu` | `handle_fs_write_text` | `src/engine/fs.c` |
| `window.vyom.fs.readDir(path, options)` | `vyom:fs:read_dir` | `0x49e02d51u` | `handle_fs_read_dir` | `src/engine/fs.c` |
| `window.vyom.fs.createFile(path)` | `vyom:fs:create_file` | `0x4bb172bcu` | `handle_fs_create_file` | `src/engine/fs.c` |
| `window.vyom.fs.createDir(path)` | `vyom:fs:create_dir` | `0xb8820841u` | `handle_fs_create_dir` | `src/engine/fs.c` |
| `window.vyom.fs.remove(path)` | `vyom:fs:remove` | `0x65be3301u` | `handle_fs_remove` | `src/engine/fs.c` |

---

## 3. Window Management (`window.vyom.window`)

Provides native window controls for custom frameless titlebars.

### `minimize()`
Minimizes the native window to the taskbar/dock.
```javascript
await window.vyom.window.minimize();
```
- **Returns**: `Promise<{ success: boolean }>`

### `toggleMaximize()`
Toggles between maximized and restored window states.
```javascript
await window.vyom.window.toggleMaximize();
```
- **Returns**: `Promise<{ success: boolean }>`

### `close()`
Closes the active window and terminates the event loop if it is the main window.
```javascript
await window.vyom.window.close();
```

### `startDrag()`
Initiates a native window drag operation. Must be bound to mouse down on custom titlebars.
```javascript
document.querySelector(".titlebar").addEventListener("mousedown", (e) => {
  if (e.button === 0 && e.target.tagName !== "BUTTON") {
    window.vyom.window.startDrag();
  }
});
```

---

## 4. Native Dialogs (`window.vyom.dialog`)

Triggers OS-native file chooser dialogs (GTK4 `GtkFileDialog` on Linux, `comdlg32` `GetOpenFileNameW`/`GetSaveFileNameW` on Windows).

### `openFile(options)`
Opens a native file selection modal.
```javascript
const res = await window.vyom.dialog.openFile();
if (!res.canceled) {
  console.log("Selected path:", res.path);
}
```
- **Returns**: `Promise<{ canceled: boolean, path: string | null }>`

### `saveFile(options)`
Opens a native save-file modal with prompt for filename and overwrite confirmation.
```javascript
const res = await window.vyom.dialog.saveFile({ defaultName: "export.json" });
if (!res.canceled) {
  console.log("Save destination:", res.path);
}
```
- **Arguments**:
  - `defaultName` *(string, optional)*: Default filename presented to user.
- **Returns**: `Promise<{ canceled: boolean, path: string | null }>`

---

## 5. File System Engine (`window.vyom.fs`)

A zero-allocation, 64-bit capable native file system abstraction.

### `fileInfo(path)`
Fetches 64-bit metadata for a given file or folder.
```javascript
const info = await window.vyom.fs.fileInfo("Makefile");
console.log(info);
// => { exists: true, size: 2701, isDir: false }
```
- **Arguments**:
  - `path` *(string)*: Absolute or relative path.
- **Returns**: `Promise<{ exists: boolean, size: number, isDir: boolean }>`

---

### `readChunk(path, offset, length)`
Reads a discrete slice of a file using 64-bit random-access seeking (`fseeko` on POSIX, `_fseeki64` on Windows).

**Wire Optimization (Header + Raw Data Protocol):**
Instead of encoding multi-megabyte payloads into JSON strings (which causes high CPU/RAM overhead and requires escaping), Vyom sends:
```text
offset=0,len=65536,total=1048576,eof=0\n<RAW PAYLOAD HERE>
```
The client-side `bridge.js` automatically parses line 1 into metadata and passes `data` as pure raw text.

```javascript
const chunk = await window.vyom.fs.readChunk("large_dataset.csv", 0, 65536);
console.log(chunk);
/*
{
  offset: 0,
  length: 65536,
  total: 104857600,
  eof: false,
  data: "id,name,role\n1,Admin,Superuser\n..."
}
*/
```
- **Arguments**:
  - `path` *(string)*: Target file path.
  - `offset` *(number, default: 0)*: Byte offset to begin reading from.
  - `length` *(number, default: 65536)*: Number of bytes to read (clamped to max 4MB per call).
- **Returns**: `Promise<{ offset: number, length: number, total: number, eof: boolean, data: string }>`

---

### `writeTextFile(path, content, append)`
Writes text content to disk. Creates the file if it does not exist.
```javascript
// Overwrite:
await window.vyom.fs.writeTextFile("output.log", "Initiated\n", false);

// Append:
await window.vyom.fs.writeTextFile("output.log", "Processing completed\n", true);
```
- **Arguments**:
  - `path` *(string)*: Target file path (any extension).
  - `content` *(string)*: Text string to write.
  - `append` *(boolean, default: false)*: When true, appends to end of file without truncating.
- **Returns**: `Promise<{ success: boolean, bytesWritten: number }>`

---

### `readDir(path, options)`
Lists the contents of a directory.

**Memory Optimization (Pure Stack Allocation):**
`handle_fs_read_dir` uses a fixed **64 KB stack buffer** with bounds checking. It performs **0 heap allocations (`malloc`/`realloc`)**. Memory is immediately reclaimed by the CPU upon return.

```javascript
// Read all (up to 500 entries):
const entries = await window.vyom.fs.readDir("./src");

// Paginated reading:
const page1 = await window.vyom.fs.readDir(".", { offset: 0, limit: 10 });
const page2 = await window.vyom.fs.readDir(".", { offset: 10, limit: 10 });

console.log(entries);
/*
[
  { name: "main.c", isDir: false, size: 1240 },
  { name: "engine", isDir: true, size: 4096 }
]
*/
```
- **Arguments**:
  - `path` *(string)*: Directory path to read.
  - `options.offset` *(number, default: 0)*: Number of entries to skip.
  - `options.limit` *(number, default: 500)*: Max entries to return (capped at 500 for 64KB stack safety).
- **Returns**: `Promise<Array<{ name: string, isDir: boolean, size: number }>>`

---

### `createFile(path)`
Creates an empty file with any extension (equivalent to POSIX `touch`). If the file already exists, its contents are preserved.
```javascript
await window.vyom.fs.createFile("src/modules/router.c");
```
- **Arguments**:
  - `path` *(string)*: Target file path.
- **Returns**: `Promise<{ success: boolean }>`

---

### `createDir(path)`
Creates a directory on disk using native OS syscalls (`mkdir(path, 0755)` on POSIX, `_mkdir(path)` on Windows).
```javascript
await window.vyom.fs.createDir("build/logs");
```
- **Arguments**:
  - `path` *(string)*: Directory path to create.
- **Returns**: `Promise<{ success: boolean }>`

---

### `remove(path)`
Removes a file or an empty directory using universal ANSI C `remove(path)`.
```javascript
// Remove file:
await window.vyom.fs.remove("temp.txt");

// Remove empty folder:
await window.vyom.fs.remove("temp_dir");
```
- **Arguments**:
  - `path` *(string)*: Path to remove.
- **Returns**: `Promise<{ success: boolean }>`

---

## 6. Extending Vyom: Adding Custom C Commands

Vyom includes a 64-slot dynamic command table for user-defined commands.

### 1. In your C application (`src/main.c` or `src/commands/*.c`):
```c
#include "app.h"
#include "json_util.h"

void handle_calculate_hash(const char *args, ipc_reply_t *reply) {
    char input[256];
    if (json_get_string(args, "text", input, sizeof(input))) {
        // Compute custom logic...
        ipc_reply_send(reply, "{\"status\": \"ok\", \"value\": 42}");
    } else {
        ipc_reply_error(reply, "Invalid text input");
    }
}

// Bind during startup:
app_bind_command("calculate_hash", handle_calculate_hash);
```

### 2. In JavaScript:
```javascript
const res = await window.vyom.invoke("calculate_hash", { text: "hello" });
console.log(res);
```

---

## 7. Bidirectional Push Events (C $\to$ JS)

To stream events from background threads in C to the frontend:

### In C:
```c
platform_app_emit("download_progress", "{\"percent\": 45.2, \"speed\": \"2.4 MB/s\"}");
```

### In JavaScript:
```javascript
const unlisten = window.vyom.on("download_progress", (data) => {
  console.log("Progress:", data.percent, data.speed);
});

// To stop listening later:
unlisten();
```

