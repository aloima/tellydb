#include <telly.h>

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

static constexpr char months[12][4] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static constexpr char charset[] = (
  "0123456789"
  "abcdefghijklmnopqrstuvwxyz"
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
);

void to_uppercase(string_t src, char *dst) {
  for (uint32_t i = 0; i < src.len; ++i) {
    const char c = src.value[i];
    const char mask = ((c >= 'a') && (c <= 'z')) * 32;
    dst[i] = c ^ mask; // To uppercase, we need to use (c - 32). It is more performant way.
  }
}

void generate_random_string(char *dest, size_t length) {
  while (length-- > 0) {
    const uint8_t index = (double) rand() / RAND_MAX * (sizeof(charset) - 1);
    *dest++ = charset[index];
  }

  *dest = '\0';
}

static inline void number_pad(char *res, const uint32_t value) {
  if (value < 10) {
    res[0] = '0';
    res[1] = (value + 48);
  } else if (value < 100) {
    res[0] = ((value / 10) + 48);
    res[1] = ((value % 10) + 48);
  }
}

void generate_date_string(char *text, const time_t value) {
  struct tm tm;
  struct tm *_ = localtime_r(&value, &tm);

  const char *month = months[tm.tm_mon];

  // 01 Jan 1970 00:00:00
  number_pad(text, tm.tm_mday); // 01
  text[2] = ' ';

  // Jan
  text[3] = month[0];
  text[4] = month[1];
  text[5] = month[2];

  text[6] = ' ';
  ltoa(1900 + tm.tm_year, text + 7); // 1970
  text[11] = ' ';
  number_pad(text + 12, tm.tm_hour); // 00
  text[14] = ':';
  number_pad(text + 15, tm.tm_min); // 00
  text[17] = ':';
  number_pad(text + 18, tm.tm_sec); // 00
  text[20] = '\0';
}
