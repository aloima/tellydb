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

bool is_integer(const char *value) {
  char *_value = (char *) value;

  if (*value == '-') {
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

void number_pad(char *res, const uint32_t value) {
  if (value < 10) {
    res[0] = '0';
    res[1] = (value + 48);
  } else if (value < 100) {
    res[0] = ((value / 10) + 48);
    res[1] = ((value % 10) + 48);
  }

  res[2] = '\0';
}

const int ltoa(const int64_t value, char *dst) {
  const bool neg = (value < 0);
  uint64_t uval = (neg ? -value : value);

  int len = 1;
  int l = 1, r = 19;

  while (l <= r) {
    int m = l + (r - l) / 2;

    if (uval >= pow10_table[m]) {
      len = m + 1;
      l = m + 1;
    } else {
      r = m - 1;
    }
  }

  const int total_len = (len + neg);
  dst[total_len] = '\0';

  int pos = (total_len - 1);

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

const int get_digit_count(const uint64_t value) {
  if (value >= pow10_table[19]) return 20;
  if (value >= pow10_table[18]) return 19;
  if (value >= pow10_table[17]) return 18;
  if (value >= pow10_table[16]) return 17;
  if (value >= pow10_table[15]) return 16;
  if (value >= pow10_table[14]) return 15;
  if (value >= pow10_table[13]) return 14;
  if (value >= pow10_table[12]) return 13;
  if (value >= pow10_table[11]) return 12;
  if (value >= pow10_table[10]) return 11;
  if (value >= pow10_table[9])  return 10;
  if (value >= pow10_table[8])  return  9;
  if (value >= pow10_table[7])  return  8;
  if (value >= pow10_table[6])  return  7;
  if (value >= pow10_table[5])  return  6;
  if (value >= pow10_table[4])  return  5;
  if (value >= pow10_table[3])  return  4;
  if (value >= pow10_table[2])  return  3;
  if (value >= pow10_table[1])  return  2;
  return 1;
}
