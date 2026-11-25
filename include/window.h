#ifndef WINDOW_H
#define WINDOW_H

#include "platform.h"

// Basic Window Controls
void handle_window_minimize(const char *args, ipc_reply_t *reply);
void handle_window_toggle_maximize(const char *args, ipc_reply_t *reply);
void handle_window_close(const char *args, ipc_reply_t *reply);
void handle_window_start_drag(const char *args, ipc_reply_t *reply);

// Geometry & State
void handle_window_set_size(const char *args, ipc_reply_t *reply);
void handle_window_get_size(const char *args, ipc_reply_t *reply);
void handle_window_set_position(const char *args, ipc_reply_t *reply);
void handle_window_get_position(const char *args, ipc_reply_t *reply);
void handle_window_center(const char *args, ipc_reply_t *reply);
void handle_window_set_always_on_top(const char *args, ipc_reply_t *reply);
void handle_window_set_resizable(const char *args, ipc_reply_t *reply);
void handle_window_set_min_size(const char *args, ipc_reply_t *reply);
void handle_window_set_max_size(const char *args, ipc_reply_t *reply);
void handle_window_set_title(const char *args, ipc_reply_t *reply);
void handle_window_is_maximized(const char *args, ipc_reply_t *reply);
void handle_window_is_minimized(const char *args, ipc_reply_t *reply);

#endif