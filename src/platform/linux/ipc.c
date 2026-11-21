// #include <webkit/webkit.h>
#include "ipc.h"
#define CMD_TABLE_SIZE 64

typedef struct {
    char name[32];
    uint32_t hash;
    app_command_fn handler;
} cmd_entry_t;

struct ipc_reply {
    JSCContext *ctx;
    WebKitScriptMessageReply *webkit_reply;
};

static cmd_entry_t cmd_table[CMD_TABLE_SIZE];

static inline uint32_t fnv1a_32(const char *str) 
{
    uint32_t hash = 2166136261u;
    while (*str) 
    {
        hash ^= (uint8_t)*str++;
        hash *= 16777619u;
    }
    return hash;
}

void ipc_reply_send(ipc_reply_t *reply, const char *json_response) 
{
    JSCValue *res = jsc_value_new_string(reply->ctx, json_response);
    webkit_script_message_reply_return_value(reply->webkit_reply, res);
}

void ipc_reply_error(ipc_reply_t *reply, const char *error_message) 
{
    webkit_script_message_reply_return_error_message(reply->webkit_reply, error_message);
}

void app_bind_command(const char *name, app_command_fn handler) 
{
    uint32_t hash = fnv1a_32(name);
    uint32_t idx = hash & (CMD_TABLE_SIZE - 1);

    for (int i = 0; i < CMD_TABLE_SIZE; i++)
    {
        uint32_t slot = (idx + i) & (CMD_TABLE_SIZE - 1);  // Linear probing

        if (cmd_table[slot].handler == NULL || strcmp(cmd_table[slot].name, name) == 0) 
        {
            strncpy(cmd_table[slot].name, name, sizeof(cmd_table[slot].name) - 1);
            cmd_table[slot].hash = hash;
            cmd_table[slot].handler = handler;
            return;
        }
    }
}

static app_command_fn lookup_command(const char *name) 
{
    uint32_t hash = fnv1a_32(name);
    uint32_t idx = hash & (CMD_TABLE_SIZE - 1);

    for (int i = 0; i < CMD_TABLE_SIZE; i++) 
    {
        uint32_t slot = (idx + i) & (CMD_TABLE_SIZE - 1);
        if (cmd_table[slot].handler == NULL) 
            return NULL;

        if (cmd_table[slot].hash == hash && strcmp(cmd_table[slot].name, name) == 0)
            return cmd_table[slot].handler;
    }
    return NULL;
}

void pong(const char *args, ipc_reply_t *reply)
{
    (void)args;
    ipc_reply_send(reply,"Pong!");
}

void greet(const char *args, ipc_reply_t *reply)
{
    (void)args;
    ipc_reply_send(reply,"Hello there!");
}

static gboolean on_script_message(WebKitUserContentManager *manager, JSCValue *message,WebKitScriptMessageReply *reply, gpointer user_data)
{
    (void)user_data;
    (void)manager;
    JSCContext *ctx = jsc_value_get_context(message);
    if (jsc_value_is_object(message)) 
    {    
        JSCValue *cmd_val = jsc_value_object_get_property(message, "cmd");
        char *cmd = jsc_value_to_string(cmd_val);

        JSCValue *args_val = jsc_value_object_get_property(message, "args");

        app_command_fn f = lookup_command((const char*)cmd);
        if(f!=NULL)
        {
            char *args_json = jsc_value_to_json(args_val,0);
            ipc_reply_t ipc_reply = {
                .ctx = ctx,
                .webkit_reply = reply
            };
            f(args_json? args_json : "{}", &ipc_reply);
            g_free(args_json);
        }
        else
        {
            webkit_script_message_reply_return_error_message(reply, "Command not found");
        }
        g_free(cmd);
        g_object_unref(cmd_val);
        g_object_unref(args_val);
    }
    return TRUE;
}


void ipc_init(WebKitUserContentManager *manager)
{
    g_signal_connect(manager,"script-message-with-reply-received::desktop",G_CALLBACK(on_script_message),NULL);
    webkit_user_content_manager_register_script_message_handler_with_reply(manager, "desktop", NULL);
    app_bind_command("greet",greet);
    app_bind_command("pong",pong);
}