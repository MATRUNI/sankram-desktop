#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include "platform.h"
#include "fs.h"
#include "json_util.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <sys/stat.h>

#if defined(_WIN32)
  #define vyom_stat _stat64
  #define vyom_stat_struct struct _stat64
  #define vyom_fseek _fseeki64
  #ifndef S_ISDIR
    #define S_ISDIR(m) (((m) & _S_IFDIR) != 0)
  #endif
#else
  #define vyom_stat stat
  #define vyom_stat_struct struct stat
  #define vyom_fseek fseeko
#endif

// 1. Metadata: getFileInfo
void handle_fs_file_info(const char *args, ipc_reply_t *reply) 
{
    char path[1024];
    if (!json_get_string(args, "path", path, sizeof(path))) 
    {
        ipc_reply_error(reply, "Missing 'path' argument");
        return;
    }

    vyom_stat_struct st;
    if (vyom_stat(path, &st) == 0) 
    {
        char json[256];
        bool is_dir = S_ISDIR(st.st_mode);
        snprintf(json, sizeof(json),
                 "{\"exists\": true, \"size\": %" PRIu64 ", \"isDir\": %s}",
                 (uint64_t)st.st_size, is_dir ? "true" : "false");
        ipc_reply_send(reply, json);
    } 
    else 
    {
        ipc_reply_send(reply, "{\"exists\": false, \"size\": 0, \"isDir\": false}");
    }
}

// 2. The Hybrid Chunk Reader (Header\nRawData)
void handle_fs_read_chunk(const char *args, ipc_reply_t *reply) 
{
    char path[1024];
    if (!json_get_string(args, "path", path, sizeof(path))) 
    {
        ipc_reply_error(reply, "Missing 'path' argument");
        return;
    }

    uint64_t offset = json_get_uint64(args, "offset", 0);
    uint64_t length = json_get_uint64(args, "length", 65536);
    if (length == 0) length = 65536;
    if (length > 4 * 1024 * 1024) length = 4 * 1024 * 1024; // 4MB safe cap

    // 1. Get total file size
    vyom_stat_struct st;
    uint64_t total_size = 0;
    if (vyom_stat(path, &st) == 0) 
    {
        total_size = (uint64_t)st.st_size;
    }

    // 2. Open file
    FILE *fp = fopen(path, "rb");
    if (!fp) 
    {
        ipc_reply_error(reply, "Failed to open file");
        return;
    }

    // 3. 64-bit Seek
    if (vyom_fseek(fp, (off_t)offset, SEEK_SET) != 0) 
    {
        fclose(fp);
        ipc_reply_error(reply, "Failed to seek file");
        return;
    }

    // 4. Header buffer: "offset=...,len=...,total=...,eof=...\n"
    char header[128];
    // Allocate buffer for header + raw data payload
    char *payload = malloc(sizeof(header) + length + 1);
    if (!payload) 
    {
        fclose(fp);
        ipc_reply_error(reply, "Out of memory");
        return;
    }

    // 5. Read raw chunk directly into memory after header
    // Temporary read into buffer:
    size_t bytes_read = fread(payload + sizeof(header), 1, length, fp);
    bool eof = feof(fp) || ((offset + bytes_read) >= total_size);
    fclose(fp);

    // Format the first-line header
    int hlen = snprintf(header, sizeof(header),
                        "offset=%" PRIu64 ",len=%llu,total=%" PRIu64 ",eof=%d\n",
                        offset, (unsigned long long)bytes_read, total_size, eof ? 1 : 0);

    // Shift payload so raw data directly follows header
    memmove(payload + hlen, payload + sizeof(header), bytes_read);
    memcpy(payload, header, hlen);
    payload[hlen + bytes_read] = '\0';

    // Send Header + Raw Data (NO JSON ESCAPING ON DATA!)
    ipc_reply_send(reply, payload);
    free(payload);
}

// 3. Write / Append Text
void handle_fs_write_text(const char *args, ipc_reply_t *reply) 
{
    char path[1024];
    if (!json_get_string(args, "path", path, sizeof(path))) 
    {
        ipc_reply_error(reply, "Missing 'path' argument");
        return;
    }

    bool append = json_get_bool(args, "append", false);

    size_t content_cap = strlen(args) + 1;
    char *content = malloc(content_cap);
    if (!content) 
    {
        ipc_reply_error(reply, "Out of memory");
        return;
    }

    if (!json_get_string(args, "content", content, content_cap)) 
    {
        content[0] = '\0';
    }

    FILE *fp = fopen(path, append ? "ab" : "wb");
    if (!fp) 
    {
        free(content);
        ipc_reply_error(reply, "Failed to open file for writing");
        return;
    }

    size_t len = strlen(content);
    size_t written = fwrite(content, 1, len, fp);
    fclose(fp);
    free(content);

    char json[128];
    snprintf(json, sizeof(json), "{\"success\": true, \"bytesWritten\": %llu}", (unsigned long long)written);
    ipc_reply_send(reply, json);
}