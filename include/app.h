#ifndef APP_H
#define APP_H

#include "ipc.h"

#include <stdbool.h>

typedef struct {
    const char *title;
    int width;
    int height;

    int min_width;
    int min_height;

    int max_width;
    int max_height;

    bool resizable;
    bool decorations;
    bool maximized;
    bool fullscreen;
    bool center_window;

    const char *icon_name;
} window_config_t;


typedef struct {
    const char *url;
    const char *user_agent;
    const char *custom_css;
    const char *startup_script;

    bool javascript_enabled;
    bool developer_extras;
    bool disable_web_security;

    bool auto_load_images;
    bool javascript_can_open_windows;
    bool enable_local_storage;
    bool enable_context_menu;
} webview_config_t;


typedef struct {
    window_config_t *window;
    webview_config_t *webview;
} app_config_t;

int app_run(const app_config_t *config, int argc, char *argv[]);

void app_emit(const char *event, const char *json_data);

#endif