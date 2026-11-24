#ifndef FS_H
#define FS_H
#include "platform.h"

void handle_fs_file_info(const char *args, ipc_reply_t *reply);

void handle_fs_read_chunk(const char *args, ipc_reply_t *reply);

void handle_fs_write_text(const char *args, ipc_reply_t *reply);

void handle_fs_read_dir(const char *args, ipc_reply_t *reply);

#endif