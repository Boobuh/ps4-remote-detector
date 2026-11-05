#pragma once

#include <string>

class EventLog {
public:
  static void InitSession();
  static void Append(const char *source, const char *phase, unsigned int rawMask, const std::string &labels,
                     const std::string &symbol);
  static const char *ActivePath();
};
