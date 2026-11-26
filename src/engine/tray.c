#include "tray.h"
#include "json_util.h"
#include <string.h>

#define MAX_TRAY_MENU_ITEMS 32

void handle_notification_send(const char *args, ipc_reply_t *reply)
{
    char title[256] = "Vyom";
    char body[1024] = "";
    char icon[256] = "";

    json_get_string(args, "title", title, sizeof(title));
    json_get_string(args, "body", body, sizeof(body));
    json_get_string(args, "icon", icon, sizeof(icon));

    platform_notification_send(title, body, icon);
    ipc_reply_send(reply, "{\"success\":true}");
}

void handle_tray_create(const char *args, ipc_reply_t *reply)
{
    char tooltip[256] = "Vyom";
    char icon[256] = "";

    json_get_string(args, "tooltip", tooltip, sizeof(tooltip));
    json_get_string(args, "icon", icon, sizeof(icon));

    platform_tray_create(tooltip, icon);
    ipc_reply_send(reply, "{\"success\":true}");
}

void handle_tray_set_tooltip(const char *args, ipc_reply_t *reply)
{
    char tooltip[256] = "";
    if (!json_get_string(args, "tooltip", tooltip, sizeof(tooltip)))
    {
        ipc_reply_error(reply, "Missing 'tooltip' argument");
        return;
    }

    platform_tray_set_tooltip(tooltip);
    ipc_reply_send(reply, "{\"success\":true}");
}

void handle_tray_set_menu(const char *args, ipc_reply_t *reply)
{
    tray_menu_item_t items[MAX_TRAY_MENU_ITEMS];
    int count = 0;

    const char *arr = strstr(args, "\"items\"");
    if (arr) arr = strchr(arr, '[');

    if (arr)
    {
        const char *p = arr + 1;
        while (*p && *p != ']' && count < MAX_TRAY_MENU_ITEMS)
        {
            const char *obj_start = strchr(p, '{');
            if (!obj_start) break;

            // Find matching closing brace (respecting strings)
            const char *q = obj_start + 1;
            int depth = 1;
            bool in_str = false;
            while (*q && depth > 0)
            {
                if (*q == '"' && *(q - 1) != '\\') in_str = !in_str;
                else if (!in_str)
                {
                    if (*q == '{') depth++;
                    else if (*q == '}') depth--;
                }
                q++;
            }
            if (depth != 0) break;

            size_t obj_len = (size_t)(q - obj_start);
            char obj_buf[512];
            if (obj_len < sizeof(obj_buf))
            {
                memcpy(obj_buf, obj_start, obj_len);
                obj_buf[obj_len] = '\0';

                memset(&items[count], 0, sizeof(tray_menu_item_t));
                items[count].separator = json_get_bool(obj_buf, "separator", false);
                json_get_string(obj_buf, "id", items[count].id, sizeof(items[count].id));
                json_get_string(obj_buf, "label", items[count].label, sizeof(items[count].label));
                count++;
            }
            p = q;
        }
    }

    platform_tray_set_menu(items, count);
    ipc_reply_send(reply, "{\"success\":true}");
}

void handle_tray_destroy(const char *args, ipc_reply_t *reply)
{
    (void)args;
    platform_tray_destroy();
    ipc_reply_send(reply, "{\"success\":true}");
}