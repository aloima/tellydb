#pragma once

#include <database/database.h>
#include <utils/utils.h>

#include <unistd.h>

void generate_headers(char *headers, const uint32_t server_age);
uint32_t generate_string_value(char **data, off_t *len, const string_t *string);
off_t generate_value(char **data, KeyValue *kv);

void get_maximum_keyvalue_size(HashTableElement element, void *external);
