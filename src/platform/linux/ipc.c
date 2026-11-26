// #include <webkit/webkit.h>
#include "ipc.h"

struct ipc_reply {
    JSCContext *ctx;
    WebKitScriptMessageReply *webkit_reply;
};

void ipc_reply_send(ipc_reply_t *reply, const char *json_response) 
{
    JSCValue *res = jsc_value_new_string(reply->ctx, json_response);
    webkit_script_message_reply_return_value(reply->webkit_reply, res);
    g_object_unref(res);
}

void ipc_reply_error(ipc_reply_t *reply, const char *error_message) 
{
    webkit_script_message_reply_return_error_message(reply->webkit_reply, error_message);
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
}