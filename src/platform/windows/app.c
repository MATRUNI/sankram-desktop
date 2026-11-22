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

// -----------------------------------------------------------------------------
// HTML5 SPA ROUTING FALLBACK: Catches 404 page requests and rewrites to index.html
// -----------------------------------------------------------------------------
static HRESULT STDMETHODCALLTYPE OnNavigationStarting(ICoreWebView2NavigationStartingEventHandler *This, ICoreWebView2 *sender, ICoreWebView2NavigationStartingEventArgs *args){
    (void)This;
    LPWSTR uri_w = NULL;
    args->lpVtbl->get_Uri(args, &uri_w);
    if (!uri_w) return S_OK;

    const wchar_t *prefix = L"https://app.localhost/";
    size_t prefix_len = wcslen(prefix);

    if (wcsncmp(uri_w, prefix, prefix_len) == 0)
    {
        const wchar_t *rel_path = uri_w + prefix_len;

        // If not root, not index.html, and not already carrying __route=
        if (*rel_path != L'\0' && wcscmp(rel_path, L"index.html") != 0 && !wcsstr(rel_path, L"__route="))
        {
            wchar_t clean_rel[MAX_PATH] = {0};
            wcsncpy(clean_rel, rel_path, MAX_PATH - 1);
            wchar_t *q = wcschr(clean_rel, L'?');
            if (q) *q = L'\0';
            wchar_t *h = wcschr(clean_rel, L'#');
            if (h) *h = L'\0';

            // Only rewrite routes without file extensions (like /dashboard, /users)
            // Leave real assets (.js, .css, .png, etc.) alone
            const wchar_t *dot = wcsrchr(clean_rel, L'.');
            if (!dot) 
            {
                wchar_t disk_path[MAX_PATH];
                GetFullPathNameW(L"dist", MAX_PATH, disk_path, NULL);
                wcscat(disk_path, L"\\");

                wchar_t disk_rel[MAX_PATH] = {0};
                wcsncpy(disk_rel, clean_rel, MAX_PATH - 1);
                for (wchar_t *p = disk_rel; *p; p++) {
                    if (*p == L'/') *p = L'\\';
                }
                wcscat(disk_path, disk_rel);

                DWORD attrs = GetFileAttributesW(disk_path);
                BOOL exists = (attrs != INVALID_FILE_ATTRIBUTES);

                if (!exists)
                {
                    // Physical file not found! SPA HTML5 fallback:
                    // Cancel the 404 navigation and rewrite to index.html?__route=...
                    args->lpVtbl->put_Cancel(args, TRUE);

                    wchar_t fallback[2048];
                    swprintf(fallback, 2048, L"https://app.localhost/index.html?__route=/%ls", clean_rel);
                    sender->lpVtbl->Navigate(sender, fallback);
                }
            }
        }
    }

    CoTaskMemFree(uri_w);
    return S_OK;
}

