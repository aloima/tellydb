#include "../../headers/telly.h"
#include "../../headers/server.h"
#include "../../headers/utils.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

#define DATA_ERR(client) write_log(LOG_ERR, "Received data from Client #%d cannot be validated as a RESP data, so it cannot be created.", (client)->id);

respdata_t *parse_resp_array(struct Client *client, uint8_t type) {
  respdata_t *data = malloc(sizeof(respdata_t));
  data->type = type;
  data->count = 0;

  while (true) {
    char c;
    _read(client, &c, 1);

    if (isdigit(c)) {
      data->count = (data->count * 10) + (c - 48);
    } else if (c == '\r') {
      _read(client, &c, 1);

      if (c == '\n') {
        data->value.array = malloc(data->count * sizeof(respdata_t));

        for (uint32_t i = 0; i < data->count; ++i) {
          data->value.array[i] = get_resp_data(client);
        }

        return data;
      } else {
        DATA_ERR(client);
        free(data);
        return NULL;
      }
    } else {
      DATA_ERR(client);
      free(data);
      return NULL;
    }
  }
}

respdata_t *parse_resp_sstring(struct Client *client, uint8_t type) {
  respdata_t *data = malloc(sizeof(respdata_t));
  data->type = type;
  data->value.string = (string_t) {
    .value = malloc(33),
    .len = 0
  };

  string_t *string = &data->value.string;

  while (true) {
    string->len += _read(client, string->value + string->len, 1);

    if (string->len % 32 == 0) {
      string->value = realloc(string->value, string->len + 33);
    }

    if (string->value[string->len - 1] == '\r') {
      char c;
      _read(client, &c, 1);

      if (c == '\n') {
        string->len -= 1;
        string->value[string->len] = '\0';
        return data;
      } else {
        DATA_ERR(client);
        free_resp_data(data);
        return NULL;
      }
    } else {
      DATA_ERR(client);
      free_resp_data(data);
      return NULL;
    }
  }
}

respdata_t *parse_resp_bstring(struct Client *client, uint8_t type) {
  respdata_t *data = malloc(sizeof(respdata_t));
  data->type = type;
  data->value.string.len = 0;

  string_t *string = &data->value.string;

  while (true) {
    char c;
    _read(client, &c, 1);

    if (isdigit(c)) {
      string->len = (string->len * 10) + (c - 48);
    } else if (c == '\r') {
      _read(client, &c, 1);

      if (c == '\n') {
        string->value = malloc(string->len + 1);
        uint64_t total = string->len;

        while (total != 0) {
          total -= _read(client, string->value + string->len - total, total);
        }

        string->value[string->len] = '\0';

        char buf[2];
        _read(client, buf, 2);

        if (buf[0] != '\r' || buf[1] != '\n') {
          DATA_ERR(client);
          free_resp_data(data);
          return NULL;
        }

        return data;
      } else {
        DATA_ERR(client);
        free(data);
        return NULL;
      }
    } else {
      DATA_ERR(client);
      free(data);
      return NULL;
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
