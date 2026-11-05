#include "Utf8Util.h"

#include <cstddef>

namespace Utf8Util {

std::vector<uint32_t> Codepoints(const std::string &input) {
  std::vector<uint32_t> out;
  out.reserve(input.size());
  const unsigned char *s = reinterpret_cast<const unsigned char *>(input.data());
  const size_t len = input.size();
  for (size_t i = 0; i < len;) {
    const unsigned char c = s[i];
    if (c < 0x80) {
      out.push_back(c);
      i++;
      continue;
    }
    if ((c & 0xE0) == 0xC0 && i + 1 < len) {
      out.push_back(((c & 0x1F) << 6) | (s[i + 1] & 0x3F));
      i += 2;
      continue;
    }
    if ((c & 0xF0) == 0xE0 && i + 2 < len) {
      out.push_back(((c & 0x0F) << 12) | ((s[i + 1] & 0x3F) << 6) | (s[i + 2] & 0x3F));
      i += 3;
      continue;
    }
    if ((c & 0xF8) == 0xF0 && i + 3 < len) {
      out.push_back(((c & 0x07) << 18) | ((s[i + 1] & 0x3F) << 12) | ((s[i + 2] & 0x3F) << 6) | (s[i + 3] & 0x3F));
      i += 4;
      continue;
    }
    out.push_back('?');
    i++;
  }
  return out;
}

} // namespace Utf8Util
