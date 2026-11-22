#include "app.h"

void handle_system_info(const char *args, ipc_reply_t *reply) 
{
    (void)args;
    ipc_reply_send(reply, "{\"os\": \"Linux\", \"arch\": \"x86_64\"}");
}

void handle_test_emit(const char *args, ipc_reply_t *reply) 
{
    (void)args;
    app_emit("server_alert", "{\"msg\": \"Hello from C event!\", \"value\": 42}");
    ipc_reply_send(reply, "{\"status\": \"emitted\"}");
}

int main(int argc, char *argv[]) 
{
    app_bind_command("get_sysinfo", handle_system_info);
    window_config_t win_cfg = {
        .title = "Vyom",
        .width = 1200,
        .height = 800,
        .min_width = 600,
        .min_height = 400,
        .resizable = true,
        .decorations = true
    };

    webview_config_t web_cfg = {
        .url = "app://localhost/index.html",
        .developer_extras = true,
        .disable_web_security = true,
        .javascript_enabled = true,
        .auto_load_images = true,
    };

    app_config_t config = {
        .window = &win_cfg,
        .webview = &web_cfg
    };
    app_bind_command("trigger_alert", handle_test_emit);
    return app_run(&config, argc, argv);
}