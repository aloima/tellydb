#include <telly.h>

bool try_parse_integer(const string_t str) {
  char *v = str.value;
  uint64_t len = str.len;

  if (*v == '-') {
    v += 1;
    len -= 1;

    if (len == 0 || *v == '0')
      return false;
  }

  while ('0' <= *v && *v <= '9') {
    v += 1;
    len -= 1;
  }

  return (v != str.value) && (len == 0);
}

bool try_parse_double(const string_t str) {
  bool point = false;
  char *v = str.value;
  uint64_t len = str.len;

  if (*v == '-') {
    v += 1;
    len -= 1;
  }

  while (len != 0) {
    const char c = *v;

    if (c == '.') {
      if (point)
        return false;

      point = true;
    } else if (c < '0' || '9' < c) {
      return false;
    }

    v += 1;
    len -= 1;
  }

  return point;
}

uint64_t atoull_s(const string_t str) {
  ASSERT(str.value, !=, NULL);
  ASSERT(str.len, !=, 0);

  errno = 0;

  const char *value = str.value;
  uint64_t len = str.len;

  if (value[0] == '-') {
    errno = EINVAL;
    return UINT64_MAX;
  }

  if (value[0] == '+') {
    value += 1;
    len -= 1;

    if (VERY_UNLIKELY(len == 0)) {
      errno = EINVAL;
      return UINT64_MAX;
    }
  }

  while (len != 0 && *value == '0') {
    value += 1;
    len -= 1;
  }

  static constexpr const uint64_t max_div_10 = UINT64_MAX / 10;
  static constexpr const uint64_t max_mod_10 = UINT64_MAX % 10;

  uint64_t result = 0;

  while (len != 0) {
    const char c = *value;

    if (VERY_UNLIKELY(!('0' <= c && c <= '9'))) {
      errno = EINVAL;
      return UINT64_MAX;
    }

    const uint8_t digit = (c - '0');

    if (VERY_UNLIKELY(result > max_div_10 || (result == max_div_10 && digit > max_mod_10))) {
      errno = ERANGE;
      return UINT64_MAX;
    }

    result = (result * 10) + digit;
    len -= 1;
    value += 1;
  }

  return result;
}

uint8_t ltoa(const int64_t value, char *dst) {
  const bool neg = (value < 0);
  uint64_t uval;

  if (value == INT64_MIN)
    uval = 9223372036854775808ULL; // 2^63
  else
    uval = (neg ? -value : value);

  const uint8_t len = get_digit_count(uval);
  const uint8_t total_len = (len + neg);
  dst[total_len] = '\0';

  uint8_t pos = total_len;

  while (uval >= 100) {
    const uint64_t remainder = (uval % 100);

    pos -= 2;
    memcpy(dst + pos, TWO_DIGITS_TABLE + remainder * 2, 2);

    uval /= 100;
  }

  if (uval > 0) {
    const uint8_t remainder = uval;

    if (remainder >= 10) {
      pos -= 2;
      memcpy(dst + pos, TWO_DIGITS_TABLE + remainder * 2, 2);
    } else {
      pos -= 1;
      dst[pos] = ('0' + remainder);
    }
  }

  if (neg) dst[0] = '-';
  return total_len;
}

uint8_t get_digit_count(const uint64_t value) {
  if (value == 0) return 1;

  const uint64_t bits = (64 - __builtin_clzll(value));
  const uint64_t approximate_digits = (bits * 308) >> 10;

  return (uint8_t) (approximate_digits + (value >= POW10_TABLE[approximate_digits]));
}

uint8_t get_bit_count(const uint64_t value) {
  if (value == 0) return 0;
  return (64 - __builtin_clzll(value));
}

uint8_t get_byte_count(const uint64_t value) {
  if (value == 0) return 0;
  return (64 - __builtin_clzll(value) + 7) / 8;
}
