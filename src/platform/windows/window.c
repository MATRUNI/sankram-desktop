#include "internal.h"

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

void platform_window_set_size(int width, int height)
{
    if (!g_hwnd) return;
    if (IsZoomed(g_hwnd)) ShowWindow(g_hwnd, SW_RESTORE);
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

