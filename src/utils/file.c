#include "../../headers/utils.h"

#include <errno.h>

#include <fcntl.h>
#include <unistd.h>

int open_file(const char *file, int flags) {
  int fd;

#if defined(__linux__)
  if ((fd = open(file, (O_RDWR | O_CREAT | O_DIRECT | flags), (S_IRUSR | S_IWUSR))) == -1) {
#else
  if ((fd = open(file, (O_RDWR | O_CREAT | flags), (S_IRUSR | S_IWUSR))) == -1) {
#endif
    switch (errno) {
    #if defined(__linux__)
      case EINVAL:
        write_log(LOG_ERR, "Direct I/O does not be supported by your file system, cannot open file.");
        break;
    #endif

      case EISDIR:
        write_log(LOG_ERR, "Specified file is a directory, cannot open file.");
        break;

      case ENOMEM:
        write_log(LOG_ERR, "No available memory to create/open file.");
        break;

      case EROFS:
        write_log(LOG_ERR, "Your file system is read-only, cannot open file for writing.");
        break;

      default:
        write_log(LOG_ERR, "File cannot be opened or created.");
    }

    return -1;
  }

  #if defined(__APPLE__)
  if ((fcntl(fd, F_NOCACHE, 1)) == -1) {
    switch (errno) {
      case EACCES:
        write_log(LOG_ERR, "File descriptor is not accessible to set for no kernel caching mode.");
        break;

      default:
        write_log(LOG_ERR, "File descriptor cannot be set for no kernel caching mode.");
    }

    close(file);
    return -1;
  }
  #endif

  #if (!defined(__APPLE__) && !defined(__linux__))
  write_log(LOG_WARN, "File descriptor cannot be opened as no kernel caching mode or Direct I/O. Use MacOS or Linux.");
  #endif

  return fd;
}
