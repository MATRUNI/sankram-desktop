#ifndef REGISTRY_H
#define REGISTRY_H
#include <stdint.h>
#define CMD_TABLE_SIZE 64

typedef struct ipc_reply ipc_reply_t;

typedef void (*app_command_fn)(const char *args, ipc_reply_t *reply);

void app_bind_command(const char *name, app_command_fn handler);

app_command_fn lookup_command(const char *name);

#endif