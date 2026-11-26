#include "internal.h"
#include <stdlib.h>

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
    return result;
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

