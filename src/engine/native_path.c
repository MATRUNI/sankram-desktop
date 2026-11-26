#include "native_path.h"
#include "json_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void handle_path_get_special(const char *args, ipc_reply_t *reply)
{
    char name[64] = {0};
    if (!json_get_string(args, "name", name, sizeof(name))) 
    {
        ipc_reply_error(reply, "Missing 'name' argument");
        return;
    }

    char *path = platform_path_get_special(name);
    if (!path) 
    {
        ipc_reply_error(reply, "Unknown or unsupported special path");
        return;
    }

    // Escape backslashes for valid JSON: C:\Users -> C:\\Users
    size_t len = strlen(path);
    char json[1024];

    int p = sprintf(json, "{\"success\":true,\"path\":\"");
    for (size_t i = 0; i < len; i++) 
    {
        unsigned char c = path[i];
        if (c == '\\')
        {
            json[p++] = '/'; 
        }
        else if (c == '"')
        {
            json[p++] = '\\';
            json[p++] = '"';
        }
        else
        {
            json[p++] = c; 
        }
    }
    json[p++] = '"';
    json[p++] = '}';
    json[p] = '\0';

    ipc_reply_send(reply, json);
    free(path);
}

void handle_path_get_all(const char *args, ipc_reply_t *reply)
{
    (void)args;
    static const char *keys[] = {
        "home", "appData", "localAppData", "desktop", "documents", "downloads", "temp"
    };
    size_t count = sizeof(keys) / sizeof(keys[0]);

    char json[2048];
    int p = sprintf(json, "{");

    for (size_t k = 0; k < count; k++) 
    {
        char *path = platform_path_get_special(keys[k]);
        if (k > 0) json[p++] = ',';
        p += sprintf(json + p, "\"%s\":\"", keys[k]);
        if (path) 
        {
            for (size_t i = 0; path[i] != '\0'; i++) 
            {
                unsigned char c = path[i];
                if (c == '\\')     { json[p++] = '/'; }
                else if (c == '"') { json[p++] = '\\'; json[p++] = '"'; }
                else               { json[p++] = c; }
            }
            free(path);
        }
        json[p++] = '"';
    }

    json[p++] = '}';
    json[p] = '\0';

    ipc_reply_send(reply, json);
}