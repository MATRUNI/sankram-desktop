#include <windows.h>
#include "webview2_lite.h"
#include <stdio.h>

#include "ipc.h"
#include "registry.h"
struct ipc_reply {
    int id;
    ICoreWebView2 *webview;
};

static HRESULT STDMETHODCALLTYPE dummy_QueryInterface(void *This, REFIID riid, void **ppv) {
    (void)This; (void)riid; *ppv = NULL; return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE dummy_AddRef(void *This) { 
    (void)This; return 1; 
}

static ULONG STDMETHODCALLTYPE dummy_Release(void *This) { 
    (void)This; return 1; 
}

static void windows_ipc_handle_message(ICoreWebView2 *sender, const char *json) 
{
    // 1. Extract "id": <number>
    int id = 0;
    const char *id_ptr = strstr(json, "\"id\":");
    if (id_ptr) 
    {
        id = atoi(id_ptr + 5);
    }

    // 2. Extract "cmd": "<string>"
    char cmd[64] = {0};
    const char *cmd_ptr = strstr(json, "\"cmd\":");
    if (cmd_ptr) 
    {
        cmd_ptr = strchr(cmd_ptr + 6, '"');
        if (cmd_ptr) 
        {
            cmd_ptr++;
            const char *cmd_end = strchr(cmd_ptr, '"');
            if (cmd_end) 
            {
                size_t len = cmd_end - cmd_ptr;
                if (len < sizeof(cmd)) 
                {
                    strncpy(cmd, cmd_ptr, len);
                    cmd[len] = '\0';
                }
            }
        }
    }

    // 3. Find args (pass "{}" as fallback)
    const char *args = strstr(json, "\"args\":");
    if (args) args += 7;
    else args = "{}";

    // 4. Dispatch
    app_command_fn handler = lookup_command(cmd);
    ipc_reply_t reply = { .id = id, .webview = sender };

    if (handler) 
    {
        handler(args, &reply);
    } 
    else 
    {
        ipc_reply_error(&reply, "Command not found");
    }
}

void ipc_reply_send(ipc_reply_t *reply, const char *json_response) 
{
    char script[4096];
    snprintf(script, sizeof(script), "window.myapp._emit('__reply_%d', { result: %s });", reply->id, json_response);

    wchar_t script_w[4096];
    MultiByteToWideChar(CP_UTF8, 0, script, -1, script_w, 4096);
    reply->webview->lpVtbl->ExecuteScript(reply->webview, script_w, NULL);
}

void ipc_reply_error(ipc_reply_t *reply, const char *error_message) 
{
    char script[4096];
    snprintf(script, sizeof(script), "window.myapp._emit('__reply_%d', { error: '%s' });", reply->id, error_message);

    wchar_t script_w[4096];
    MultiByteToWideChar(CP_UTF8, 0, script, -1, script_w, 4096);
    reply->webview->lpVtbl->ExecuteScript(reply->webview, script_w, NULL);
}

// 1. Declare the WebMessage handler
static HRESULT STDMETHODCALLTYPE OnWebMessageReceived(ICoreWebView2WebMessageReceivedEventHandler *This, ICoreWebView2 *sender, ICoreWebView2WebMessageReceivedEventArgs *args)
{
    (void)This;
    LPWSTR json_w = NULL;
    args->lpVtbl->get_WebMessageAsJson(args, &json_w);
    if (!json_w) return S_OK;

    char json_utf8[2048];
    WideCharToMultiByte(CP_UTF8, 0, json_w, -1, json_utf8, sizeof(json_utf8), NULL, NULL);
    CoTaskMemFree(json_w);

    windows_ipc_handle_message(sender, json_utf8);

    return S_OK;
}

static ICoreWebView2WebMessageReceivedEventHandlerVtbl WebMessageHandlerVtbl = {
    .QueryInterface = (void*)dummy_QueryInterface,
    .AddRef         = (void*)dummy_AddRef,
    .Release        = (void*)dummy_Release,
    .Invoke         = OnWebMessageReceived
};
static ICoreWebView2WebMessageReceivedEventHandler WebMessageHandler = {
    .lpVtbl = &WebMessageHandlerVtbl
};

void windows_ipc_init(ICoreWebView2 *webview)
{
    EventRegistrationToken token;
    webview->lpVtbl->add_WebMessageReceived(webview, &WebMessageHandler, &token);
}