#pragma once

#include <orbis/_types/pad.h>

#ifndef ORBIS_PAD_PORT_TYPE_REMOTE_CONTROL
#define ORBIS_PAD_PORT_TYPE_REMOTE_CONTROL 16
#endif

#ifndef ORBIS_PAD_BUTTON_PLAYSTATION
#define ORBIS_PAD_BUTTON_PLAYSTATION 0x00010000u
#endif

#ifndef ORBIS_PAD_BUTTON_INTERCEPTED
#define ORBIS_PAD_BUTTON_INTERCEPTED 0x80000000u
#endif

inline unsigned int ps4linuxNormalizePadButtons(unsigned int raw) {
  return raw & ~ORBIS_PAD_BUTTON_INTERCEPTED;
}

inline bool ps4linuxIsInterceptedOnly(unsigned int raw) {
  return raw == ORBIS_PAD_BUTTON_INTERCEPTED;
}

inline bool ps4linuxIsTvRemoteHomeRaw(unsigned int raw) {
  if (ps4linuxIsInterceptedOnly(raw)) {
    return true;
  }
  const unsigned int stripped = ps4linuxNormalizePadButtons(raw);
  return (stripped & ORBIS_PAD_BUTTON_PLAYSTATION) != 0;
}

// Match ps4-linux-pro Controller.h — bits that may mean TV center OK / confirm.
constexpr unsigned int PS4LINUX_TV_REMOTE_OK_MASK =
    ORBIS_PAD_BUTTON_OPTIONS | ORBIS_PAD_BUTTON_CROSS | ORBIS_PAD_BUTTON_CIRCLE |
    ORBIS_PAD_BUTTON_L3 | ORBIS_PAD_BUTTON_R3 | ORBIS_PAD_BUTTON_TOUCH_PAD |
    ORBIS_PAD_BUTTON_PLAYSTATION;
