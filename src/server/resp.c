#include "../telly.h"

#include <stdlib.h>
#include <ctype.h>

#include <unistd.h>

respdata_t parse_resp_array(int connfd, uint8_t type) {
  respdata_t data = {
    .type = type
  };

  char *len = malloc(33);
  uint32_t read_count = 0;

  while (true) {
    char c;
    read(connfd, &c, 1);

    if (isdigit(c)) {
      len[read_count] = c;
      read_count += 1;
    } else if (c == '\r') {
      read(connfd, &c, 1);

      if (c == '\n') {
        len[read_count] = '\0';
        data.count = atoi(len);
        free(len);

        data.value.array = malloc(data.count * sizeof(respdata_t));

        for (uint32_t i = 0; i < data.count; ++i) {
          data.value.array[i] = get_resp_data(connfd);
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

respdata_t parse_resp_sstring(int connfd, uint8_t type) {
  respdata_t data = {
    .type = type
  };

  string_t string = {
    .data = malloc(33 * sizeof(char)),
    .len = 0
  };

  while (true) {
    string.len += read(connfd, string.data + string.len, 1);

    if (string.len % 32 == 0) {
      string.data = realloc(string.data, string.len + 33);
    }

    if (string.data[string.len - 1] == '\r') {
      char c;
      read(connfd, &c, 1);

      if (c == '\n') {
        string.data[string.len - 1] = '\0';
        string.len -= 1;
        data.value.string = string;
        break;
      } else {
        client_error();
      }
    }
  }

  return data;
}

respdata_t parse_resp_bstring(int connfd, uint8_t type) {
  respdata_t data = {
    .type = type
  };

  char *len = malloc(33);
  uint32_t read_count = 0;

  while (true) {
    read_count += read(connfd, len + read_count, 1);

    if (read_count % 32 == 0) {
      len = realloc(len, read_count + 33);
    }

    if (len[read_count - 1] == '\r') {
      char c;
      read(connfd, &c, 1);

      if (c == '\n') {
        len[read_count - 1] = '\0';
        uint32_t lend = atoi(len);
        free(len);

        data.value.string = (string_t) {
          .data = malloc(lend + 1),
          .len = lend
        };

        read(connfd, data.value.string.data, lend);
        data.value.string.data[lend + 1] = '\0';

        char buf[2];
        read(connfd, buf, 2);

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

respdata_t get_resp_data(int connfd) {
  respdata_t data;
  uint8_t type;

  while (true) {
    read(connfd, &type, 1);

    switch (type) {
      case RDT_ARRAY:
        data = parse_resp_array(connfd, type);
        return data;

      case RDT_SSTRING:
        data = parse_resp_sstring(connfd, type);
        return data;

      case RDT_BSTRING:
        data = parse_resp_bstring(connfd, type);
        return data;

      case RDT_ERR:
        data = parse_resp_sstring(connfd, type);
        return data;
    }

    usleep(3000);
  }
}
