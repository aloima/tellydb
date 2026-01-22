#pragma once

#include <stdint.h>

#include <gmp.h>

#include "../server/client.h"
#include "../utils/string.h"

#define RDT_SSTRING "+"
#define RDT_BSTRING "$"
#define RDT_ARRAY "*"
#define RDT_INTEGER ":"

// valkey-cli / probably redis-cli do not support formatting/sending big numbers
// valkey-cli throws "Unknown type: 13"
#define RDT_BIGNUMBER "("
#define RDT_DOUBLE ","
#define RDT_ERROR "-"

uint8_t create_resp_integer(char *buf, uint64_t value);
uint64_t create_resp_integer_mpf(const enum ProtocolVersion protover, char *buf, mpf_t value);
uint64_t create_resp_integer_mpz(const enum ProtocolVersion protover, char *buf, mpz_t value);
uint64_t create_resp_string(char *buf, string_t string);
