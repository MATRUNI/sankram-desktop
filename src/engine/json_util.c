#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "json_util.h"

bool json_get_string(const char *json, const char *key, char *out, size_t max_len) 
{
    if (!json || !key || !out || max_len == 0) return false;
    char search[64];
    snprintf(search, sizeof(search), "\"%s\":", key);

    const char *p = strstr(json, search);
    if (!p) return false;
    p += strlen(search);

    while (*p == ' ' || *p == '\t') p++;
    if (*p != '"') return false;
    p++;

    size_t idx = 0;
    while (*p && *p != '"' && idx < max_len - 1) 
    {
        if (*p == '\\') 
        {
            p++;
            if (*p == '"') out[idx++] = '"';
            else if (*p == '\\') out[idx++] = '\\';
            else if (*p == 'n') out[idx++] = '\n';
            else if (*p == 'r') out[idx++] = '\r';
            else if (*p == 't') out[idx++] = '\t';
            else if (*p) out[idx++] = *p;
            else break;
        } else {
            out[idx++] = *p;
        }
        p++;
    }
    out[idx] = '\0';
    return true;
}

uint64_t json_get_uint64(const char *json, const char *key, uint64_t default_val) 
{
    if (!json || !key) return default_val;

    char search[64];
    snprintf(search, sizeof(search), "\"%s\":", key);

    const char *p = strstr(json, search);
    if (!p) return default_val;

    p += strlen(search);

    while (*p == ' ' || *p == '\t') p++;

    char *endptr = NULL;
    unsigned long long val = strtoull(p, &endptr, 10);

    return (endptr == p) ? default_val : (uint64_t)val;
}

bool json_get_bool(const char *json, const char *key, bool default_val) 
{
    if (!json || !key) return default_val;
    char search[64];
    snprintf(search, sizeof(search), "\"%s\":", key);

    const char *p = strstr(json, search);
    if (!p) return default_val;
    p += strlen(search);

    while (*p == ' ' || *p == '\t') p++;

    if (strncmp(p, "true", 4) == 0) return true;
    if (strncmp(p, "false", 5) == 0) return false;
    
    return default_val;
}