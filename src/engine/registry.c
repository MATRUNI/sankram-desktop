#include "hash.h"
#include "registry.h"
#include "platform.h"
#include <stdio.h>
#include <string.h>

#define HASH_WIN_MINIMIZE    0xf460cba8u  // fnv1a_32("vyom:window:minimize")
#define HASH_WIN_TOGGLE_MAX  0x17a84d23u  // fnv1a_32("vyom:window:toggle_maximize")
#define HASH_WIN_CLOSE       0x0590e82cu  // fnv1a_32("vyom:window:close")

typedef struct {
    char name[32];
    uint32_t hash;
    app_command_fn handler;
} cmd_entry_t;

static cmd_entry_t cmd_table[CMD_TABLE_SIZE];

void app_bind_command(const char *name, app_command_fn handler) 
{
    uint32_t hash = fnv1a_32(name);
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

app_command_fn lookup_command(const char *name) 
{
    uint32_t hash = fnv1a_32(name);
    // 1. Fast CPU register integer check (Takes 1 CPU cycle, ZERO strcmp!)
    switch (hash) 
    {
        case HASH_WIN_MINIMIZE:
            return (app_command_fn)handle_win_minimize;
        case HASH_WIN_TOGGLE_MAX:
            return (app_command_fn)handle_win_toggle_max;
        case HASH_WIN_CLOSE:
            return (app_command_fn)handle_win_close;
    }

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