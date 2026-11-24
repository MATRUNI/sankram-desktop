#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include "platform.h"

void handle_clipboard_write(const char *args, ipc_reply_t *reply);

void handle_clipboard_read(const char *args, ipc_reply_t *reply);

void handle_clipboard_clear(const char *args, ipc_reply_t *reply);

void handle_clipboard_has_text(const char *args, ipc_reply_t *reply);

#endif