#include "internal.h"
#include <stdlib.h>
#include <string.h>
#include <commdlg.h>
#include <shlobj.h>

static char* win32_file_dialog(const char *title, const char *default_name, BOOL is_save)
{
    wchar_t file_path[MAX_PATH] = {0};
    if (default_name && *default_name)
        MultiByteToWideChar(CP_UTF8, 0, default_name, -1, file_path, MAX_PATH - 1);

    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = g_hwnd;
    ofn.lpstrFile = file_path;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"All Files (*.*)\0*.*\0Text Files (*.txt)\0*.txt\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (is_save)
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

    int len = WideCharToMultiByte(CP_UTF8, 0, file_path, -1, NULL, 0, NULL, NULL);
    if (len <= 0) return NULL;

    char *utf8_path = (char *)malloc(len);
    if (!utf8_path) return NULL;

    WideCharToMultiByte(CP_UTF8, 0, file_path, -1, utf8_path, len, NULL, NULL);
    return utf8_path;
}

char* platform_dialog_open_file(const char *title, const char *filter)
{
    (void)filter;
    return win32_file_dialog(title, NULL, FALSE);
}

char* platform_dialog_save_file(const char *title, const char *filter, const char *default_name)
{
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
    return result_path;
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

