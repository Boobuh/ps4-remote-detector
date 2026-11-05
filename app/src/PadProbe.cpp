#include "PadProbe.h"

#include "PadInputCommon.h"

#include <orbis/Keyboard.h>
#include <orbis/Pad.h>
#include <orbis/UserService.h>
#include <orbis/_types/user.h>

#include <cstring>
#include <string>

#include "EventLog.h"
#include "PadBitNames.h"

namespace {

bool isHandle(int h) { return h >= 0; }

unsigned int buttonsWithSticks(const OrbisPadData &data) {
  unsigned int buttons = data.buttons;
  if (data.leftStick.y < 0x40) {
    buttons |= ORBIS_PAD_BUTTON_UP;
  }
  if (data.leftStick.y > 0xC0) {
    buttons |= ORBIS_PAD_BUTTON_DOWN;
  }
  if (data.leftStick.x < 0x40) {
    buttons |= ORBIS_PAD_BUTTON_LEFT;
  }
  if (data.leftStick.x > 0xC0) {
    buttons |= ORBIS_PAD_BUTTON_RIGHT;
  }
  return buttons;
}

} // namespace

void PadProbe::logAttachResult(const char *name, int handle) {
  EventLog::Append("attach", handle >= 0 ? "OK" : "FAIL", static_cast<unsigned>(handle >= 0 ? handle : 0), name,
                   handle >= 0 ? std::to_string(handle) : "none");
}

void PadProbe::rebuildAttachSummary() {
  m_AttachSummary.clear();
  for (const Source &s : m_Sources) {
    if (!m_AttachSummary.empty()) {
      m_AttachSummary += " ";
    }
    m_AttachSummary += s.name + "=" + std::to_string(s.handle);
  }
  if (m_KeyboardHandle >= 0) {
    m_AttachSummary += " KEYBOARD=" + std::to_string(m_KeyboardHandle);
  }
  if (m_AttachSummary.empty()) {
    m_AttachSummary = "(no handles)";
  }
}

bool PadProbe::Init() {
  m_UserId = -1;
  m_KeyboardHandle = -1;
  m_LastKeyboardMask = 0;
  m_LastUserStdRaw = 0;
  m_LastUserStdRawButtons = 0;
  m_AttachSummary = "(init)";
  m_LastKeyboardHint = "KEYBOARD: (not opened)";
  std::memset(m_KeyboardKeyDown, 0, sizeof(m_KeyboardKeyDown));
  m_Sources.clear();

  OrbisUserServiceInitializeParams params {};
  params.priority = ORBIS_KERNEL_PRIO_FIFO_LOWEST;
  (void)sceUserServiceInitialize(&params);
  if (sceUserServiceGetInitialUser(&m_UserId) != 0 || m_UserId < 0) {
    (void)sceUserServiceGetForegroundUser(&m_UserId);
  }

  if (scePadInit() != 0) {
    return false;
  }
  (void)scePadSetProcessFocus();
  tryAttach();
  rebuildAttachSummary();

  EventLog::Append("init", "ready", static_cast<unsigned>(m_Sources.size()), "handle_count", m_AttachSummary);
  return !m_Sources.empty() || m_KeyboardHandle >= 0;
}

bool PadProbe::hasHandle(int h) const {
  for (const Source &s : m_Sources) {
    if (s.handle == h) {
      return true;
    }
  }
  return false;
}

bool PadProbe::hasHandleName(const std::string &name) const {
  for (const Source &s : m_Sources) {
    if (s.name == name) {
      return true;
    }
  }
  return false;
}

