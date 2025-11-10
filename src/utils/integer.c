#include <telly.h>

#include <stdbool.h>
#include <stdint.h>

bool try_parse_integer(const char *value) {
  const char *_value = value;

  if (*_value == '-') {
    _value += 1;

    if (*_value == '0' || *_value == '\0') {
      return false;
    }
  }

  while ('0' <= *_value && *_value <= '9') {
    _value += 1;
  }

  return (_value != value) && (*_value == 0x00);
}

bool try_parse_double(const char *value) {
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

uint8_t ltoa(const int64_t value, char *dst) {
  const bool neg = (value < 0);
  uint64_t uval;

  if (value == INT64_MIN) {
    uval = 9223372036854775808ULL; // 2^63
  } else {
    uval = (neg ? -value : value);
  }

  const uint8_t len = get_digit_count(uval);
  const uint8_t total_len = (len + neg);
  dst[total_len] = '\0';

  uint8_t pos = total_len;

  while (uval >= 100) {
    const uint64_t remainder = (uval % 100);

    pos -= 2;
    *((uint16_t *) (dst + pos)) = *((uint16_t *) (TWO_DIGITS_TABLE + remainder * 2));

    uval /= 100;
  }

  if (uval > 0) {
    const uint8_t remainder = uval;
    const uint16_t chars = *((uint16_t*) (TWO_DIGITS_TABLE + remainder * 2));

    if (remainder >= 10) {
      pos -= 2;
      *((uint16_t *) (dst + pos)) = chars;
    } else {
      pos -= 1;
      dst[pos] = ('0' + remainder);
    }
  }

  if (neg) dst[0] = '-';
  return total_len;
}

uint8_t get_digit_count(const uint64_t value) {
  if (value == 0) {
    return 1;
  }

  const uint64_t bits = (64 - __builtin_clzll(value));
  const uint64_t approximate_digits = (bits * 308) >> 10;

  return (uint8_t) (approximate_digits + (value >= POW10_TABLE[approximate_digits]));
}

uint8_t get_bit_count(const uint64_t value) {
  if (value == 0) {
    return 0;
  }

  return (64 - __builtin_clzll(value));
}

uint8_t get_byte_count(const uint64_t value) {
  if (value == 0) {
    return 0;
  }

  return (64 - __builtin_clzll(value) + 7) / 8;
}
