#include <telly.h>

#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>

static const uint64_t pow10_table[] = {
  1UL,
  10UL,
  100UL,
  1000UL,
  10000UL,
  100000UL,
  1000000UL,
  10000000UL,
  100000000UL,
  1000000000UL,
  10000000000UL,
  100000000000UL,
  1000000000000UL,
  10000000000000UL,
  100000000000000UL,
  1000000000000000UL,
  10000000000000000UL,
  100000000000000000UL,
  1000000000000000000UL,
  10000000000000000000UL
};

const bool try_parse_integer(const char *value) {
  const char *_value = value;

  if (*_value == '-') {
    _value += 1;

    if (*_value == '0' || *_value == '\0') {
      return false;
    }
  }

  while (isdigit(*_value)) {
    _value += 1;
  }

  return (_value != value) && (*_value == 0x00);
}

const bool try_parse_double(const char *value) {
  uint64_t i = 0;

  if (value[i] == '-') {
    i += 1;
  }

  bool point = false;

  while (value[i] != '\0') {
    const char c = value[i];

    if (c == '.') {
      if (point) {
        return false;
      }

      point = true;
    } else if (c < '0' || '9' < c) {
      return false;
    }

    i += 1;
  }

  return point;
}

const uint8_t ltoa(const int64_t value, char *dst) {
  const bool neg = (value < 0);
  uint64_t uval = (neg ? -value : value);

  uint8_t len = 1;
  uint8_t l = 1, r = 19;

  while (l <= r) {
    uint8_t m = l + (r - l) / 2;

    if (uval >= pow10_table[m]) {
      len = m + 1;
      l = m + 1;
    } else {
      r = m - 1;
    }
  }

  const uint8_t total_len = (len + neg);
  dst[total_len] = '\0';

  uint8_t pos = (total_len - 1);

  while (uval >= 10) {
    uint64_t q = (uval / 10);
    dst[pos--] = ('0' + (uval - q * 10));
    uval = q;
  }

  dst[pos] = ('0' + uval);

  if (neg) {
    dst[0] = '-';
  }

  return total_len;
}

const uint8_t get_digit_count(const uint64_t value) {
  if (value < pow10_table[10]) {
    if (value < pow10_table[5]) {
      if (value < pow10_table[1]) return 1;
      if (value < pow10_table[2]) return 2;
      if (value < pow10_table[3]) return 3;
      if (value < pow10_table[4]) return 4;
      return 5;
    } else {
      if (value < pow10_table[6]) return 6;
      if (value < pow10_table[7]) return 7;
      if (value < pow10_table[8]) return 8;
      if (value < pow10_table[9]) return 9;
      return 10;
    }
  } else {
    if (value < pow10_table[15]) {
      if (value < pow10_table[11]) return 11;
      if (value < pow10_table[12]) return 12;
      if (value < pow10_table[13]) return 13;
      if (value < pow10_table[14]) return 14;
      return 15;
    } else {
      if (value < pow10_table[16]) return 16;
      if (value < pow10_table[17]) return 17;
      if (value < pow10_table[18]) return 18;
      return 19;
    }
  }
}

const uint8_t get_bit_count(const uint64_t value) {
  if (value == 0) {
    return 0;
  }

  return (64 - __builtin_clzll(value));
}

const uint8_t get_byte_count(const uint64_t value) {
  if (value == 0) {
    return 0;
  }

  return (64 - __builtin_clzll(value) + 7) / 8;
}
