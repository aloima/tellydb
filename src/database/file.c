#include "../../headers/database.h"
#include "../../headers/btree.h"
#include "../../headers/hashtable.h"
#include "../../headers/utils.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

static int fd = -1;
static bool saving = false;

bool open_database_fd(const char *filename, uint64_t *server_age) {
  fd = open(filename, (O_RDWR | O_CREAT), (S_IRUSR | S_IWUSR));

  if (fd == -1) {
    write_log(LOG_ERR, "Database file cannot be opened or created.");
    return false;
  }

  if (lseek(fd, 0, SEEK_END) != 0) {
    lseek(fd, 2, SEEK_SET);
    read(fd, server_age, sizeof(long));
  } else {
    *server_age = 0;
  }

  return true;
}

int get_database_fd() {
  return fd;
}

void close_database_fd() {
  while (saving) usleep(100);
  close(fd);
}

static off_t generate_value(char **line, struct KVPair *kv) {
  off_t len;

  switch (kv->type) {
    case TELLY_NULL:
      len = 1;
      *line = malloc(len);

      (*line)[0] = TELLY_NULL;
      break;

    case TELLY_NUM: {
      const long *number = kv->value;
      const uint32_t bit_count = log2(*number) + 1;
      const uint32_t byte_count = (bit_count / 8) + 1;

      len = byte_count + 2;
      *line = malloc(len);

      (*line)[0] = TELLY_NUM;
      (*line)[1] = byte_count;
      memcpy(*line + 2, number, byte_count);
      break;
    }

    case TELLY_STR: {
      const string_t *string = kv->value;

      const uint8_t bit_count = log2(string->len) + 1;
      const uint8_t byte_count = ceil((float) (bit_count - 6) / 8);
      const uint8_t first = (byte_count << 6) | (string->len & 0b111111);
      const uint32_t length_in_bytes = string->len >> 6;

      len = string->len + byte_count + 2;
      *line = malloc(len);

      (*line)[0] = TELLY_STR;
      (*line)[1] = first;
      memcpy(*line + 2, &length_in_bytes, byte_count);

      memcpy(*line + byte_count + 2, string->value, string->len);
      break;
    }

    case TELLY_BOOL:
      len = 2;
      *line = malloc(len);

      (*line)[0] = TELLY_BOOL;
      (*line)[1] = *((bool *) kv->value);
      break;

    case TELLY_HASHTABLE: {
      struct HashTable *table = kv->value;

      len = 5;
      *line = malloc(len);

      (*line)[0] = TELLY_HASHTABLE;
      memcpy(*line + 1, &table->size.allocated, sizeof(uint32_t));

      off_t at = len;

      for (uint32_t i = 0; i < table->size.allocated; ++i) {
        struct FVPair *fv = table->fvs[i];

        while (fv) {
          const string_t name = fv->name;
          const uint8_t n_bit_count = log2(name.len) + 1;
          const uint8_t n_byte_count = ceil((float) (n_bit_count - 6) / 8);
          const uint8_t n_first = (n_byte_count << 6) | (name.len & 0b111111);
          const uint32_t n_length_in_bytes = name.len >> 6;

          switch (fv->type) {
            case TELLY_NULL:
              len += 2 + n_byte_count + name.len;
              *line = realloc(*line, len);

              (*line)[at] = TELLY_NULL;
              (*line)[at += 1] = n_first;
              memcpy(*line + (at += 1), &n_length_in_bytes, n_byte_count);
              memcpy(*line + (at += n_byte_count), name.value, name.len);
              at += name.len;
              break;

            case TELLY_NUM: {
              const long *number = fv->value;
              const uint32_t bit_count = log2(*number) + 1;
              const uint32_t byte_count = (bit_count / 8) + 1;

              len += 3 + n_byte_count + name.len + byte_count;
              *line = realloc(*line, len);

              (*line)[at] = TELLY_NUM;
              (*line)[at += 1] = n_first;
              memcpy(*line + (at += 1), &n_length_in_bytes, n_byte_count);
              memcpy(*line + (at += n_byte_count), name.value, name.len);

              (*line)[at += name.len] = byte_count;
              memcpy(*line + (at += 1), number, byte_count);
              at += byte_count;
              break;
            }

            case TELLY_STR: {
              string_t *string = fv->value;

              const uint8_t bit_count = log2(string->len) + 1;
              const uint8_t byte_count = ceil((float) (bit_count - 6) / 8);
              const uint8_t first = (byte_count << 6) | (string->len & 0b111111);
              const uint32_t length_in_bytes = string->len >> 6;

              len += 3 + n_byte_count + name.len + byte_count + string->len;
              *line = realloc(*line, len);

              (*line)[at] = TELLY_STR;
              (*line)[at += 1] = n_first;
              memcpy(*line + (at += 1), &n_length_in_bytes, n_byte_count);
              memcpy(*line + (at += n_byte_count), name.value, name.len);

              (*line)[at += name.len] = first;
              memcpy(*line + (at += 1), &length_in_bytes, byte_count);
              memcpy(*line + (at += byte_count), string->value, string->len);
              at += string->len;
              break;
            }

            case TELLY_BOOL:
              len += 3 + n_byte_count + name.len;
              *line = realloc(*line, len);

              (*line)[at] = TELLY_BOOL;
              (*line)[at += 1] = n_first;
              memcpy(*line + (at += 1), &n_length_in_bytes, n_byte_count);
              memcpy(*line + (at += n_byte_count), name.value, name.len);

              (*line)[at += name.len] = *((bool *) fv->value);
              at += 1;
              break;

            default:
              break;
          }

          fv = fv->next;
        }
      }

      len += 1;
      *line = realloc(*line, len);
      (*line)[at] = 0x17;
      break;
    }

    case TELLY_LIST: {
      struct List *list = kv->value;
      struct ListNode *node = list->begin;

      len = 5;
      *line = malloc(len);

      (*line)[0] = TELLY_LIST;
      memcpy(*line + 1, &list->size, sizeof(uint32_t));

      while (node) {
        switch (node->type) {
          case TELLY_NULL:
            len += 1;
            *line = realloc(*line, len + 1);

            (*line)[len - 1] = TELLY_NULL;
            break;

          case TELLY_NUM: {
            const long *number = node->value;
            const uint32_t bit_count = log2(*number) + 1;
            const uint32_t byte_count = (bit_count / 8) + 1;

            len += byte_count + 2;
            *line = realloc(*line, len + 1);

            (*line)[len - byte_count - 2] = TELLY_NUM;
            (*line)[len - byte_count - 1] = byte_count;
            memcpy(*line + len - byte_count, number, byte_count);
            break;
          }

          case TELLY_STR: {
            string_t *string = node->value;

            const uint8_t bit_count = log2(string->len) + 1;
            const uint8_t byte_count = ceil((float) (bit_count - 6) / 8);
            const uint8_t first = (byte_count << 6) | (string->len & 0b111111);
            const uint32_t length_in_bytes = string->len >> 6;

            len += string->len + byte_count + 2;
            *line = realloc(*line, len + 1);

            (*line)[len - string->len - byte_count - 2] = TELLY_STR;
            (*line)[len - string->len - byte_count - 1] = first;
            memcpy(*line + len - string->len - byte_count, &length_in_bytes, byte_count);
            memcpy(*line + len - string->len, string->value, string->len);
            break;
          }

          case TELLY_BOOL:
            len += 2;
            *line = realloc(*line, len + 1);

            (*line)[len - 2] = TELLY_BOOL;
            (*line)[len - 1] = *((bool *) node->value);
            break;

          default:
            break;
        }

        node = node->next;
      }

      break;
    }

    default:
      len = 0;
  }

  return len;
}

