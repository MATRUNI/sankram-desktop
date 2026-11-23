#ifndef JSON_UTIL_H
#define JSON_UTIL_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

bool json_get_string(const char *json, const char *key, char *out, size_t max_len);

uint64_t json_get_uint64(const char *json, const char *key, uint64_t default_val);

bool json_get_bool(const char *json, const char *key, bool default_val);

#endif