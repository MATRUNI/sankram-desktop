#ifndef PLATFORM_H
#define PLATFORM_H

#include "app.h"

// 1. Starts the native OS event loop (GTK4 on Linux, Win32 on Windows, Cocoa on Mac)
int platform_app_run(const app_config_t *config, int argc, char *argv[]);

// 2. Evaluates JavaScript in the active webview
void platform_app_emit(const char *event, const char *json_data);

// 3. Basic controls of open window
// Window controls (Direct native calls)
void platform_window_minimize(void);
void platform_window_toggle_maximize(void);
void platform_window_close(void);
void platform_window_start_drag(void);

// 4. Native Dialogs: returns dynamically allocated UTF-8 path string, or NULL if cancelled
char* platform_dialog_open_file(const char *title, const char *filter);

char* platform_dialog_save_file(const char *title, const char *filter, const char *default_name);

// 5. Native Clipboard
void  platform_clipboard_write_text(const char *text);

char* platform_clipboard_read_text(void);

void  platform_clipboard_clear(void);

bool  platform_clipboard_has_text(void);

#endif