#include "ipc.h"

void app_bind_command(const char *name, app_command_fn handler)
{
    (void)name;
    (void)handler;
}
void ipc_reply_send(ipc_reply_t *reply, const char *json_response)
{
    (void)reply;
    (void)json_response;
}
void ipc_reply_error(ipc_reply_t *reply, const char *error_message)
{
    (void)reply;
    (void)error_message;
}