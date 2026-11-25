#include "window.h"
#include "json_util.h"
#include <stdio.h>
#include <stdbool.h>

void handle_window_minimize(const char *args, ipc_reply_t *reply) 
{
    (void)args;
    platform_window_minimize();
    ipc_reply_send(reply, "{\"success\":true}");
}

void handle_window_toggle_maximize(const char *args, ipc_reply_t *reply) 
{
    (void)args;
    platform_window_toggle_maximize();
    ipc_reply_send(reply, "{\"success\":true}");
}

void handle_window_close(const char *args, ipc_reply_t *reply) 
{
    (void)args;
    platform_window_close();
    ipc_reply_send(reply, "{\"success\":true}");
}

void handle_window_start_drag(const char *args, ipc_reply_t *reply) 
{
    (void)args;
    platform_window_start_drag();
    ipc_reply_send(reply, "{\"success\":true}");
}

void handle_window_set_size(const char *args, ipc_reply_t *reply) 
{
    uint64_t w = json_get_uint64(args, "width", 0);
    uint64_t h = json_get_uint64(args, "height", 0);
    if (w > 0 && h > 0) 
    {
        platform_window_set_size((int)w, (int)h);
        ipc_reply_send(reply, "{\"success\":true}");
    } 
    else ipc_reply_error(reply, "Invalid width or height");
}

void handle_window_get_size(const char *args, ipc_reply_t *reply) 
{
    (void)args;
    int w = 0, h = 0;
    platform_window_get_size(&w, &h);
    char buf[64];
    snprintf(buf, sizeof(buf), "{\"width\":%d,\"height\":%d}", w, h);
    ipc_reply_send(reply, buf);
}

void handle_window_set_position(const char *args, ipc_reply_t *reply) 
{
    uint64_t x = json_get_uint64(args, "x", 0);
    uint64_t y = json_get_uint64(args, "y", 0);
    platform_window_set_position((int)x, (int)y);
    ipc_reply_send(reply, "{\"success\":true}");
}

void handle_window_get_position(const char *args, ipc_reply_t *reply) 
{
    (void)args;
    int x = 0, y = 0;
    platform_window_get_position(&x, &y);
    char buf[64];
    snprintf(buf, sizeof(buf), "{\"x\":%d,\"y\":%d}", x, y);
    ipc_reply_send(reply, buf);
}

void handle_window_center(const char *args, ipc_reply_t *reply) 
{
    (void)args;
    platform_window_center();
    ipc_reply_send(reply, "{\"success\":true}");
}

void handle_window_set_always_on_top(const char *args, ipc_reply_t *reply) 
{
    bool on_top = json_get_bool(args, "alwaysOnTop", false);
    platform_window_set_always_on_top(on_top);
    ipc_reply_send(reply, "{\"success\":true}");
}

void handle_window_set_resizable(const char *args, ipc_reply_t *reply) 
{
    bool resizable = json_get_bool(args, "resizable", true);
    platform_window_set_resizable(resizable);
    ipc_reply_send(reply, "{\"success\":true}");
}

void handle_window_set_min_size(const char *args, ipc_reply_t *reply) 
{
    uint64_t w = json_get_uint64(args, "width", 0);
    uint64_t h = json_get_uint64(args, "height", 0);
    platform_window_set_min_size((int)w, (int)h);
    ipc_reply_send(reply, "{\"success\":true}");
}

void handle_window_set_max_size(const char *args, ipc_reply_t *reply) 
{
    uint64_t w = json_get_uint64(args, "width", 0);
    uint64_t h = json_get_uint64(args, "height", 0);
    platform_window_set_max_size((int)w, (int)h);
    ipc_reply_send(reply, "{\"success\":true}");
}

void handle_window_set_title(const char *args, ipc_reply_t *reply) 
{
    char title[256] = {0};
    if (json_get_string(args, "title", title, sizeof(title))) 
    {
        platform_window_set_title(title);
        ipc_reply_send(reply, "{\"success\":true}");
    } 
    else ipc_reply_error(reply, "Missing 'title' parameter");
}

void handle_window_is_maximized(const char *args, ipc_reply_t *reply) 
{
    (void)args;
    ipc_reply_send(reply, platform_window_is_maximized() ? "{\"maximized\":true}" : "{\"maximized\":false}");
}

void handle_window_is_minimized(const char *args, ipc_reply_t *reply) 
{
    (void)args;
    ipc_reply_send(reply, platform_window_is_minimized() ? "{\"minimized\":true}" : "{\"minimized\":false}");
}