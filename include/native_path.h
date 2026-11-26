#ifndef PATH_H
#define PATH_H

#include "platform.h"

void handle_path_get_special(const char *args, ipc_reply_t *reply);

void handle_path_get_all(const char *args, ipc_reply_t *reply);

#endif