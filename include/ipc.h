#ifndef IPC_H
#define IPC_H

#if defined(__linux__)
#include <gtk/gtk.h>
#include <webkit/webkit.h>
void ipc_init(WebKitUserContentManager *manager);
#endif

typedef struct ipc_reply ipc_reply_t;
typedef void (*app_command_fn)(const char *args, ipc_reply_t *reply);

void app_bind_command(const char *name, app_command_fn handler);
void ipc_reply_send(ipc_reply_t *reply, const char *json_response);
void ipc_reply_error(ipc_reply_t *reply, const char *error_message);

#endif