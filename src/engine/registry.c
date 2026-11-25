#include "hash.h"
#include "registry.h"
#include "platform.h"
#include "fs.h"
#include "clipboard.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define HASH_WIN_MINIMIZE          0x0d48ed56u // wyhash32("vyom:window:minimize")
#define HASH_WIN_TOGGLE_MAX        0xfe1b2c5bu // wyhash32("vyom:window:toggle_maximize")
#define HASH_WIN_CLOSE             0xafd91676u // wyhash32("vyom:window:close")
#define HASH_WIN_START_DRAG        0x018bb464u // wyhash32("vyom:window:start_drag")
#define HASH_DIALOG_OPEN_FILE      0x8f8202e3u // wyhash32("vyom:dialog:open_file")
#define HASH_DIALOG_SAVE_FILE      0x148805c7u // wyhash32("vyom:dialog:save_file")
#define HASH_FS_FILE_INFO          0x2bfcb39bu // wyhash32("vyom:fs:file_info")
#define HASH_FS_READ_CHUNK         0x79ea54d3u // wyhash32("vyom:fs:read_chunk")
#define HASH_FS_WRITE_TEXT         0xb0e8eb90u // wyhash32("vyom:fs:write_text")
#define HASH_FS_READ_DIR           0xd11449e8u // wyhash32("vyom:fs:read_dir")
#define HASH_FS_CREATE_FILE        0xe59c953fu // wyhash32("vyom:fs:create_file")
#define HASH_FS_CREATE_DIR         0xedad688au // wyhash32("vyom:fs:create_dir")
#define HASH_FS_REMOVE             0x1354a270u // wyhash32("vyom:fs:remove")
#define HASH_FS_RENAME             0x9462544eu // wyhash32("vyom:fs:rename")
#define HASH_FS_MOVE               0xae3da64du // wyhash32("vyom:fs:move")
#define HASH_CLIPBOARD_WRITE_TEXT  0xfd14bb60u // wyhash32("vyom:clipboard:write_text")
#define HASH_CLIPBOARD_READ_TEXT   0x989ead8eu // wyhash32("vyom:clipboard:read_text")
#define HASH_CLIPBOARD_CLEAR       0x8eee9d42u // wyhash32("vyom:clipboard:clear")
#define HASH_CLIPBOARD_HAS_TEXT    0xe86a74f5u // wyhash32("vyom:clipboard:has_text")
typedef struct {
    char name[32];
    uint32_t hash;
    app_command_fn handler;
} cmd_entry_t;

static cmd_entry_t cmd_table[CMD_TABLE_SIZE];

void app_bind_command(const char *name, app_command_fn handler) 
{
    uint32_t hash = wyhash32(name);
    uint32_t idx = hash & (CMD_TABLE_SIZE - 1);

    for (int i = 0; i < CMD_TABLE_SIZE; i++)
    {
        uint32_t slot = (idx + i) & (CMD_TABLE_SIZE - 1);  // Linear probing

        if (cmd_table[slot].handler == NULL || strcmp(cmd_table[slot].name, name) == 0) 
        {
            strncpy(cmd_table[slot].name, name, sizeof(cmd_table[slot].name) - 1);
            cmd_table[slot].hash = hash;
            cmd_table[slot].handler = handler;
            return;
        }
    }
}

static void handle_win_minimize(const char *args, ipc_reply_t *reply) 
{
    (void)args;
    platform_window_minimize();
    ipc_reply_send(reply, "{\"success\": true}");
}

static void handle_win_toggle_max(const char *args, ipc_reply_t *reply) 
{
    (void)args;
    platform_window_toggle_maximize();
    ipc_reply_send(reply, "{\"success\": true}");
}

static void handle_win_close(const char *args, ipc_reply_t *reply) 
{
    (void)args;
    platform_window_close();
    ipc_reply_send(reply, "{\"success\": true}");
}

static void handle_win_start_drag(const char *args, ipc_reply_t *reply) 
{
    (void)args;
    platform_window_start_drag();
    ipc_reply_send(reply, "{\"success\": true}");
}

