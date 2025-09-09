#include <telly.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>

static const char months[12][4] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

void to_uppercase(char *in, char *out) {
  while (*in != '\0') {
    const char c = *in;
    *(out++) = (c <= 'Z') ? c : (c + 48);
    in += 1;
  }

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

  // 01 Jan 1970 00:00:00
  number_pad(text, tm->tm_mday); // 01
  text[2] = ' ';
  sprintf(text + 3, "%.3s", months[tm->tm_mon]); // Jan
  text[6] = ' ';
  ltoa(1900 + tm->tm_year, text + 7); // 1970
  text[11] = ' ';
  number_pad(text + 12, tm->tm_hour); // 00
  text[14] = ':';
  number_pad(text + 15, tm->tm_min); // 00
  text[17] = ':';
  number_pad(text + 18, tm->tm_sec); // 00
  // latest number_pad writes '\0' to latest byte
}
