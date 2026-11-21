#include <windows.h>
#include <WebView2.h>
#include "app.h"


static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) 
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_SIZE:
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
    (void)event;
    (void)json_data;
}