#include "../../headers/telly.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

static const char months[][4] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

void to_uppercase(char *in, char *out) {
  while (*in != '\0') *(out++) = toupper(*(in++));
  *out = '\0';
}

void generate_random_string(char *dest, size_t length) {
  const char charset[] = (
    "0123456789"
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  );

  while (length-- > 0) {
    const uint8_t index = (double) rand() / RAND_MAX * (sizeof(charset) - 1);
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

  sprintf(text, "%s %s %d %s:%s:%s", date, mon, 1900 + tm->tm_year, hour, min, sec);
}