void PadProbe::logOkRingSnapshot(const char *triggerSource) {
  for (const Source &s : m_Sources) {
    if (s.isTvRemote || !isHandle(s.handle)) {
      continue;
    }
    OrbisPadData state {};
    if (scePadReadState(s.handle, &state) != 0) {
      continue;
    }
    const unsigned int logical = buttonsWithSticks(state);
    const unsigned int raw = state.buttons;
    char detail[96];
    std::snprintf(detail, sizeof(detail), "conn=%u raw=0x%08X log=0x%08X", static_cast<unsigned>(state.connected), raw,
                  logical);
    EventLog::Append("OK_RING", triggerSource, raw, s.name.c_str(), detail);
    const unsigned int okBits = ps4linuxNormalizePadButtons(raw) & PS4LINUX_TV_REMOTE_OK_MASK;
    if (okBits != 0) {
      EventLog::Append(s.name.c_str(), "OK_RING_HIT", okBits, PadBitNamesList(okBits), "OK?");
    }
  }
}

void PadProbe::tryAttach() {
  if (m_UserId >= 0) {
    int h = scePadOpen(m_UserId, ORBIS_PAD_PORT_TYPE_STANDARD, 0, nullptr);
    if (!isHandle(h)) {
      h = scePadOpen(m_UserId, 0, 0, nullptr);
    }
    if (!isHandle(h)) {
      h = scePadGetHandle(m_UserId, ORBIS_PAD_PORT_TYPE_STANDARD, 0);
    }
    if (!isHandle(h)) {
      h = scePadGetHandle(m_UserId, 0, 0);
    }
    if (isHandle(h) && !hasHandle(h)) {
      Source s {"USER_STD", h, 0, 0, {}, false, true};
      std::memset(s.lastExt, 0, sizeof(s.lastExt));
      m_Sources.push_back(s);
      logAttachResult("USER_STD", h);
    }
  }

  for (int index = 0; index < 4; index++) {
    const std::string name = "SYS_STD_" + std::to_string(index);
    if (hasHandleName(name)) {
      continue;
    }
    int h = scePadOpen(ORBIS_USER_SERVICE_USER_ID_SYSTEM, ORBIS_PAD_PORT_TYPE_STANDARD, index, nullptr);
    if (!isHandle(h)) {
      h = scePadGetHandle(ORBIS_USER_SERVICE_USER_ID_SYSTEM, ORBIS_PAD_PORT_TYPE_STANDARD, index);
    }
    if (!isHandle(h) && m_UserId >= 0) {
      h = scePadGetHandle(m_UserId, ORBIS_PAD_PORT_TYPE_STANDARD, index);
    }
    if (isHandle(h) && !hasHandle(h)) {
      Source s {name, h, 0, 0, {}, false, true};
      std::memset(s.lastExt, 0, sizeof(s.lastExt));
      m_Sources.push_back(s);
      logAttachResult(name.c_str(), h);
    }
  }

  for (int index = 0; index < 4; index++) {
    int h = scePadOpen(ORBIS_USER_SERVICE_USER_ID_SYSTEM, ORBIS_PAD_PORT_TYPE_REMOTE_CONTROL, index, nullptr);
    if (!isHandle(h)) {
      h = scePadGetHandle(ORBIS_USER_SERVICE_USER_ID_SYSTEM, ORBIS_PAD_PORT_TYPE_REMOTE_CONTROL, index);
    }
    if (isHandle(h) && !hasHandle(h)) {
      Source s {"TV_REMOTE_" + std::to_string(index), h, 0, 0, {}, true, true};
      std::memset(s.lastExt, 0, sizeof(s.lastExt));
      m_Sources.push_back(s);
      logAttachResult(("TV_REMOTE_" + std::to_string(index)).c_str(), h);
      break;
    }
  }

  if (m_KeyboardHandle < 0 && m_UserId >= 0) {
    if (sceKeyboardInit() == 0) {
      m_KeyboardHandle = sceKeyboardOpen(m_UserId, 0, 0, nullptr);
      if (m_KeyboardHandle >= 0) {
        (void)sceKeyboardSetProcessFocus();
        m_LastKeyboardHint = "KEYBOARD: ready";
      } else {
        m_LastKeyboardHint = "KEYBOARD: open failed";
      }
    } else {
      m_LastKeyboardHint = "KEYBOARD: init skipped";
    }
  }
}

