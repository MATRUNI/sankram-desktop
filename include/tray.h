#ifndef TRAY_H
#define TRAY_H

#include "platform.h"

void handle_notification_send(const char *args, ipc_reply_t *reply);
void handle_tray_create(const char *args, ipc_reply_t *reply);
void handle_tray_set_tooltip(const char *args, ipc_reply_t *reply);
void handle_tray_set_menu(const char *args, ipc_reply_t *reply);
void handle_tray_destroy(const char *args, ipc_reply_t *reply);

#endif