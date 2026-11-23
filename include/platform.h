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

#endif