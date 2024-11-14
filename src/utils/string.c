#include "../../headers/utils.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#include <unistd.h>

static const char months[][4] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

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

void generate_date_string(char *text, const time_t value) {
  const struct tm *tm = localtime(&value);

  char date[3], mon[4], hour[3], min[3], sec[3];
  number_pad(date, tm->tm_mday);
  sprintf(mon, "%.3s", months[tm->tm_mon]);
  number_pad(hour, tm->tm_hour);
  number_pad(min, tm->tm_min);
  number_pad(sec, tm->tm_sec);

  // tm->tm_year is 2-digit integer, not 4-digit
  sprintf(text, "%s %s %d %s:%s:%s", date, mon, 1900 + tm->tm_year, hour, min, sec);
}
