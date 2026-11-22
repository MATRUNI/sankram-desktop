#include "hash.h"
#include "registry.h"
#include <stdio.h>
#include <string.h>

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

app_command_fn lookup_command(const char *name) 
{
    uint32_t hash = fnv1a_32(name);
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