void PadProbe::logTvRemoteExt(const Source &src, const OrbisPadData &data, unsigned int buttons) {
  if (!src.isTvRemote) {
    return;
  }
  if (std::memcmp(data.ext, src.lastExt, sizeof(src.lastExt)) == 0) {
    return;
  }
  char extHex[64];
  char *p = extHex;
  const size_t cap = sizeof(extHex);
  for (int i = 0; i < 16; i++) {
    const int n = std::snprintf(p, cap - static_cast<size_t>(p - extHex), "%02X", data.ext[i]);
    if (n <= 0) {
      break;
    }
    p += n;
  }
  EventLog::Append("TV_REMOTE_EXT", "CHANGE", buttons, extHex, "EXT");
}

void PadProbe::sampleKeyboard(std::vector<PadInputEvent> &outEvents) {
  if (m_KeyboardHandle < 0) {
    return;
  }
  OrbisKeyboardData kbd {};
  if (sceKeyboardReadState(m_KeyboardHandle, &kbd) != 0) {
    return;
  }

  bool downNow[256] {};
  for (int i = 0; i < kbd.nkeys && i < 32; i++) {
    const uint16_t key = kbd.keycodes[i];
    if (key < 256) {
      downNow[key] = true;
    }
  }

  for (unsigned key = 0; key < 256; key++) {
    if (key == 0) {
      continue;
    }
    if (downNow[key] && !m_KeyboardKeyDown[key]) {
      PadInputEvent ev {};
      ev.source = "KEYBOARD";
      ev.pressed = true;
      ev.mask = key;
      ev.symbol = "KEY";
      ev.labels = "key=" + std::to_string(key);
      outEvents.push_back(ev);
      EventLog::Append("KEYBOARD", "PRESS", key, ev.labels, ev.symbol);
      if (key == ORBIS_KEYBOARD_KEY_RETURN) {
        m_LastKeyboardHint = "KEYBOARD: Return/Enter";
      } else {
        m_LastKeyboardHint = "KEYBOARD: key " + std::to_string(key);
      }
    }
    if (!downNow[key] && m_KeyboardKeyDown[key]) {
      EventLog::Append("KEYBOARD", "RELEASE", key, "key=" + std::to_string(key), "KEY");
    }
  }
  std::memcpy(m_KeyboardKeyDown, downNow, sizeof(downNow));
}

void PadProbe::emitEdge(Source &src, unsigned int prev, unsigned int next, std::vector<PadInputEvent> &outEvents) {
  const unsigned int pressed = next & ~prev;
  const unsigned int released = prev & ~next;

  auto emit = [&](unsigned int mask, bool down) {
    if (mask == 0) {
      return;
    }
    PadInputEvent ev {};
    ev.source = src.name;
    ev.pressed = down;
    ev.mask = mask;
    PadDescribeMask(mask, ev.symbol, ev.labels);
    outEvents.push_back(ev);
    EventLog::Append(ev.source.c_str(), down ? "PRESS" : "RELEASE", mask, ev.labels, ev.symbol);
  };

  for (unsigned int b = 0; b < 32; b++) {
    const unsigned int bit = 1u << b;
    if (pressed & bit) {
      emit(bit, true);
    }
    if (released & bit) {
      emit(bit, false);
    }
  }
  if (src.isTvRemote && pressed != 0) {
    logOkRingSnapshot(src.name.c_str());
  }
}

