#include "internal.h"
#include <stdio.h>
#include <string.h>
#include <mntent.h>
#include <sys/statvfs.h>

char* platform_path_get_special(const char *name)
{
    const char *path = NULL;

    if (g_ascii_strcasecmp(name, "home") == 0 || g_ascii_strcasecmp(name, "user") == 0)
        path = g_get_home_dir();
    else if (g_ascii_strcasecmp(name, "appdata") == 0)
        path = g_get_user_config_dir(); // ~/.config
    else if (g_ascii_strcasecmp(name, "localappdata") == 0)
        path = g_get_user_data_dir();   // ~/.local/share
    else if (g_ascii_strcasecmp(name, "desktop") == 0)
        path = g_get_user_special_dir(G_USER_DIRECTORY_DESKTOP);
    else if (g_ascii_strcasecmp(name, "documents") == 0)
        path = g_get_user_special_dir(G_USER_DIRECTORY_DOCUMENTS);
    else if (g_ascii_strcasecmp(name, "downloads") == 0)
        path = g_get_user_special_dir(G_USER_DIRECTORY_DOWNLOAD);
    else if (g_ascii_strcasecmp(name, "temp") == 0 || g_ascii_strcasecmp(name, "tmp") == 0)
        path = g_get_tmp_dir();

    return path ? g_strdup(path) : NULL;
}

char* platform_fs_get_drives(void)
{
    FILE *mtab = setmntent("/proc/mounts", "r");
    if (!mtab) mtab = setmntent("/etc/mtab", "r");
    if (!mtab) return "[]";

    static char json[4096];

    int p = sprintf(json, "[");
    bool first = true;
    struct mntent *ent;

    while ((ent = getmntent(mtab)) != NULL)
    {
        // Only real storage devices (starting with /dev/ or root /)
        if (strncmp(ent->mnt_fsname, "/dev/", 5) == 0 || strcmp(ent->mnt_dir, "/") == 0)
        {
            if (strstr(ent->mnt_fsname, "/dev/loop")) continue; // Skip snap/loop mounts

            struct statvfs st;
            unsigned long long total = 0, free_b = 0;
            if (statvfs(ent->mnt_dir, &st) == 0)
            {
                total = (unsigned long long)st.f_blocks * st.f_frsize;
                free_b = (unsigned long long)st.f_bavail * st.f_frsize;
            }

            const char *type = "fixed";
            if (strstr(ent->mnt_dir, "/media/") || strstr(ent->mnt_dir, "/mnt/"))
                type = "removable";

            if (!first) json[p++] = ',';
            first = false;

            p += sprintf(json + p, "{\"mount\":\"%s\",\"type\":\"%s\",\"total\":%llu,\"free\":%llu}",
                         ent->mnt_dir, type, total, free_b);
        }
    }

    endmntent(mtab);
    json[p++] = ']';
    json[p] = '\0';
    return json;
}

