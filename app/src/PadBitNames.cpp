#include "PadBitNames.h"

#include "PadInputCommon.h"

#include <orbis/_types/pad.h>

#include <cstdio>
#include <string>

namespace {

struct BitInfo {
  unsigned int bit;
  const char *name;
  const char *symbol;
};

constexpr BitInfo kKnownBits[] = {
    {ORBIS_PAD_BUTTON_L3, "L3", "L3"},
    {ORBIS_PAD_BUTTON_R3, "R3", "R3"},
    {ORBIS_PAD_BUTTON_UP, "ORBIS_UP", "UP"},
    {ORBIS_PAD_BUTTON_RIGHT, "ORBIS_RIGHT", "RT"},
    {ORBIS_PAD_BUTTON_DOWN, "ORBIS_DOWN", "DN"},
    {ORBIS_PAD_BUTTON_LEFT, "ORBIS_LEFT", "LF"},
    {ORBIS_PAD_BUTTON_L2, "L2", "L2"},
    {ORBIS_PAD_BUTTON_R2, "R2", "R2"},
    {ORBIS_PAD_BUTTON_L1, "L1", "L1"},
    {ORBIS_PAD_BUTTON_R1, "R1", "R1"},
    {ORBIS_PAD_BUTTON_OPTIONS, "ORBIS_OPTIONS_0x8", "O8"},
    {ORBIS_PAD_BUTTON_TRIANGLE, "TV_OPTIONS_0x1000", "OPT"},
    {ORBIS_PAD_BUTTON_CIRCLE, "CIRCLE", "O"},
    {ORBIS_PAD_BUTTON_CROSS, "CROSS", "X"},
    {ORBIS_PAD_BUTTON_SQUARE, "SQUARE", "SQ"},
    {ORBIS_PAD_BUTTON_TOUCH_PAD, "TOUCH_PAD", "TP"},
    {ORBIS_PAD_BUTTON_PLAYSTATION, "PS_BUTTON", "PS"},
    {ORBIS_PAD_BUTTON_INTERCEPTED, "INTERCEPTED", "INT"},
};

void appendUnknownBits(unsigned int mask, std::string &labelOut, std::string &symbolOut) {
  for (unsigned int b = 0; b < 32; b++) {
    const unsigned int bit = 1u << b;
    if ((mask & bit) == 0) {
      continue;
    }
    bool known = false;
    for (const BitInfo &k : kKnownBits) {
      if (k.bit == bit) {
        known = true;
        break;
      }
    }
    if (known) {
      continue;
    }
    char buf[32];
    std::snprintf(buf, sizeof(buf), "BIT%u", b);
    if (!labelOut.empty()) {
      labelOut += '+';
    }
    labelOut += buf;
    char sym[16];
    std::snprintf(sym, sizeof(sym), "%X", b);
    if (!symbolOut.empty()) {
      symbolOut += ' ';
    }
    symbolOut += sym;
  }
}

} // namespace

std::string PadBitNamesList(unsigned int mask) {
  std::string label;
  std::string sym;
  PadDescribeMask(mask, sym, label);
  return label;
}

void PadDescribeMask(unsigned int mask, std::string &symbolOut, std::string &labelOut) {
  symbolOut.clear();
  labelOut.clear();
  if (mask == 0) {
    symbolOut = "-";
    labelOut = "NONE";
    return;
  }

  for (const BitInfo &k : kKnownBits) {
    if ((mask & k.bit) == 0) {
      continue;
    }
    if (!labelOut.empty()) {
      labelOut += '+';
      symbolOut += ' ';
    }
    labelOut += k.name;
    symbolOut += k.symbol;
  }
  appendUnknownBits(mask, labelOut, symbolOut);

  if (symbolOut.empty()) {
    char hex[16];
    std::snprintf(hex, sizeof(hex), "0x%08X", mask);
    symbolOut = hex;
    labelOut = hex;
  }
}
