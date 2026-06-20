#include <telly.h>
#include "read.h"

CollectionResult collect_string(const GenericArguments *arguments, string_t *string) {
  string->len = 0;

  uint8_t first;
  collect_bytes(arguments, 1, &first);

  const uint8_t byte_count = (first >> 6);
  collect_bytes(arguments, byte_count, &string->len);

  string->len = ((string->len << 6) | (first & 0b111111));
  string->value = malloc(string->len);
  if (string->value == NULL)
    return CREATE_COLLECTION_RESULT(false, 0);

  collect_bytes(arguments, string->len, string->value);

  return CREATE_COLLECTION_RESULT(true, (1 + byte_count + string->len));
}

CollectionResult collect_integer(const GenericArguments *arguments, mpz_t *number) {
  uint8_t specifier;
  collect_bytes(arguments, 1, &specifier);

  const bool negative = (specifier & 0x80);
  const uint8_t byte_count = ((specifier & 0x7F) + 1);

  uint8_t data[byte_count];
  collect_bytes(arguments, byte_count, data);

  mpz_t value;
  mpz_init(value);
  mpz_init_set_ui(*number, 0);

  const uint8_t end = (byte_count - 1);

  for (uint8_t i = 0; i < end; ++i) {
    mpz_set_ui(value, data[i]);
    mpz_ior(*number, *number, value);
    mpz_mul_2exp(*number, *number, 8);
  }

  mpz_set_ui(value, data[end]);
  mpz_ior(*number, *number, value);

  if (negative) {
    mpz_neg(*number, *number);
  }

  mpz_clear(value);
  return CREATE_COLLECTION_RESULT(true, (1 + byte_count));
}

CollectionResult collect_double(const GenericArguments *arguments, mpf_t *number) {
  uint8_t two[2];
  collect_bytes(arguments, 2, two);

  uint8_t specifier = two[0], indicator = two[1];
  const bool negative = (specifier & 0x80);
  const uint8_t byte_count = ((specifier & 0x7F) + 1);
  int16_t exp = ((indicator & 0x80) ? (indicator & 0x7F) : -1);

  uint8_t data[byte_count];
  collect_bytes(arguments, byte_count, data);

  const bool is_even = (data[0] > 0x10);

  const uint16_t hex_len = ((byte_count * 2) + (exp != -1));
  char hex[hex_len + 1];

  if (!is_even) {
    exp += 1;
  }

  if (exp != -1) {
    const bool exp_even = ((exp & 1) == 0); // (exp % 2) == 0

    if (exp_even) {
      for (uint8_t i = 0; i < byte_count; ++i) {
        if ((i * 2) == exp) {
          sprintf(hex + (i * 2), ".%02x", data[i]);
        } else {
          sprintf(hex + (i * 2) + (exp < (i * 2)), "%02x", data[i]);
        }
      }
    } else {
      for (uint8_t i = 0; i < byte_count; ++i) {
        const uint8_t mul = (i * 2);

        if ((mul + 1) == exp) {
          char value[3];
          sprintf(value, "%02x", data[i]);

          hex[mul] = value[0];
          hex[mul + 1] = '.';
          hex[mul + 2] = value[1];
        } else {
          sprintf(hex + (i * 2) + (exp < (mul + 1)), "%02x", data[i]);
        }
      }
    }
  } else {
    for (uint8_t i = 0; i < byte_count; ++i) {
      sprintf(hex + (i * 2), "%02x", data[i]);
    }
  }

  hex[hex_len] = '\0';

  mpf_init2(*number, FLOAT_PRECISION);
  mpf_set_str(*number, hex, 16);

  if (negative) {
    mpf_neg(*number, *number);
  }

  return CREATE_COLLECTION_RESULT(true, (2 + byte_count));
}
