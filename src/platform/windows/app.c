#include <windows.h>
#include <stdio.h>
#include "webview2_lite.h"
#include "app.h"
static ICoreWebView2Controller *g_controller = NULL;
static ICoreWebView2 *g_webview    = NULL;
static HWND g_hwnd = NULL;

static HRESULT STDMETHODCALLTYPE dummy_QueryInterface(void *This, REFIID riid, void **ppv) {
    (void)This; (void)riid;
    *ppv = NULL;
    return E_NOINTERFACE;
}
static ULONG STDMETHODCALLTYPE dummy_AddRef(void *This) { 
    (void)This; return 1; 
}
static ULONG STDMETHODCALLTYPE dummy_Release(void *This) { 
    (void)This; return 1; 
}

static HRESULT STDMETHODCALLTYPE OnControllerCreated(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *This,HRESULT result,ICoreWebView2Controller *controller)
{
    (void)This;
    if (FAILED(result) || !controller) return S_OK;

    g_controller = controller;
    controller->lpVtbl->AddRef(controller);

    controller->lpVtbl->get_CoreWebView2(controller, &g_webview);
    windows_ipc_init(g_webview);
    g_webview->lpVtbl->AddRef(g_webview);

    RECT bounds;
    GetClientRect(g_hwnd, &bounds);
    controller->lpVtbl->put_Bounds(controller, bounds);
    controller->lpVtbl->put_IsVisible(controller, TRUE);

    g_webview->lpVtbl->Navigate(g_webview, L"https://www.google.com");

    return S_OK;
}

static ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl ControllerHandlerVtbl = {
    .QueryInterface = (void*)dummy_QueryInterface,
    .AddRef = (void *) dummy_AddRef,
    .Release = (void *) dummy_Release,
    .Invoke = OnControllerCreated
};
static ICoreWebView2CreateCoreWebView2ControllerCompletedHandler ControllerHandler = {
    .lpVtbl = &ControllerHandlerVtbl
};

static HRESULT STDMETHODCALLTYPE OnEnvironmentCreated(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *This,HRESULT result,ICoreWebView2Environment *env)
{
    (void)This;
    if (FAILED(result) || !env) return S_OK;

    env->lpVtbl->CreateCoreWebView2Controller(env, g_hwnd, &ControllerHandler);
    return S_OK;
}

static ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl EnvHandlerVtbl = {
    .QueryInterface = (void*)dummy_QueryInterface,
    .AddRef         = (void*)dummy_AddRef,
    .Release        = (void*)dummy_Release,
    .Invoke         = OnEnvironmentCreated
};
static ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler EnvHandler = {
    .lpVtbl = &EnvHandlerVtbl
};

static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) 
    {
        case WM_DESTROY:
            if(g_controller!=NULL)
            {
                g_controller->lpVtbl->Close(g_controller);
            }
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
            if(g_controller!=NULL)
            {
                RECT bounds;
                GetClientRect(hwnd,&bounds);
                g_controller->lpVtbl->put_Bounds(g_controller,bounds);
            }
            return 0;

        case WM_GETMINMAXINFO: 
        {
            MINMAXINFO *mmi = (MINMAXINFO *)lParam;
            return 0;
        }
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}


int platform_app_run(const app_config_t *config, int argc, char *argv[]) 
{
    HINSTANCE hInstance = GetModuleHandle(NULL);
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    WNDCLASSEXW wc = {0};

    wc.hInstance = hInstance;
    wc.cbSize = sizeof(WNDCLASSEXW);
    wchar_t title_w[256] = L"DesktopApp";
    if (config->window->title) {
        MultiByteToWideChar(CP_UTF8, 0, config->window->title, -1, title_w, 256);
    }
    wc.lpszClassName = L"SankramDesktop";
    wc.lpfnWndProc = WndProc;

    RegisterClassExW(&wc);
    DWORD style = WS_OVERLAPPEDWINDOW;

    if (!config->window->decorations)
        style = WS_POPUP;


    if (!config->window->resizable)
        style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    
    HWND hwnd = CreateWindowExW
    (
        0,
        L"SankramDesktop",
        title_w,
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        config->window->width,
        config->window->height,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    g_hwnd = hwnd;
    HMODULE hLoader = LoadLibraryW(L"WebView2Loader.dll");
    if (hLoader) 
    {
        CreateCoreWebView2EnvironmentWithOptionsFn create_env = (CreateCoreWebView2EnvironmentWithOptionsFn)GetProcAddress(hLoader, "CreateCoreWebView2EnvironmentWithOptions");
        if (create_env) 
        {
            create_env(NULL, NULL, NULL, &EnvHandler);
        }
    }
    if (config->window->maximized)
        ShowWindow(hwnd, SW_MAXIMIZE);
    else
        ShowWindow(hwnd, SW_SHOW);

    UpdateWindow(hwnd);
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}

void platform_app_emit(const char *event, const char *json_data) 
{
    if (!g_webview || !event) return;

    const char *payload = (json_data && *json_data) ? json_data : "null";

    char script[4096];
    snprintf(script, sizeof(script), "window.myapp._emit(\"%s\", %s);", event, payload);

    wchar_t script_w[4096];
    MultiByteToWideChar(CP_UTF8, 0, script, -1, script_w, 4096);
    g_webview->lpVtbl->ExecuteScript(g_webview, script_w, NULL);
}