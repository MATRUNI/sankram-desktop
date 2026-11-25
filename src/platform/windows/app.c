#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <commdlg.h>
#include <shlobj.h>
#include "webview2_lite.h"
#include "app.h"
#include "bridge.h"

typedef HRESULT (STDMETHODCALLTYPE *CreateWebViewEnvironmentWithOptionsInternalFn)(
    BOOLEAN unknown1,
    int runtime_type,
    PCWSTR user_data_dir,
    IUnknown *options,
    ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *handler
);

static HMODULE find_and_load_webview2_runtime(void)
{
    const wchar_t *subkeys[] = {
        L"Software\\Microsoft\\EdgeUpdate\\ClientState\\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}",
        L"Software\\WOW6432Node\\Microsoft\\EdgeUpdate\\ClientState\\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}",
        L"Software\\Microsoft\\EdgeUpdate\\Clients\\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}",
        L"Software\\WOW6432Node\\Microsoft\\EdgeUpdate\\Clients\\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}",
    };
    HKEY roots[] = { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER };

    wchar_t runtime_dir[MAX_PATH];

    for (size_t r = 0; r < 2; r++) {
        for (size_t s = 0; s < sizeof(subkeys)/sizeof(subkeys[0]); s++) {
            HKEY hKey;
            if (RegOpenKeyExW(roots[r], subkeys[s], 0, KEY_READ | KEY_WOW64_32KEY, &hKey) == ERROR_SUCCESS) {
                DWORD size = sizeof(runtime_dir);
                if (RegQueryValueExW(hKey, L"EBWebView", NULL, NULL, (LPBYTE)runtime_dir, &size) == ERROR_SUCCESS ||
                    RegQueryValueExW(hKey, L"location", NULL, NULL, (LPBYTE)runtime_dir, &size) == ERROR_SUCCESS) {
                    RegCloseKey(hKey);

                    wchar_t dll_path[MAX_PATH];
                    wcsncpy(dll_path, runtime_dir, MAX_PATH - 1);
#if defined(_WIN64) || defined(__x86_64__)
                    wcsncat(dll_path, L"\\EBWebView\\x64\\EmbeddedBrowserWebView.dll", MAX_PATH - wcslen(dll_path) - 1);
#else
                    wcsncat(dll_path, L"\\EBWebView\\x86\\EmbeddedBrowserWebView.dll", MAX_PATH - wcslen(dll_path) - 1);
#endif
                    // LOAD_WITH_ALTERED_SEARCH_PATH ensures neighboring runtime DLLs resolve correctly
                    HMODULE hMod = LoadLibraryExW(dll_path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
                    if (hMod) return hMod;
                } else {
                    RegCloseKey(hKey);
                }
            }
        }
    }
    return NULL;
}

static ICoreWebView2Controller *g_controller = NULL;
static ICoreWebView2 *g_webview    = NULL;
static HWND g_hwnd = NULL;
static int g_min_width = 0;
static int g_min_height = 0;
static int g_max_width = 0;
static int g_max_height = 0;
static const app_config_t *g_config = NULL;

static HRESULT STDMETHODCALLTYPE dummy_QueryInterface(void *This, REFIID riid, void **ppv) {
    (void)This; (void)riid;
    *ppv = This;
    return S_OK;
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
    wchar_t bridge_w[32768];
    MultiByteToWideChar(CP_UTF8, 0, bridge_js, -1, bridge_w, 32768);
    g_webview->lpVtbl->AddScriptToExecuteOnDocumentCreated(g_webview, bridge_w, NULL);
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
            int min_w = g_min_width  ? g_min_width  : (g_config && g_config->window ? g_config->window->min_width : 0);
            int min_h = g_min_height ? g_min_height : (g_config && g_config->window ? g_config->window->min_height : 0);
            int max_w = g_max_width  ? g_max_width  : (g_config && g_config->window ? g_config->window->max_width : 0);
            int max_h = g_max_height ? g_max_height : (g_config && g_config->window ? g_config->window->max_height : 0);

            if (min_w) mmi->ptMinTrackSize.x = min_w;
            if (min_h) mmi->ptMinTrackSize.y = min_h;
            if (max_w) mmi->ptMaxTrackSize.x = max_w;
            if (max_h) mmi->ptMaxTrackSize.y = max_h;
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
    // NEW (Zero External DLLs!):
    HMODULE hRuntime = find_and_load_webview2_runtime();
    if (hRuntime) 
    {
        CreateWebViewEnvironmentWithOptionsInternalFn create_env = 
            (CreateWebViewEnvironmentWithOptionsInternalFn)(void *)GetProcAddress(hRuntime, "CreateWebViewEnvironmentWithOptionsInternal");
        if (create_env) 
        {
            create_env(TRUE, 0, NULL, NULL, &EnvHandler);
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

void platform_window_minimize(void) 
{
    if (g_hwnd) ShowWindow(g_hwnd, SW_MINIMIZE);
}

void platform_window_toggle_maximize(void) 
{
    if (!g_hwnd) return;
    if (IsZoomed(g_hwnd)) ShowWindow(g_hwnd, SW_RESTORE);
    else ShowWindow(g_hwnd, SW_MAXIMIZE);
}

void platform_window_close(void) 
{
    if (g_hwnd) PostMessageW(g_hwnd, WM_CLOSE, 0, 0);
}

void platform_window_start_drag(void) 
{
    if (!g_hwnd) return;
    ReleaseCapture();
    SendMessageW(g_hwnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
}

// -----------------------------------------------------------------------------
// WINDOW GEOMETRY & STATE (Win32)
// -----------------------------------------------------------------------------
void platform_window_set_size(int width, int height) 
{
    if (!g_hwnd) return;
    SetWindowPos(g_hwnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void platform_window_get_size(int *width, int *height) 
{
    RECT rect;
    if (g_hwnd && GetWindowRect(g_hwnd, &rect)) 
    {
        *width = rect.right - rect.left;
        *height = rect.bottom - rect.top;
    } 
    else 
    {
        *width = 0;
        *height = 0;
    }
}

void platform_window_set_position(int x, int y) 
{
    if (!g_hwnd) return;
    SetWindowPos(g_hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void platform_window_get_position(int *x, int *y) 
{
    RECT rect;
    if (g_hwnd && GetWindowRect(g_hwnd, &rect)) 
    {
        *x = rect.left;
        *y = rect.top;
    } 
    else 
    {
        *x = 0;
        *y = 0;
    }
}

void platform_window_center(void) 
{
    if (!g_hwnd) return;
    RECT rc;
    GetWindowRect(g_hwnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    int screen_w = GetSystemMetrics(SM_CXSCREEN);
    int screen_h = GetSystemMetrics(SM_CYSCREEN);
    int x = (screen_w - w) / 2;
    int y = (screen_h - h) / 2;
    SetWindowPos(g_hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void platform_window_set_always_on_top(bool always_on_top) 
{
    if (!g_hwnd) return;
    SetWindowPos(g_hwnd, always_on_top ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void platform_window_set_resizable(bool resizable) 
{
    if (!g_hwnd) return;
    LONG style = GetWindowLongW(g_hwnd, GWL_STYLE);
    if (resizable) 
        style |= (WS_THICKFRAME | WS_MAXIMIZEBOX);
    else 
        style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);

    SetWindowLongW(g_hwnd, GWL_STYLE, style);
    SetWindowPos(g_hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

void platform_window_set_min_size(int width, int height) 
{
    g_min_width = width;
    g_min_height = height;
}

void platform_window_set_max_size(int width, int height) 
{
    g_max_width = width;
    g_max_height = height;
}

void platform_window_set_title(const char *title) 
{
    if (!g_hwnd || !title) return;
    wchar_t title_w[256] = {0};
    MultiByteToWideChar(CP_UTF8, 0, title, -1, title_w, 255);
    SetWindowTextW(g_hwnd, title_w);
}

bool platform_window_is_maximized(void) 
{
    if (!g_hwnd) return false;
    return IsZoomed(g_hwnd) != 0;
}

bool platform_window_is_minimized(void) 
{
    if (!g_hwnd) return false;
    return IsIconic(g_hwnd) != 0;
}

static char* win32_file_dialog(const char *title, const char *default_name, BOOL is_save) {
    // Fill OPENFILENAMEW...
    wchar_t file_path[MAX_PATH] = {0};
    if (default_name && *default_name) 
    {
        MultiByteToWideChar(CP_UTF8, 0, default_name, -1, file_path, MAX_PATH - 1);
    }
    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = g_hwnd;               // Parent window (makes it modal)
    ofn.lpstrFile = file_path;            // Buffer that receives the selected path
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"All Files (*.*)\0*.*\0Text Files (*.txt)\0*.txt\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST |  OFN_NOCHANGEDIR;
    if(is_save)
        ofn.Flags |= OFN_OVERWRITEPROMPT;
    else 
        ofn.Flags |= OFN_FILEMUSTEXIST;
    
    wchar_t title_w[128] = {0};
    if (title && *title) 
    {
        MultiByteToWideChar(CP_UTF8, 0, title, -1, title_w, 127);
        ofn.lpstrTitle = title_w;
    }
    BOOL ok = is_save ? GetSaveFileNameW(&ofn) : GetOpenFileNameW(&ofn);
    if (!ok) return NULL;
    // UTF-16 to UTF-8 conversion...
    // 1. Calculate how many bytes we need in UTF-8
    int len = WideCharToMultiByte(CP_UTF8, 0, file_path, -1, NULL, 0, NULL, NULL);
    if (len <= 0) return NULL;
    
    // 2. Allocate the buffer (caller will free this)
    char *utf8_path = (char *)malloc(len);
    if (!utf8_path) return NULL;
    
    // 3. Convert UTF-16 -> UTF-8
    WideCharToMultiByte(CP_UTF8, 0, file_path, -1, utf8_path, len, NULL, NULL);
    return utf8_path;
}

char* platform_dialog_open_file(const char *title, const char *filter) {
    (void)filter;
    return win32_file_dialog(title, NULL, FALSE);
}

char* platform_dialog_save_file(const char *title, const char *filter, const char *default_name) {
    (void)filter;
    return win32_file_dialog(title, default_name, TRUE);
}

char* platform_dialog_open_directory(const char *title) 
{
    wchar_t title_w[256] = L"Select Folder";
    if (title && *title) MultiByteToWideChar(CP_UTF8, 0, title, -1, title_w, 256);

    BROWSEINFOW bi = {0};
    bi.hwndOwner = g_hwnd;
    bi.lpszTitle = title_w;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_USENEWUI;

    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (!pidl) return NULL;

    wchar_t path_w[MAX_PATH];
    char *result_path = NULL;
    if (SHGetPathFromIDListW(pidl, path_w)) 
    {
        int len = WideCharToMultiByte(CP_UTF8, 0, path_w, -1, NULL, 0, NULL, NULL);
        if (len > 0) 
        {
            result_path = (char *)malloc(len);
            if (result_path) WideCharToMultiByte(CP_UTF8, 0, path_w, -1, result_path, len, NULL, NULL);
        }
    }

    CoTaskMemFree(pidl);
    return result_path; // Caller frees with free()
}

void platform_dialog_message(const char *title, const char *message, const char *type) 
{
    wchar_t title_w[256] = L"Alert";
    wchar_t msg_w[1024] = L"";

    if (title && *title) MultiByteToWideChar(CP_UTF8, 0, title, -1, title_w, 256);
    if (message && *message) MultiByteToWideChar(CP_UTF8, 0, message, -1, msg_w, 1024);

    UINT icon = MB_ICONINFORMATION;
    if (type) 
    {
        if (strcmp(type, "error") == 0) icon = MB_ICONERROR;
        else if (strcmp(type, "warning") == 0) icon = MB_ICONWARNING;
    }

    MessageBoxW(g_hwnd, msg_w, title_w, MB_OK | icon | MB_TASKMODAL);
}

bool platform_dialog_confirm(const char *title, const char *message) 
{
    wchar_t title_w[256] = L"Confirm";
    wchar_t msg_w[1024] = L"";

    if (title && *title) MultiByteToWideChar(CP_UTF8, 0, title, -1, title_w, 256);
    if (message && *message) MultiByteToWideChar(CP_UTF8, 0, message, -1, msg_w, 1024);

    int res = MessageBoxW(g_hwnd, msg_w, title_w, MB_OKCANCEL | MB_ICONQUESTION | MB_TASKMODAL);
    return res == IDOK;
}

// -----------------------------------------------------------------------------
// NATIVE CLIPBOARD (Win32)
// -----------------------------------------------------------------------------
void platform_clipboard_write_text(const char *text) 
{
    if (!text) return;
    if (!OpenClipboard(g_hwnd)) return;
    EmptyClipboard();

    int len_w = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    if (len_w > 0) 
    {
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len_w * sizeof(wchar_t));
        if (hMem) 
        {
            wchar_t *dst = (wchar_t *)GlobalLock(hMem);
            if (dst) 
            {
                MultiByteToWideChar(CP_UTF8, 0, text, -1, dst, len_w);
                GlobalUnlock(hMem);
                SetClipboardData(CF_UNICODETEXT, hMem);
            }
            else 
                GlobalFree(hMem);
        }
    }
    CloseClipboard();
}

char* platform_clipboard_read_text(void) 
{
    if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) return NULL;
    if (!OpenClipboard(g_hwnd)) return NULL;

    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    char *result = NULL;
    if (hData) 
    {
        wchar_t *src = (wchar_t *)GlobalLock(hData);
        if (src) 
        {
            int len_u8 = WideCharToMultiByte(CP_UTF8, 0, src, -1, NULL, 0, NULL, NULL);
            if (len_u8 > 0) 
            {
                result = (char *)malloc(len_u8);
                if (result) 
                    WideCharToMultiByte(CP_UTF8, 0, src, -1, result, len_u8, NULL, NULL);
            }
            GlobalUnlock(hData);
        }
    }
    CloseClipboard();
    return result; // Caller frees
}

void platform_clipboard_clear(void) 
{
    if (OpenClipboard(g_hwnd)) 
    {
        EmptyClipboard();
        CloseClipboard();
    }
}

bool platform_clipboard_has_text(void) 
{
    return IsClipboardFormatAvailable(CF_UNICODETEXT) != 0;
}