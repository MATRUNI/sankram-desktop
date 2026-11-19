#ifndef IPC_H
#define IPC_H

#include <webkit/webkit.h>

typedef struct ipc_reply ipc_reply_t;

void ipc_init(WebKitUserContentManager *manager);

typedef void (*app_command_fn)(const char *args, ipc_reply_t *reply);

void app_bind_command(const char *name, app_command_fn handler);

void ipc_reply_send(ipc_reply_t *reply, const char *json_response);

void ipc_reply_error(ipc_reply_t *reply, const char *error_message);

#endif