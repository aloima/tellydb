#pragma once

#include <utils/utils.h>

#include <stdint.h>

// Will be used for method arguments, includes everything which will be used except individual data such as string
typedef struct GenericArguments {
  const int fd;
  char *block;
  const uint16_t block_size;
  uint16_t *at;
} GenericArguments;

typedef struct UnallocatedValue {
  void **data;
  const enum TellyTypes type;
  uint64_t *element_count;
} UnallocatedValue;

typedef struct CollectionResult {
  bool succeed;
  size_t value;
} CollectionResult;

#define CREATE_COLLECTION_RESULT(_succeed, _value) \
  ((CollectionResult) { .succeed = (_succeed), .value = (_value) })

#define UNSUCCESSFUL_COLLECTION() CREATE_COLLECTION_RESULT(false, 0)

void collect_bytes(const GenericArguments *arguments, const uint32_t count, void *data);
CollectionResult collect_string(const GenericArguments *arguments, string_t *string);
CollectionResult collect_integer(const GenericArguments *arguments, mpz_t *number);
CollectionResult collect_double(const GenericArguments *arguments, mpf_t *number);
