#include "app.h"

int main(int argc, char *argv[]) {
    window_config_t win_cfg = {
        .title = "My app Desktop",
        .width = 1200,
        .height = 800,
        .min_width = 600,
        .min_height = 400,
        .resizable = true,
        .decorations = true
    };

    webview_config_t web_cfg = {
        .url = "http://localhost:5173",
        .developer_extras = true,
        .disable_web_security = true,
        .javascript_enabled = true,
        .auto_load_images = true,
        .startup_script =
        "window.myapp = {"
        "  invoke: function(cmd, args = {}) {"
        "    return window.webkit.messageHandlers.desktop.postMessage({ cmd, args });"
        "  }"
        "};"
    };

    app_config_t config = {
        .window = &win_cfg,
        .webview = &web_cfg
    };

    return app_run(&config, argc, argv);
}