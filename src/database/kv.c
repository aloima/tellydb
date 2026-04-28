#include <telly.h>

static HashSet *expiry_set;

int create_expiry_set() {
  expiry_set = create_hashset(128);
  return (expiry_set != NULL) ? 0 : -1;
}

void destroy_expiry_set() {
  destroy_hashset(expiry_set, NULL);
}

static inline uint64_t add_to_index(const uint64_t index, const uint64_t capacity) {
  return ((index + 1) % capacity);
}

int set_kv(struct KVPair *kv, const string_t key, void *value, const enum TellyTypes type, const uint64_t *expire_at_p) {
  kv->key.value = malloc(key.len);
  if (!kv->key.value) return -1;

  kv->hashed = OPENSSL_LH_strhash(key.value);
  kv->key.len = key.len;
  memcpy(kv->key.value, key.value, key.len);

  kv->type = type;
  kv->value = value;

  if (expire_at_p != NULL) {
    kv->expire.enabled = true;
    kv->expire.at = *expire_at_p;

    if (insert_into_hashset(expiry_set, kv) < 0)
      return -2;
  } else {
    kv->expire.enabled = false;
  }

  return 0;
}

bool delete_kv(Database *database, struct KVPair *kv) {
  const uint64_t capacity = database->size.capacity;
  const uint64_t start_idx = (kv->hashed % capacity);
  uint64_t index = start_idx;

  while (true) {
    if (kv == database->data[index]) {
      break;
    }

    index = add_to_index(index, capacity);

    if (index == start_idx) {
      return false;
    }
  }

  free_kv(kv);
  delete_from_hashset(expiry_set, database->data[index]);
  database->data[index] = NULL; // Needs it for uncollised indexes and filled next index

  for (uint64_t i = add_to_index(index, capacity); i != index; i = add_to_index(index, capacity)) {
    struct KVPair *pair = database->data[i];
    const uint64_t prev = ((i == 0) ? (capacity - 1) : (i - 1));

    if (!pair) {
      database->size.stored -= 1;
      database->data[prev] = NULL;
      break;
    }

    // On collised index
    if (index == (pair->hashed % capacity)) {
      database->data[prev] = pair;
    } else {
      break;
    }
  }

  return true;
}

int check_kv_expiry(Database *database, struct KVPair *kv) {
  if (!kv->expire.enabled) return 0;

  struct timespec ts;
  if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    return -2;

  const uint64_t now = (ts.tv_sec * 1e3) + (ts.tv_nsec * 1e6);

  if (kv->expire.at <= now) {
    if (!delete_kv(database, kv))
      return -1;

    return 1;
  }

  return 0;
}

void free_value(const enum TellyTypes type, void *value) {
  switch (type) {
    case TELLY_NULL:
      break;

    case TELLY_INT:
      mpz_clear(*((mpz_t *) value));
      free(value);
      break;

    case TELLY_DOUBLE:
      mpf_clear(*((mpf_t *) value));
      free(value);
      break;

    case TELLY_STR:
      free(((string_t *) value)->value);
      free(value);
      break;

    case TELLY_BOOL:
      free(value);
      break;

    case TELLY_HASHTABLE:
      free_hashtable(value);
      break;

    case TELLY_LIST:
      free_list(value);
      break;
  }
}

void free_kv(struct KVPair *kv) {
  free_value(kv->type, kv->value);
  free(kv->key.value);
  free(kv);
}
