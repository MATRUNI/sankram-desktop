#include "clipboard.h"
#include "json_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

void handle_clipboard_write(const char *args, ipc_reply_t *reply) 
{
    size_t cap = strlen(args) + 1;
    char *text = malloc(cap);
    if (!text) 
    {
        ipc_reply_error(reply, "Out of memory");
        return;
    }
    
    if (json_get_string(args, "text", text, cap)) 
    {
        platform_clipboard_write_text(text);
        ipc_reply_send(reply, "{\"success\": true}");
    } 
    else 
    ipc_reply_error(reply, "Missing 'text' argument");

    free(text);
}

void handle_clipboard_read(const char *args, ipc_reply_t *reply) 
{
    (void)args;
    char *text = platform_clipboard_read_text();
    if (!text) 
    {
        ipc_reply_send(reply, "{\"text\": \"\"}");
        return;
    }

    size_t len = strlen(text);
    char *json = malloc(len * 2 + 32);
    if (json) 
    {
        int p = sprintf(json, "{\"text\":\"");
        for (size_t i = 0; i < len; i++) 
        {
            unsigned char c = text[i];
            if (c == '"')       { json[p++] = '\\'; json[p++] = '"'; }
            else if (c == '\\') { json[p++] = '\\'; json[p++] = '\\'; }
            else if (c == '\n') { json[p++] = '\\'; json[p++] = 'n'; }
            else if (c == '\r') { json[p++] = '\\'; json[p++] = 'r'; }
            else if (c == '\t') { json[p++] = '\\'; json[p++] = 't'; }
            else                { json[p++] = c; }
        }
        json[p++] = '"';
        json[p++] = '}';
        json[p] = '\0';
        ipc_reply_send(reply, json);
        free(json);
    } 
    else 
    ipc_reply_error(reply, "Out of memory");

    free(text);
}

void handle_clipboard_clear(const char *args, ipc_reply_t *reply) 
{
    (void)args;
    platform_clipboard_clear();
    ipc_reply_send(reply, "{\"success\": true}");
}

void handle_clipboard_has_text(const char *args, ipc_reply_t *reply) 
{
    (void)args;
    bool has = platform_clipboard_has_text();
    ipc_reply_send(reply, has ? "{\"hasText\": true}" : "{\"hasText\": false}");
}