static void reply_with_path(ipc_reply_t *reply, char *path) {
    if (path) {
        char json[1024];
        snprintf(json, sizeof(json), "{\"canceled\": false, \"path\": \"%s\"}", path);
        for (char *p = json; *p; p++) {
            if (*p == '\\') *p = '/';
        }
        ipc_reply_send(reply, json);
        free(path);
    } else {
        ipc_reply_send(reply, "{\"canceled\": true, \"path\": null}");
    }
}

// Now both handlers become 2 lines!
static void handle_dialog_open_file(const char *args, ipc_reply_t *reply) {
    (void)args;
    reply_with_path(reply, platform_dialog_open_file("Select File", NULL));
}

static void handle_dialog_save_file(const char *args, ipc_reply_t *reply) 
{
    char default_name[128] = "untitled.txt"; // Fallback default
    // Extract "defaultName": "<string>" if provided in args
    const char *name_ptr = strstr(args, "\"defaultName\":");
    if (name_ptr) 
    {
        name_ptr = strchr(name_ptr + 14, '"');
        if (name_ptr) 
        {
            name_ptr++;
            const char *end_ptr = strchr(name_ptr, '"');
            if (end_ptr) 
            {
                size_t len = end_ptr - name_ptr;
                if (len < sizeof(default_name)) 
                {
                    strncpy(default_name, name_ptr, len);
                    default_name[len] = '\0';
                }
            }
        }
    }
    reply_with_path(reply, platform_dialog_save_file("Save File", NULL, default_name));
}

app_command_fn lookup_command(const char *name) 
{
    uint32_t hash = wyhash32(name);
    // 1. Fast CPU register integer check (Takes 1 CPU cycle, ZERO strcmp!)
    switch (hash) 
    {
        case HASH_WIN_MINIMIZE:
            return (app_command_fn)handle_win_minimize;
        case HASH_WIN_TOGGLE_MAX:
            return (app_command_fn)handle_win_toggle_max;
        case HASH_WIN_CLOSE:
            return (app_command_fn)handle_win_close;
        case HASH_DIALOG_OPEN_FILE:
            return (app_command_fn)handle_dialog_open_file;
        case HASH_WIN_START_DRAG:
            return (app_command_fn)handle_win_start_drag;
        case HASH_DIALOG_SAVE_FILE:
            return (app_command_fn)handle_dialog_save_file;
        case HASH_FS_FILE_INFO:
            return (app_command_fn)handle_fs_file_info;
        case HASH_FS_READ_CHUNK:
            return (app_command_fn)handle_fs_read_chunk;
        case HASH_FS_WRITE_TEXT:
            return (app_command_fn)handle_fs_write_text;
        case HASH_FS_READ_DIR:
            return (app_command_fn)handle_fs_read_dir;
        case HASH_FS_CREATE_FILE:
            return (app_command_fn)handle_fs_create_file;
        case HASH_FS_CREATE_DIR:
            return (app_command_fn)handle_fs_create_dir;
        case HASH_FS_REMOVE:
            return (app_command_fn)handle_fs_remove;
        case HASH_FS_RENAME:
        case HASH_FS_MOVE:
            return (app_command_fn)handle_fs_rename;
        case HASH_CLIPBOARD_WRITE_TEXT:
            return (app_command_fn)handle_clipboard_write;
        case HASH_CLIPBOARD_READ_TEXT:
            return (app_command_fn)handle_clipboard_read;
        case HASH_CLIPBOARD_CLEAR:
            return (app_command_fn)handle_clipboard_clear;
        case HASH_CLIPBOARD_HAS_TEXT:
            return (app_command_fn)handle_clipboard_has_text;
        default:
            // User commands (search 64-slot cmd_table)
            uint32_t idx = hash & (CMD_TABLE_SIZE - 1);
        
            for (int i = 0; i < CMD_TABLE_SIZE; i++) 
            {
                uint32_t slot = (idx + i) & (CMD_TABLE_SIZE - 1);
                if (cmd_table[slot].handler == NULL) 
                    return NULL;
        
                if (cmd_table[slot].hash == hash && strcmp(cmd_table[slot].name, name) == 0)
                    return cmd_table[slot].handler;
            }
            return NULL;
    }
}