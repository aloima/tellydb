#include "../headers/telly.h"

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  switch (argc) {
    case 1: {
      struct Configuration *config = get_configuration(NULL);
      start_server(config);
      return EXIT_SUCCESS;
    }

    case 2: {
      const char *arg = argv[1];

      if (streq(arg, "version")) {
        puts((
          "tellydb version " VERSION "\n"
          "open source at https://github.com/aloima/tellydb\n"
          "licensed under BSD-3 Clause Clear License"
        ));
        return EXIT_SUCCESS;
      } else if (streq(arg, "help")) {
        puts((
          "telly [<ARGUMENT>]\n\n"
          "Arguments:\n"
          " version        - Prints version\n"
          " help           - Prints this page\n"
          " config [FILE]  - Runs the server using configuration file. If file is not exist, use .tellyconf\n"
          " create-config  - Creates a .tellyconf file using default values. If it is exists, it will be discarded\n"
          " default-config - Prints default configuration values\n\n"
          "Without an argument, starts the server using .tellyconf file in working directory.\n"
          "If .tellyconf is not exists, starts the server using default values."
        ));

        return EXIT_SUCCESS;
      } else if (streq(arg, "default-config")) {
        struct Configuration conf = get_default_configuration();
        char buf[1024];

        get_configuration_string(buf, conf);
        puts(buf);

        return EXIT_SUCCESS;
      } else if (streq(arg, "create-config")) {
        FILE *file = fopen(".tellyconf", "w");
        struct Configuration conf = get_default_configuration();
        char buf[1024];

        const size_t n = get_configuration_string(buf, conf);
        fwrite(buf, sizeof(char), n, file);
        fclose(file);

        return EXIT_SUCCESS;
      } else {
        fputs("Invalid argument, use help command\n", stderr);
        return EXIT_FAILURE;
      }
    }

    case 3:
      if (streq(argv[1], "config")) {
        struct Configuration *config = get_configuration(argv[2]);
        start_server(config);
        return EXIT_SUCCESS;
      } else {
        fputs("Invalid argument usage, use help command\n", stderr);
        return EXIT_FAILURE;
      }

    default:
      fputs("Invalid argument count, use help command\n", stderr);
      return EXIT_FAILURE;
  }
}