static ICoreWebView2NavigationStartingEventHandlerVtbl NavStartingHandlerVtbl = {
    .QueryInterface = (void*)dummy_QueryInterface,
    .AddRef         = (void*)dummy_AddRef,
    .Release        = (void*)dummy_Release,
    .Invoke         = OnNavigationStarting
};
static ICoreWebView2NavigationStartingEventHandler NavStartingHandler = {
    .lpVtbl = &NavStartingHandlerVtbl
};

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
    // -------------------------------------------------------------------------
    // WEBVIEW SETTINGS: Map webview_config_t into WebView2 COM properties
    // -------------------------------------------------------------------------
    ICoreWebView2Settings *settings = NULL;
    g_webview->lpVtbl->get_Settings(g_webview, (void**)&settings);
    if (settings && g_config && g_config->webview) 
    {
        // developer_extras: Enables F12 Developer Tools / Inspect Element
        settings->lpVtbl->put_AreDevToolsEnabled(settings, g_config->webview->developer_extras);
    
        // enable_context_menu: Right-click context menu (Inspect, Back, Reload)
        settings->lpVtbl->put_AreDefaultContextMenusEnabled(settings, g_config->webview->enable_context_menu);
        
        // javascript_enabled: Enable or disable JavaScript execution in pages
        settings->lpVtbl->put_IsScriptEnabled(settings, g_config->webview->javascript_enabled);

        settings->lpVtbl->Release(settings);
    }
    RECT bounds;
    GetClientRect(g_hwnd, &bounds);
    controller->lpVtbl->put_Bounds(controller, bounds);
    controller->lpVtbl->put_IsVisible(controller, TRUE);

    // -------------------------------------------------------------------------
    // VIRTUAL HOST MAPPING: Serves local "dist/" folder over https://app.localhost
    // Avoids CORS, local file:// security limits, and root-path routing issues.
    // -------------------------------------------------------------------------
    wchar_t dist_path[MAX_PATH];
    GetFullPathNameW(L"dist", MAX_PATH, dist_path, NULL);

    ICoreWebView2_3 *wv3 = NULL;
    g_webview->lpVtbl->QueryInterface(g_webview, &IID_ICoreWebView2_3, (void**)&wv3);
    if (wv3) 
    {
        wv3->lpVtbl->SetVirtualHostNameToFolderMapping(wv3,L"app.localhost",dist_path,COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW);
        wv3->lpVtbl->Release(wv3);
    }

    // -------------------------------------------------------------------------
    // HTML5 SPA ROUTING FALLBACK: Intercept 404 navigations and rewrite to index.html
    // -------------------------------------------------------------------------
    EventRegistrationToken nav_token;
    g_webview->lpVtbl->add_NavigationStarting(g_webview, &NavStartingHandler, &nav_token);

    // Initial navigation URL
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

        // ---------------------------------------------------------------------
        // WM_GETMINMAXINFO: Enforces min_width, min_height, max_width, max_height
        // Windows sends this message before resizing or maximizing.
        // We override ptMinTrackSize / ptMaxTrackSize to restrict window resizing.
        // ---------------------------------------------------------------------
        case WM_GETMINMAXINFO: 
        {
            MINMAXINFO *mmi = (MINMAXINFO *)lParam;
            if (g_config && g_config->window) 
            {
                if (g_config->window->min_width)  mmi->ptMinTrackSize.x = g_config->window->min_width;
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
    // -------------------------------------------------------------------------
    // 1. WINDOW TITLE (UTF-8 to UTF-16 Conversion)
    // Win32 Unicode APIs (CreateWindowExW) require wide characters (wchar_t).
    // Convert config->window->title from standard C UTF-8 into Windows UTF-16.
    // -------------------------------------------------------------------------
    wchar_t title_w[256] = L"DesktopApp";
    if (config->window->title) {
        MultiByteToWideChar(CP_UTF8, 0, config->window->title, -1, title_w, 256);
    }
    wc.lpszClassName = L"VyomWindow";
    wc.lpfnWndProc = WndProc;

    RegisterClassExW(&wc);

    // -------------------------------------------------------------------------
    // 2. WINDOW STYLES (Decorations & Resizability)
    // WS_OVERLAPPEDWINDOW = Titlebar + Borders + System Menu + Min/Max/Close
    // -------------------------------------------------------------------------
    DWORD style = WS_OVERLAPPEDWINDOW;

    // Frameless window: If decorations are disabled, strip titlebar/borders (WS_POPUP)
    if (!config->window->decorations)
        style = WS_POPUP;

    // Resizability: Remove thick resizing border (WS_THICKFRAME) and maximize button
    if (!config->window->resizable)
        style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    
    // -------------------------------------------------------------------------
    // 3. CREATE WINDOW (Width & Height)
    // CW_USEDEFAULT lets Windows decide initial X/Y placement on screen.
    // -------------------------------------------------------------------------
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

    // -------------------------------------------------------------------------
    // 4. INITIALIZE WEBVIEW2 (Asynchronous COM Environment Creation)
    // Loads WebView2Loader.dll and starts the Chromium controller pipeline.
    // -------------------------------------------------------------------------
    HMODULE hLoader = LoadLibraryW(L"WebView2Loader.dll");
    if (hLoader) 
    {
        CreateCoreWebView2EnvironmentWithOptionsFn create_env = (CreateCoreWebView2EnvironmentWithOptionsFn)(void *)GetProcAddress(hLoader, "CreateCoreWebView2EnvironmentWithOptions");
        if (create_env) 
        {
            create_env(NULL, NULL, NULL, &EnvHandler);
        }
    }

    // -------------------------------------------------------------------------
    // 5. WINDOW DISPLAY STATE (Maximized vs Normal)
    // -------------------------------------------------------------------------
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