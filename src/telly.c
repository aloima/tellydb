#include <stdio.h>
#include <stdlib.h>

#include "../headers/telly.h"

int main(int argc, char *argv[]) {
  switch (argc) {
    case 1: {
      struct Configuration *conf = get_configuration(NULL);
      start_server(conf);
      return EXIT_SUCCESS;
    }

    case 2: {
      const char *arg = argv[1];

      if (streq(arg, "version")) {
        puts("tellydb version 0.1.0");
        return EXIT_SUCCESS;
      } else if (streq(arg, "help")) {
        puts((
          "telly [<ARGUMENT>]\n\n"
          "Arguments:\n"
          " version        - Prints version\n"
          " help           - Prints this page\n"
          " config [FILE]  - Runs the server using configuration file\n"
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

        get_configuration_string(buf, conf);
        fwrite(buf, sizeof(char), strlen(buf), file);
        fclose(file);

        return EXIT_SUCCESS;
      } else {
        puts("invalid argument, use help command");
        return EXIT_FAILURE;
      }
    }

    case 3:
      if (streq(argv[1], "config")) {
        struct Configuration *conf = get_configuration(argv[2]);
        start_server(conf);
        return EXIT_SUCCESS;
      } else {
        puts("invalid argument usage, use help command");
        return EXIT_FAILURE;
      }

    default: {
      puts("invalid argument count, use help command");
      return EXIT_FAILURE;
    }
  }
}
