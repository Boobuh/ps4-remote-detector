#include "BootTrace.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *kPaths[] = {
    "/data/ps4-remote-detector/boot.log",
    "/user/data/ps4-remote-detector-boot.log",
    "/user/app/REMD00001/boot.log",
    NULL,
};

static void ensureDir(const char *path) {
  char dir[256];
  (void)snprintf(dir, sizeof(dir), "%s", path);
  char *slash = strrchr(dir, '/');
  if (slash == NULL) {
    return;
  }
  *slash = '\0';
  (void)mkdir(dir, 0777);
}

void bootTrace(const char *line) {
  if (line == NULL) {
    return;
  }
  for (int i = 0; kPaths[i] != NULL; i++) {
    ensureDir(kPaths[i]);
    int fd = open(kPaths[i], O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd < 0) {
      continue;
    }
    char buf[512];
    int n = snprintf(buf, sizeof(buf), "%s\n", line);
    if (n > 0) {
      (void)write(fd, buf, (size_t)n);
    }
    close(fd);
    return;
  }
}

void bootTraceStep(const char *step) {
  char line[128];
  (void)snprintf(line, sizeof(line), "step: %s", step);
  bootTrace(line);
}