static off_t generate_authorization_part(char **part) {
  struct Password **passwords = get_passwords();
  const uint32_t password_count = get_password_count();
  const uint8_t password_count_byte_count = (password_count != 0) ? (log2(password_count) + 1) : 0;
  off_t part_length = 1 + password_count_byte_count;
  *part = malloc(part_length);

  (*part)[0] = password_count_byte_count;
  memcpy(*part + 1, &password_count, password_count_byte_count);

  char *buf = malloc(1);

  for (uint32_t i = 0; i < password_count; ++i) {
    struct Password *password = passwords[i];
    string_t data = password->data;

    const uint8_t bit_count = log2(data.len) + 1;
    const uint8_t byte_count = ceil((float) (bit_count - 6) / 8);
    const uint8_t first = (byte_count << 6) | (data.len & 0b111111);
    const uint32_t length_in_bytes = data.len >> 6;

    const uint32_t buf_len = 4 + byte_count + data.len;
    buf = realloc(buf, buf_len);
    buf[0] = first;
    memcpy(buf + 1, &length_in_bytes, byte_count);
    memcpy(buf + 1 + byte_count, data.value, data.len);
    memcpy(buf + 1 + byte_count + data.len, password->salt, 2);
    buf[3 + byte_count + data.len] = password->permissions;

    *part = realloc(*part, part_length + buf_len);
    memcpy(*part + part_length, buf, buf_len);
    part_length += buf_len;
  }

  free(buf);
  return part_length;
}

