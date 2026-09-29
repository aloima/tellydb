#include <telly.h>

// Source: https://github.com/openssl/openssl/blob/master/crypto/lhash/lhash.c#L359 (OPENSSL_LH_strhash)
// Modified for to use string length, not null terminator
uint64_t string_hash(void *data) {
  string_t *key = (string_t *) data;

  char *c = key->value;
  uint64_t len = key->len;

  unsigned long ret = 0;
  long n;
  unsigned long v;
  int r;

  if ((c == NULL) || (len == '\0')) {
    return ret;
  }

  n = 0x100;
  while (len > 0) {
    v = n | (*c);
    n += 0x100;
    r = (int) ((v >> 2) ^ v) & 0x0f;
    /* cast to uint64_t to avoid 32 bit shift of 32 bit value */
    ret = (ret << r) | (unsigned long) ((uint64_t) ret >> (32 - r));
    ret &= 0xFFFFFFFFL;
    ret ^= v * v;

    c++;
    len--;
  }

  return (ret >> 16) ^ ret;
}

bool string_compare(void *string_a, void *string_b) {
  if (string_a == NULL || string_b == NULL) {
    return NULL;
  }

  string_t *a = (string_t *) string_a;
  string_t *b = (string_t *) string_b;

  return SSTREQ(*a, *b);
}

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
  if (value >= 100) return;

  const char *digits = &TWO_DIGITS_TABLE[value * 2];
  res[0] = digits[0];
  res[1] = digits[1];
}

void generate_date_string(char *text, const time_t value) {
  struct tm tm;
  ASSERT(localtime_r(&value, &tm), ==, &tm);

  const char *month = months[tm.tm_mon];

  // 01 Jan 1970 00:00:00
  number_pad(text, tm.tm_mday); // 01
  text[2] = ' ';

  // Jan
  text[3] = month[0];
  text[4] = month[1];
  text[5] = month[2];

  text[6] = ' ';
  ASSERT(ltoa(1900 + tm.tm_year, text + 7), ==, 4); // 1970
  text[11] = ' ';
  number_pad(text + 12, tm.tm_hour); // 00
  text[14] = ':';
  number_pad(text + 15, tm.tm_min); // 00
  text[17] = ':';
  number_pad(text + 18, tm.tm_sec); // 00
  text[20] = '\0';
}
