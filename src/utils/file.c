#include <telly.h>

#include <errno.h>

#include <fcntl.h>
#include <unistd.h>

#define CHECK_ERROR(ERROR_CODE, message, ...) \
  case (ERROR_CODE): \
    write_log(LOG_ERR, (message), ##__VA_ARGS__); \
    break

int open_file(const char *file, int flags) {
  int fd;

#if defined(__linux__)
  if ((fd = open(file, (O_RDWR | O_CREAT | O_DIRECT | flags), (S_IRUSR | S_IWUSR))) == -1) {
#else
  if ((fd = open(file, (O_RDWR | O_CREAT | flags), (S_IRUSR | S_IWUSR))) == -1) {
#endif
    switch (errno) {
#if defined(__linux__)
      CHECK_ERROR(EINVAL, "Direct I/O does not be supported by your file system, cannot open file.");
#endif
      CHECK_ERROR(EISDIR, "Specified file is a directory, cannot open file.");
      CHECK_ERROR(ENOMEM, "No available memory to create/open file.");
      CHECK_ERROR(EROFS,  "Your file system is read-only, cannot open file for writing.");

      default:
        write_log(LOG_ERR, "File cannot be opened or created.");
    }

    return -1;
  }

#if defined(__APPLE__)
  if ((fcntl(fd, F_NOCACHE, 1)) == -1) {
    switch (errno) {
      CHECK_ERROR(EACCES,  "File descriptor is not accessible to set for no kernel caching mode.");

      default:
        write_log(LOG_ERR, "File descriptor cannot be set for no kernel caching mode.");
    }

    close(fd);
    return -1;
  }
#endif

#if (!defined(__APPLE__) && !defined(__linux__))
  write_log(LOG_WARN, "File descriptor cannot be opened as no kernel caching mode or Direct I/O. Use MacOS or Linux.");
#endif

  if (lockf(fd, F_TEST, 0) == -1) {
    write_log(LOG_ERR, "%s file cannot be locked, because it is already locked by another process.", file);
    close(fd);

    return -1;
  } else {
    if (lockf(fd, F_LOCK, 0) == -1) {
      switch (errno) {
        CHECK_ERROR(EDEADLK, "%s file cannot be locked, because a deadlock is detected.", file);
        CHECK_ERROR(EINTR,   "Locking operation of %s file is interrupted.", file);
      }

      close(fd);
      return -1;
    } else {
      return fd;
    }
  }
}
