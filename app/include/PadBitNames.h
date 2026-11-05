#pragma once

#include <cstdint>
#include <string>

/// Human label + on-screen symbol for a single raw button mask (may combine bits).
void PadDescribeMask(unsigned int mask, std::string &symbolOut, std::string &labelOut);

/// Comma-separated list of known bit names for logging.
std::string PadBitNamesList(unsigned int mask);