void PadProbe::sampleSource(Source &src, std::vector<PadInputEvent> &outEvents) {
  if (!isHandle(src.handle)) {
    return;
  }

  auto trackRawButtons = [&](const OrbisPadData &data) {
    if (src.isTvRemote) {
      return;
    }
    const unsigned int rawNorm = ps4linuxNormalizePadButtons(data.buttons);
    if (data.connected == 0 && rawNorm == 0) {
      return;
    }
    if (rawNorm == src.lastRawButtons) {
      return;
    }
    const unsigned int novel = rawNorm & ~src.lastRawButtons;
    src.lastRawButtons = rawNorm;
    EventLog::Append(src.name.c_str(), "RAW", rawNorm, PadBitNamesList(rawNorm), "RAW");
    const unsigned int okNovel = novel & PS4LINUX_TV_REMOTE_OK_MASK;
    if (okNovel != 0) {
      EventLog::Append(src.name.c_str(), "OK_CANDIDATE", okNovel, PadBitNamesList(okNovel), "OK");
    }
  };

  OrbisPadData batch[32];
  for (int drain = 0; drain < 8; drain++) {
    const int got = scePadRead(src.handle, batch, 32);
    if (got <= 0) {
      break;
    }
    for (int i = 0; i < got; i++) {
      trackRawButtons(batch[i]);
      unsigned int actual = src.isTvRemote ? batch[i].buttons : buttonsWithSticks(batch[i]);
      actual = ps4linuxNormalizePadButtons(actual);
      const unsigned int rawNorm = ps4linuxNormalizePadButtons(batch[i].buttons);
      if (!src.isTvRemote && batch[i].connected == 0 && actual == 0 && rawNorm == 0) {
        continue;
      }
      if (batch[i].connected == 0 && !src.allowDisconnected) {
        continue;
      }
      const unsigned int prev = src.lastButtons;
      src.lastButtons = actual;
      emitEdge(src, prev, actual, outEvents);
      if (src.isTvRemote) {
        logTvRemoteExt(src, batch[i], actual);
        std::memcpy(src.lastExt, batch[i].ext, sizeof(src.lastExt));
      }
    }
  }

  OrbisPadData state {};
  if (scePadReadState(src.handle, &state) != 0) {
    return;
  }
  trackRawButtons(state);
  if (!src.allowDisconnected && state.connected == 0) {
    return;
  }
  unsigned int actual = src.isTvRemote ? state.buttons : buttonsWithSticks(state);
  const unsigned int rawButtons = state.buttons;
  actual = ps4linuxNormalizePadButtons(actual);
  const unsigned int rawNormState = ps4linuxNormalizePadButtons(rawButtons);
  if (!src.isTvRemote && state.connected == 0 && actual == 0 && rawNormState == 0 &&
      actual == src.lastButtons) {
    return;
  }
  if ((rawButtons & ORBIS_PAD_BUTTON_INTERCEPTED) != 0 && actual != src.lastButtons) {
    EventLog::Append(src.name.c_str(), "INTERCEPT", rawButtons, "raw_with_intercepted",
                     ps4linuxIsInterceptedOnly(rawButtons) ? "HOME?" : "MIXED");
  }
  if (actual != src.lastButtons) {
    const unsigned int prev = src.lastButtons;
    src.lastButtons = actual;
    emitEdge(src, prev, actual, outEvents);
    if ((actual & ORBIS_PAD_BUTTON_CROSS) != 0 && (prev & ORBIS_PAD_BUTTON_CROSS) == 0) {
      EventLog::Append(src.name.c_str(), "CROSS_OK", actual, "CEC_OK_Candidate", "OK");
    }
  }
  if (src.isTvRemote) {
    logTvRemoteExt(src, state, src.lastButtons);
    std::memcpy(src.lastExt, state.ext, sizeof(src.lastExt));
  }
}

void PadProbe::Update(std::vector<PadInputEvent> &outEvents) {
  outEvents.clear();
  static unsigned counter = 0;
  if ((counter++ % 120) == 0) {
    tryAttach();
    rebuildAttachSummary();
    (void)scePadSetProcessFocus();
  }

  for (Source &src : m_Sources) {
    sampleSource(src, outEvents);
    if (src.name == "USER_STD") {
      m_LastUserStdRaw = src.lastButtons;
      m_LastUserStdRawButtons = src.lastRawButtons;
    }
  }
  sampleKeyboard(outEvents);
}
