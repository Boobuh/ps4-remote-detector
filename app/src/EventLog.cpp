#include "EventLog.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sys/stat.h>

namespace {

const char *kPaths[] = {
    "/data/ps4-remote-detector/events.log",
    "/data/ps4-remote-detector/map.csv",
    "/user/data/ps4-remote-detector-events.log",
    "/user/app/REMD00001/events.log",
    nullptr,
};

const char *gActivePath = kPaths[0];

void ensureDir() {
  (void)mkdir("/data/ps4-remote-detector", 0777);
}

void writeLine(const char *path, const char *line) {
  FILE *f = std::fopen(path, "a");
  if (f == nullptr) {
    return;
  }
  std::fputs(line, f);
  std::fputc('\n', f);
  std::fflush(f);
  std::fclose(f);
}

void isoTimestamp(char *out, size_t n) {
  out[0] = '\0';
  const std::time_t now = std::time(nullptr);
  if (now <= 0) {
    return;
  }
  struct tm tm_buf {};
  if (gmtime_r(&now, &tm_buf) == nullptr) {
    return;
  }
  std::strftime(out, n, "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
}

} // namespace

void EventLog::InitSession() {
  ensureDir();
  for (int i = 0; kPaths[i] != nullptr; i++) {
    FILE *f = std::fopen(kPaths[i], "a");
    if (f != nullptr) {
      gActivePath = kPaths[i];
      char ts[64];
      isoTimestamp(ts, sizeof(ts));
      std::fprintf(f, "# session %s PS4 Remote Detector\n", ts);
      std::fflush(f);
      std::fclose(f);
      break;
    }
  }
  writeLine("/data/ps4-remote-detector/map.csv",
            "timestamp,source,phase,raw_hex,labels,symbol");
}

const char *EventLog::ActivePath() { return gActivePath; }

void EventLog::Append(const char *source, const char *phase, unsigned int rawMask, const std::string &labels,
                      const std::string &symbol) {
  char ts[64];
  isoTimestamp(ts, sizeof(ts));
  char line[1024];
  std::snprintf(line, sizeof(line), "[%s] %s %s raw=0x%08X labels=%s symbol=%s", ts, source, phase, rawMask,
                labels.c_str(), symbol.c_str());

  for (int i = 0; kPaths[i] != nullptr; i++) {
    if (std::strstr(kPaths[i], "map.csv") != nullptr) {
      char csv[1024];
      std::snprintf(csv, sizeof(csv), "%s,%s,%s,0x%08X,\"%s\",\"%s\"", ts, source, phase, rawMask, labels.c_str(),
                    symbol.c_str());
      writeLine(kPaths[i], csv);
    } else {
      writeLine(kPaths[i], line);
    }
  }
}
