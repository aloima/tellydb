#include "../../headers/utils.h"

#include <errno.h>

#include <fcntl.h>
#include <unistd.h>

int open_file(const char *file, int flags) {
  int fd;

  if ((fd = open(file, (O_RDWR | O_CREAT | O_DIRECT | flags), (S_IRUSR | S_IWUSR))) == -1) {
    switch (errno) {
      case EINVAL:
        write_log(LOG_ERR, "Direct I/O does not be supported by your file system, cannot open file.");
        break;

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

  return fd;
}
