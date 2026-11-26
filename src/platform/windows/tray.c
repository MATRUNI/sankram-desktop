#include "internal.h"
#include <shellapi.h>
#include <stdio.h>
#include <string.h>

static NOTIFYICONDATAW g_nid = {0};
static bool g_tray_added = false;
static tray_menu_item_t g_tray_items[32];
static int g_tray_item_count = 0;

static void ensure_tray_icon(const char *tooltip)
{
    if (!g_hwnd) return;
    if (!g_tray_added)
    {
        memset(&g_nid, 0, sizeof(g_nid));
        g_nid.cbSize = sizeof(NOTIFYICONDATAW);
        g_nid.hWnd = g_hwnd;
        g_nid.uID = 1;
        g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        g_nid.uCallbackMessage = WM_TRAYICON;
        g_nid.hIcon = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
        wcsncpy(g_nid.szTip, L"Vyom", 127);
        if (tooltip && *tooltip)
            MultiByteToWideChar(CP_UTF8, 0, tooltip, -1, g_nid.szTip, 127);
        Shell_NotifyIconW(NIM_ADD, &g_nid);
        g_tray_added = true;
    }
}

void platform_notification_send(const char *title, const char *body, const char *icon)
{
    (void)icon;
    if (!g_hwnd) return;
    bool temporary = !g_tray_added;
    ensure_tray_icon("Vyom");

    NOTIFYICONDATAW nid = g_nid;
    nid.uFlags = NIF_INFO;
    nid.dwInfoFlags = NIIF_INFO;
    wcsncpy(nid.szInfoTitle, L"Vyom", 63);
    nid.szInfo[0] = L'\0';
    if (title && *title) MultiByteToWideChar(CP_UTF8, 0, title, -1, nid.szInfoTitle, 63);
    if (body && *body)   MultiByteToWideChar(CP_UTF8, 0, body, -1, nid.szInfo, 255);

    Shell_NotifyIconW(NIM_MODIFY, &nid);
    (void)temporary;
}

void platform_tray_create(const char *tooltip, const char *icon)
{
    (void)icon;
    ensure_tray_icon(tooltip);
}

void platform_tray_set_tooltip(const char *tooltip)
{
    if (!g_tray_added || !tooltip) return;
    g_nid.uFlags = NIF_TIP;
    MultiByteToWideChar(CP_UTF8, 0, tooltip, -1, g_nid.szTip, 127);
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);
}

void platform_tray_set_menu(const tray_menu_item_t *items, int count)
{
    if (count > 32) count = 32;
    g_tray_item_count = count;
    if (count > 0 && items)
        memcpy(g_tray_items, items, (size_t)count * sizeof(tray_menu_item_t));
}

void platform_tray_destroy(void)
{
    if (g_tray_added)
    {
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        g_tray_added = false;
    }
}

void win32_tray_cleanup(void)
{
    platform_tray_destroy();
}

bool win32_tray_handle_message(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_TRAYICON)
    {
        if (lParam == WM_LBUTTONUP)
        {
            ShowWindow(hwnd, SW_RESTORE);
            SetForegroundWindow(hwnd);
            platform_app_emit("tray:click", "{}");
        }
        else if (lParam == WM_RBUTTONUP && g_tray_item_count > 0)
        {
            HMENU hMenu = CreatePopupMenu();
            for (int i = 0; i < g_tray_item_count; i++)
            {
                if (g_tray_items[i].separator)
                {
                    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
                }
                else
                {
                    wchar_t label_w[128] = {0};
                    MultiByteToWideChar(CP_UTF8, 0, g_tray_items[i].label, -1, label_w, 127);
                    AppendMenuW(hMenu, MF_STRING, TRAY_CMD_BASE + i, label_w);
                }
            }
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hwnd);
            TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
        }
        return true;
    }

    if (uMsg == WM_COMMAND && LOWORD(wParam) >= TRAY_CMD_BASE && LOWORD(wParam) < TRAY_CMD_BASE + g_tray_item_count)
    {
        int idx = LOWORD(wParam) - TRAY_CMD_BASE;
        char payload[128];
        snprintf(payload, sizeof(payload), "{\"id\":\"%s\"}", g_tray_items[idx].id);
        platform_app_emit("tray:menu_click", payload);
        return true;
    }

    return false;
}