void save_data(const uint64_t server_age) {
  if (saving) return;
  saving = true;

  uint32_t size;
  struct BTreeValue **values = get_sorted_kvs_by_pos_as_values(&size);

  uint32_t file_size = lseek(fd, 0, SEEK_END);
  int32_t diff = 0;

  if (file_size != 0) {
    // File headers
    {
      uint8_t constants[2];
      lseek(fd, 0, SEEK_SET);

      if (read(fd, constants, 2) != 2 || constants[0] != 0x18 || constants[1] != 0x10 || file_size < 11) {
        write_log(LOG_ERR, "Cannot save data, invalid file headers");
        free(values);
        return;
      }

      write(fd, &server_age, sizeof(uint64_t));
    }

    // Authorization part
    {
      char *part;
      const off_t length = generate_authorization_part(&part);
      off_t *end_at = get_authorization_end_at();

      char *buf = malloc(file_size - *end_at);
      lseek(fd, *end_at, SEEK_SET);
      read(fd, buf, file_size - *end_at);

      lseek(fd, 10, SEEK_SET);
      write(fd, part, length);
      write(fd, buf, file_size - *end_at);

      file_size += length - (*end_at - 10);
      *end_at += (length + 10) - *end_at;
      free(part);
      free(buf);
    }
  } else {
    // File headers
    {
      uint8_t data[10] = {0x18, 0x10};
      memcpy(data + 2, &server_age, 8);

      lseek(fd, 0, SEEK_SET);
      write(fd, data, 10);
    }

    // Authorization part
    {
      char *part;
      const off_t length = generate_authorization_part(&part);
      off_t *end_at = get_authorization_end_at();

      write(fd, part, length);
      *end_at = (file_size = 10 + length);
      free(part);
    }
  }

  for (uint32_t i = 0; i < size; ++i) {
    struct KVPair *kv = values[i]->data;

    char *line;
    const off_t line_len = generate_value(&line, kv);

    if (line_len != 0) {
      if (kv->pos.start_at != -1) {
        const off_t line_len_in_file = kv->pos.end_at - (kv->pos.start_at - 1);

        if (line_len_in_file != line_len) {
          const off_t buf_len = file_size - kv->pos.end_at;
          const off_t total_len = line_len + buf_len;
          line = realloc(line, total_len);

          lseek(fd, kv->pos.end_at, SEEK_SET);
          read(fd, line + line_len, buf_len);
          lseek(fd, kv->pos.start_at + diff - 1, SEEK_SET);
          write(fd, line, total_len);

          diff += line_len - line_len_in_file;
          kv->pos.end_at += diff;
        } else {
          lseek(fd, kv->pos.start_at + diff - 1, SEEK_SET);
          write(fd, line, line_len);
        }
      } else {
        kv->pos.start_at = lseek(fd, 0, SEEK_END);

        const string_t key = kv->key;

        const uint8_t bit_count = log2(key.len) + 1;
        const uint8_t byte_count = ceil((float) (bit_count - 6) / 8);
        const uint8_t first = (byte_count << 6) | (key.len & 0b111111);
        const uint32_t length_in_bytes = key.len >> 6;

        const uint32_t length_specifier_len = byte_count + 1;
        const uint32_t key_part_len = key.len + length_specifier_len;

        kv->pos.end_at = kv->pos.start_at + key_part_len + line_len;
        kv->pos.start_at += key_part_len + 1;

        const uint32_t buf_len = key_part_len + line_len;
        char buf[buf_len];

        buf[0] = first;
        memcpy(buf + 1, &length_in_bytes, byte_count);
        memcpy(buf + length_specifier_len, key.value, key.len);
        memcpy(buf + length_specifier_len + key.len, line, line_len);

        write(fd, buf, buf_len);
        file_size += buf_len;
      }

      free(line);
    }
  }

  ftruncate(fd, file_size + diff);
  free(values);

  saving = false;
}

void *save_thread(void *arg) {
  const uint64_t *server_age = arg;
  save_data(*server_age);

  pthread_exit(NULL);
}

bool bg_save(uint64_t server_age) {
  if (saving) return false;

  pthread_t thread;
  pthread_create(&thread, NULL, save_thread, &server_age);
  pthread_detach(thread);

  return true;
}
