#include "../../headers/utils.h"

#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

#include <unistd.h>

void to_uppercase(char *in, char *out) {
  while (*in != '\0') *(out++) = toupper(*(in++));
  *out = '\0';
}

void read_string_from_file(const int fd, string_t *string, const bool unallocated, const bool terminator) {
  uint8_t first;
  read(fd, &first, 1);

  const uint8_t byte_count = first >> 6;
  string->len = 0;
  read(fd, &string->len, byte_count);
  string->len = (string->len << 6) | (first & 0b111111);

  const uint32_t size = terminator ? (string->len + 1) : string->len;

  if (unallocated) string->value = malloc(size);
  else string->value = realloc(string->value, size);

  read(fd, string->value, string->len);
  if (terminator) string->value[string->len] = '\0';
}

void generate_random_string(char *dest, size_t length) {
  const char charset[] = "0123456789"
                         "abcdefghijklmnopqrstuvwxyz"
                         "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

  while (length-- > 0) {
    const size_t index = (double) rand() / RAND_MAX * (sizeof(charset) - 1);
    *dest++ = charset[index];
  }

  *dest = '\0';
}
