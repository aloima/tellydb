#pragma once

#include <string.h> // IWYU pragma: keep
#include <stdint.h>
#include <stddef.h>
#include <time.h>

#define streq(s1, s2) (strcmp((s1), (s2)) == 0)

typedef struct String {
  char *value;
  uint32_t len;
} string_t;

#define EMPTY_STRING() ((string_t) {"", 0})
#define CREATE_STRING(value, len) ((string_t) {value, len})

uint64_t hash(char *key, uint32_t length);
void to_uppercase(string_t src, char *dst);

static constexpr char months[12][4] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static constexpr char charset[] = (
  "0123456789"
  "abcdefghijklmnopqrstuvwxyz"
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
);

void generate_random_string(char *dest, size_t length);
void generate_date_string(char *text, const time_t value);
