#ifndef IPC_H
#define IPC_H

#include "registry.h"

#if defined(__linux__)
#include <gtk/gtk.h>
#include <webkit/webkit.h>
void ipc_init(WebKitUserContentManager *manager);
#elif defined(__WIN32)
typedef struct ICoreWebView2 ICoreWebView2;
void windows_ipc_init(ICoreWebView2 *webview); 
#endif

void ipc_reply_send(ipc_reply_t *reply, const char *json_response);
void ipc_reply_error(ipc_reply_t *reply, const char *error_message);

#endif