#include <windows.h>
#include <stdio.h>
#include "webview2_lite.h"
#include "app.h"
#include "bridge.h"
static ICoreWebView2Controller *g_controller = NULL;
static ICoreWebView2 *g_webview    = NULL;
static HWND g_hwnd = NULL;
static const app_config_t *g_config = NULL;
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
    wchar_t bridge_w[4096];
    MultiByteToWideChar(CP_UTF8, 0, bridge_js, -1, bridge_w, 4096);
    g_webview->lpVtbl->AddScriptToExecuteOnDocumentCreated(g_webview, bridge_w, NULL);
    g_webview->lpVtbl->AddRef(g_webview);
    ICoreWebView2Settings *settings = NULL;
    g_webview->lpVtbl->get_Settings(g_webview, (void**)&settings);
    if (settings && g_config && g_config->webview) 
    {
        settings->lpVtbl->put_AreDevToolsEnabled(settings, g_config->webview->developer_extras);
    
        settings->lpVtbl->put_AreDefaultContextMenusEnabled(settings, g_config->webview->enable_context_menu);
        
        settings->lpVtbl->put_IsScriptEnabled(settings, g_config->webview->javascript_enabled);
        settings->lpVtbl->Release(settings);
    }
    RECT bounds;
    GetClientRect(g_hwnd, &bounds);
    controller->lpVtbl->put_Bounds(controller, bounds);
    controller->lpVtbl->put_IsVisible(controller, TRUE);

    wchar_t dist_path[MAX_PATH];
    GetFullPathNameW(L"dist", MAX_PATH, dist_path, NULL);

    ICoreWebView2_3 *wv3 = NULL;
    g_webview->lpVtbl->QueryInterface(g_webview, &IID_ICoreWebView2_3, (void**)&wv3);
    if (wv3) 
    {
        wv3->lpVtbl->SetVirtualHostNameToFolderMapping(wv3,L"app.localhost",dist_path,COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW);
        wv3->lpVtbl->Release(wv3);
    }

    g_webview->lpVtbl->Navigate(g_webview, L"https://app.localhost/index.html");

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
            if (g_config && g_config->window) 
            {
                if (g_config->window->min_width) mmi->ptMinTrackSize.x = g_config->window->min_width;
                if (g_config->window->min_height) mmi->ptMinTrackSize.y = g_config->window->min_height;
                if (g_config->window->max_width)  mmi->ptMaxTrackSize.x = g_config->window->max_width;
                if (g_config->window->max_height) mmi->ptMaxTrackSize.y = g_config->window->max_height;
            }
            return 0;
        }
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}


int platform_app_run(const app_config_t *config, int argc, char *argv[]) 
{
    HINSTANCE hInstance = GetModuleHandle(NULL);
    (void)argc;
    (void)argv;
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    WNDCLASSEXW wc = {0};
    g_config = config;

    wc.hInstance = hInstance;
    wc.cbSize = sizeof(WNDCLASSEXW);
    wchar_t title_w[256] = L"DesktopApp";
    if (config->window->title) {
        MultiByteToWideChar(CP_UTF8, 0, config->window->title, -1, title_w, 256);
    }
    wc.lpszClassName = L"VyomWindow";
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
        L"VyomWindow",
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
        CreateCoreWebView2EnvironmentWithOptionsFn create_env = (CreateCoreWebView2EnvironmentWithOptionsFn)(void *)GetProcAddress(hLoader, "CreateCoreWebView2EnvironmentWithOptions");
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
    snprintf(script, sizeof(script), "window.vyom._emit(\"%s\", %s);", event, payload);

    wchar_t script_w[4096];
    MultiByteToWideChar(CP_UTF8, 0, script, -1, script_w, 4096);
    g_webview->lpVtbl->ExecuteScript(g_webview, script_w, NULL);
}