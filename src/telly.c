#include <telly.h>

#include <stdio.h>
#include <stdlib.h>

static inline void print_version() {
  puts((
    "tellydb version " VERSION "\n"
    "open source at https://github.com/aloima/tellydb\n"
    "licensed under BSD-3 Clause Clear License"
  ));
}

static inline void print_help(const char *executable) {
  printf((
    "%s [<ARGUMENTS>]\n\n"
    "-v, --version | Prints version\n"
    "-h, --help    | Prints this page\n"
  ), executable);
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    return EXIT_SUCCESS;
  }

  for (int i = 1; i < argc; ++i) {
    const char *arg = argv[i];

    if (arg[0] == '-') {
      const char *value = (arg + 1);

      if (STREQ(value, "v") || STREQ(value, "-version")) {
        print_version();
        return EXIT_SUCCESS;
      } else if (STREQ(value, "h") || STREQ(value, "-help")) {
        print_help(argv[0]);
        return EXIT_SUCCESS;
      } else {
        printf("Invalid flag: %s\n", arg);
        return EXIT_FAILURE;
      }
    } else {
      printf("Invalid argument: %s\n", arg);
      return EXIT_FAILURE;
    }
  }

  return 0;
}
