#include "internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shlobj.h>

char* platform_path_get_special(const char *name)
{
    char buf[MAX_PATH];

    if (_stricmp(name, "home") == 0 || _stricmp(name, "user") == 0)
    {
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, buf))) return _strdup(buf);
    }
    else if (_stricmp(name, "appdata") == 0)
    {
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, buf))) return _strdup(buf);
    }
    else if (_stricmp(name, "localappdata") == 0)
    {
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, buf))) return _strdup(buf);
    }
    else if (_stricmp(name, "desktop") == 0)
    {
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, buf))) return _strdup(buf);
    }
    else if (_stricmp(name, "documents") == 0)
    {
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, buf))) return _strdup(buf);
    }
    else if (_stricmp(name, "temp") == 0 || _stricmp(name, "tmp") == 0)
    {
        if (GetTempPathA(sizeof(buf), buf))
        {
            size_t l = strlen(buf);
            if (l > 1 && (buf[l - 1] == '\\' || buf[l - 1] == '/')) buf[l - 1] = '\0';
            return _strdup(buf);
        }
    }
    else if (_stricmp(name, "downloads") == 0)
    {
        static const GUID FOLDERID_Downloads_Local = { 0x374de290, 0x123f, 0x4565, { 0x91, 0x64, 0x39, 0xc4, 0x92, 0x5e, 0x46, 0x7b } };
        PWSTR wpath = NULL;
        if (SUCCEEDED(SHGetKnownFolderPath(&FOLDERID_Downloads_Local, 0, NULL, &wpath)))
        {
            int len = WideCharToMultiByte(CP_UTF8, 0, wpath, -1, NULL, 0, NULL, NULL);
            if (len > 0)
            {
                char *res = malloc(len);
                if (res) WideCharToMultiByte(CP_UTF8, 0, wpath, -1, res, len, NULL, NULL);
                CoTaskMemFree(wpath);
                return res;
            }
            CoTaskMemFree(wpath);
        }
    }

    return NULL;
}

char* platform_fs_get_drives(void)
{
    DWORD mask = GetLogicalDrives();
    static char json[4096];

    int p = sprintf(json, "[");
    bool first = true;

    for (int i = 0; i < 26; i++)
    {
        if (mask & (1 << i))
        {
            char root[4] = { (char)('A' + i), ':', '\\', '\0' };
            UINT dtype = GetDriveTypeA(root);
            const char *type_str = "unknown";
            if (dtype == DRIVE_FIXED)          type_str = "fixed";
            else if (dtype == DRIVE_REMOVABLE) type_str = "removable";
            else if (dtype == DRIVE_REMOTE)    type_str = "remote";
            else if (dtype == DRIVE_CDROM)     type_str = "cdrom";
            else if (dtype == DRIVE_RAMDISK)   type_str = "ramdisk";

            ULARGE_INTEGER free_bytes, total_bytes;
            unsigned long long total = 0, free_b = 0;
            if (GetDiskFreeSpaceExA(root, &free_bytes, &total_bytes, NULL))
            {
                total = total_bytes.QuadPart;
                free_b = free_bytes.QuadPart;
            }

            if (!first) json[p++] = ',';
            first = false;

            p += sprintf(json + p, "{\"mount\":\"%c:/\",\"type\":\"%s\",\"total\":%llu,\"free\":%llu}",
                         'A' + i, type_str, total, free_b);
        }
    }

    json[p++] = ']';
    json[p] = '\0';
    return json;
}

