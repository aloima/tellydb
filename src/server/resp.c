#include "../../headers/server.h"
#include "../../headers/utils.h"

#include <openssl/ssl.h>

#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

respdata_t *parse_resp_array(struct Client *client, uint8_t type) {
  respdata_t *data = malloc(sizeof(respdata_t));
  data->type = type;

  char *len = malloc(33);
  uint32_t read_count = 0;

  while (true) {
    char c;
    _read(client, &c, 1);

    if (isdigit(c)) {
      len[read_count] = c;
      read_count += 1;
    } else if (c == '\r') {
      _read(client, &c, 1);

      if (c == '\n') {
        len[read_count] = '\0';
        data->count = atoi(len);
        free(len);

        data->value.array = malloc(data->count * sizeof(respdata_t));

        for (uint32_t i = 0; i < data->count; ++i) {
          data->value.array[i] = get_resp_data(client);
        }

        break;
      } else {
        client_error();
      }
    } else {
      client_error();
    }

    if (read_count % 32 == 0) {
      len = realloc(len, read_count + 33);
    }
  }

  return data;
}

respdata_t *parse_resp_sstring(struct Client *client, uint8_t type) {
  respdata_t *data = malloc(sizeof(respdata_t));
  data->type = type;
  data->count = 0;

  string_t string = {
    .value = malloc(33),
    .len = 0
  };

  while (true) {
    string.len += _read(client, string.value + string.len, 1);

    if (string.len % 32 == 0) {
      string.value = realloc(string.value, string.len + 33);
    }

    if (string.value[string.len - 1] == '\r') {
      char c;
      _read(client, &c, 1);

      if (c == '\n') {
        string.value[string.len - 1] = '\0';
        string.len -= 1;
        data->value.string = string;
        break;
      } else {
        client_error();
      }
    }
  }

  return data;
}

respdata_t *parse_resp_bstring(struct Client *client, uint8_t type) {
  respdata_t *data = malloc(sizeof(respdata_t));
  data->type = type;
  data->count = 0;

  char *len = malloc(33);
  uint32_t read_count = 0;

  while (true) {
    read_count += _read(client, len + read_count, 1);

    if (read_count % 32 == 0) {
      len = realloc(len, read_count + 33);
    }

    if (len[read_count - 1] == '\r') {
      char c;
      _read(client, &c, 1);

      if (c == '\n') {
        len[read_count - 1] = '\0';
        uint32_t lend = atoi(len);
        free(len);

        data->value.string = (string_t) {
          .value = malloc(lend + 1),
          .len = lend
        };

        uint64_t total = lend;

        while (total != 0) {
          total -= _read(client, data->value.string.value + lend - total, total);
        }

        data->value.string.value[lend] = '\0';

        char buf[2];
        _read(client, buf, 2);

        if (buf[0] != '\r' || buf[1] != '\n') {
          client_error();
        }

        break;
      } else {
        client_error();
      }
    }
  }

  return data;
}

respdata_t *get_resp_data(struct Client *client) {
  respdata_t *data;
  uint8_t type;

  if (_read(client, &type, 1) == 0) {
    data = malloc(sizeof(respdata_t));
    data->type = RDT_CLOSE;

    return data;
  } else {
    switch (type) {
      case RDT_ARRAY:
        data = parse_resp_array(client, type);
        return data;

      case RDT_SSTRING:
        data = parse_resp_sstring(client, type);
        return data;

      case RDT_BSTRING:
        data = parse_resp_bstring(client, type);
        return data;

      case RDT_ERR:
        data = parse_resp_sstring(client, type);
        return data;

      default:
        return NULL;
    }
  }
}

void free_resp_data(respdata_t *data) {
  switch (data->type) {
    case RDT_ARRAY:
      for (uint32_t i = 0; i < data->count; ++i) {
        free_resp_data(data->value.array[i]);
      }

      free(data->value.array);
      break;

    case RDT_BSTRING:
      free(data->value.string.value);
      break;

    case RDT_SSTRING:
      free(data->value.string.value);
      break;

    case RDT_ERR:
      free(data->value.string.value);
      break;

    default:
      break;
  }

  free(data);
}
