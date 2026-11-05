#include "SystemServiceRuntime.h"

#include <cstring>

extern "C" int sceSystemServiceReceiveEvent(OrbisSystemServiceEventLite *event);
extern "C" int sceSystemServiceGetStatus(void *status);
extern "C" int sceSystemServicePowerTick(void);

namespace {

constexpr int kSystemServiceNoEvent = static_cast<int>(0x80940002u);

alignas(16) uint8_t g_SystemServiceStatus[kOrbisSystemServiceStatusBytes];
OrbisSystemServiceEventLite g_SystemServiceEvent {};

} // namespace

void SystemServicePumpFrame(bool &systemUiOverlayActive) {
  std::memset(g_SystemServiceStatus, 0, sizeof(g_SystemServiceStatus));
  if (sceSystemServiceGetStatus(g_SystemServiceStatus) == 0) {
    const auto *st = reinterpret_cast<const OrbisSystemServiceStatusLite *>(g_SystemServiceStatus);
    systemUiOverlayActive = st->is_system_ui_overlaid != 0;
  }

  for (int i = 0; i < 16; i++) {
    std::memset(&g_SystemServiceEvent, 0, sizeof(g_SystemServiceEvent));
    const int ret = sceSystemServiceReceiveEvent(&g_SystemServiceEvent);
    if (ret == kSystemServiceNoEvent) {
      break;
    }
    if (ret != 0) {
      break;
    }
  }

  (void)sceSystemServicePowerTick();
}
