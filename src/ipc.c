// #include <webkit/webkit.h>
#include "ipc.h"

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

        if (g_strcmp0(cmd, "ping") == 0) 
        {
            JSCValue *res = jsc_value_new_string(ctx, "pong");
            webkit_script_message_reply_return_value(reply, res);
            
        } 
        else if (g_strcmp0(cmd, "greet") == 0)
        {
            JSCValue *name_val = jsc_value_object_get_property(args_val, "name");
            char *name = jsc_value_to_string(name_val);
            
            char greeting[128];
            snprintf(greeting, sizeof(greeting), "Hello, %s!", name);
            
            JSCValue *res = jsc_value_new_string(ctx, greeting);
            webkit_script_message_reply_return_value(reply, res);

            g_free(name);
            g_object_unref(name_val);
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
}