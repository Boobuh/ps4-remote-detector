#pragma once

#include <orbis/_types/pad.h>

#include <cstdint>
#include <string>
#include <vector>

struct PadInputEvent {
  std::string source;
  bool pressed;
  unsigned int mask;
  std::string symbol;
  std::string labels;
};

class PadProbe {
public:
  bool Init();
  void Update(std::vector<PadInputEvent> &outEvents);
  unsigned int lastUserStdRaw() const { return m_LastUserStdRaw; }
  unsigned int lastUserStdRawButtons() const { return m_LastUserStdRawButtons; }
  const std::string &lastKeyboardHint() const { return m_LastKeyboardHint; }
  const std::string &attachSummary() const { return m_AttachSummary; }

private:
  struct Source {
    std::string name;
    int handle;
    unsigned int lastButtons;
    unsigned int lastRawButtons;
    uint8_t lastExt[16];
    bool isTvRemote;
    bool allowDisconnected;
  };

  bool hasHandle(int h) const;
  bool hasHandleName(const std::string &name) const;
  void tryAttach();
  void sampleSource(Source &src, std::vector<PadInputEvent> &outEvents);
  void emitEdge(Source &src, unsigned int prev, unsigned int next, std::vector<PadInputEvent> &outEvents);

  int m_UserId;
  std::vector<Source> m_Sources;
  int m_KeyboardHandle;
  unsigned int m_LastKeyboardMask;
  unsigned int m_LastUserStdRaw;
  unsigned int m_LastUserStdRawButtons;
  std::string m_LastKeyboardHint;
  std::string m_AttachSummary;
  bool m_KeyboardKeyDown[256];
  void sampleKeyboard(std::vector<PadInputEvent> &outEvents);
  void logTvRemoteExt(const Source &src, const OrbisPadData &data, unsigned int buttons);
  void logOkRingSnapshot(const char *triggerSource);
  void logAttachResult(const char *name, int handle);
  void rebuildAttachSummary();
};
