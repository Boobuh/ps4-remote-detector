// PS4 Remote Detector — map HDMI TV remote / pad bits for homebrew input.

#include "BootTrace.h"
#include "EventLog.h"
#include "Graphics.h"
#include "PadProbe.h"
#include "SystemServiceRuntime.h"

#include <orbis/Pad.h>
#include <orbis/_types/pad.h>
#include <orbis/Sysmodule.h>
#include <orbis/SystemService.h>
#include <orbis/_types/sysmodule.h>

#include <clocale>
#include <cstdio>
#include <signal.h>
#include <string>
#include <unistd.h>
#include <vector>

#include "libLog.h"

static const OrbisSysModuleInternal kSysutil = (OrbisSysModuleInternal)0x80000026;
static const OrbisSysModuleInternal kGnmDriver = (OrbisSysModuleInternal)0x80000052;

static void fatalLoop(Graphics *graph, const char *reason) {
  bootTrace(reason);
  EventLog::Append("fatal", "stop", 0, reason, reason);
  if (graph != nullptr && graph->IsReady()) {
    Color red = {0xFF, 0x44, 0x44, 0xFF};
    Color bg = {0x10, 0x08, 0x08, 0xFF};
    for (int i = 0; i < 3; i++) {
      graph->FrameBufferFill(bg);
      graph->DrawRectangle(200, 400, 1520, 120, red);
      graph->SwapBuffer(i);
    }
  }
  for (;;) {
    sleep(5);
  }
}

static void loadCoreModulesOrFatal() {
  bootTraceStep("11 SYSTEM_SERVICE");
  if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_SYSTEM_SERVICE) != 0) {
    fatalLoop(nullptr, "fatal: SYSTEM_SERVICE");
  }
  bootTraceStep("12 VIDEO_OUT");
  if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_VIDEO_OUT) != 0) {
    fatalLoop(nullptr, "fatal: VIDEO_OUT");
  }
  bootTraceStep("13 NET");
  if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_NET) != 0) {
    fatalLoop(nullptr, "fatal: NET");
  }
  bootTraceStep("14 USER_SERVICE");
  if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_USER_SERVICE) != 0) {
    fatalLoop(nullptr, "fatal: USER_SERVICE");
  }
  bootTraceStep("15 PAD");
  if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_PAD) != 0) {
    fatalLoop(nullptr, "fatal: PAD");
  }
  bootTraceStep("15b KEYBOARD");
  (void)sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_KEYBOARD);
  (void)sceSysmoduleLoadModule(ORBIS_SYSMODULE_KEYBOARD);
  bootTraceStep("16 SYSUTIL");
  if (sceSysmoduleLoadModuleInternal(kSysutil) != 0) {
    fatalLoop(nullptr, "fatal: SYSUTIL");
  }
  bootTraceStep("17 GNM");
  if (sceSysmoduleLoadModuleInternal(kGnmDriver) != 0) {
    fatalLoop(nullptr, "fatal: GNM");
  }
  bootTraceStep("18 FREETYPE");
  if (sceSysmoduleLoadModule(ORBIS_SYSMODULE_FREETYPE_OL) < 0) {
    fatalLoop(nullptr, "fatal: FREETYPE_OL");
  }
}

static void drawCentered(Graphics *g, std::vector<FT_Face> &faces, int fontSize, const std::string &text, int y,
                         Color fg) {
  g->SetFontSize(faces, fontSize);
  FontSize size {};
  g->GetTextSize(text, faces, &size);
  const int x = (g->GetScreenWidth() - size.width) / 2;
  g->DrawText(text, faces, x, y, fg, fg);
}

static int appMain() {
  bootTrace("app: remote detector main entered");
  (void)setlocale(LC_ALL, "en_US.UTF-8");
  EventLog::InitSession();

  loadCoreModulesOrFatal();

  Graphics graph;
  if (!graph.IsReady()) {
    fatalLoop(nullptr, "fatal: graphics not ready");
  }
  bootTraceStep("GRAPHICS_OK");

  std::vector<FT_Face> faces;
  const int fontRc = graph.InitTypefaces(faces, {"/app0/assets/fonts/NotoSans-Regular.ttf"}, 24);
  if (fontRc < 0 || faces.empty()) {
    fatalLoop(&graph, "fatal: font load failed");
  }
  bootTraceStep("FONT_OK");

  bootTraceStep("PAD_PROBE_BEGIN");
  PadProbe probe;
  if (!probe.Init()) {
    EventLog::Append("init", "warn", 0, "no_pads", "none");
    bootTrace("warn: no pad handles (TV remote / DS4)");
  }
  bootTraceStep("PAD_PROBE_OK");

  std::string bigSymbol = "?";
  std::string bigLabel = "Press any remote / pad button";
  std::string subLine = "Logging to /data/ps4-remote-detector/";
  std::vector<std::string> history;
  int flip = 0;

  sceSystemServiceHideSplashScreen();
  bootTraceStep("first_frame_loop");

  bool systemUiOverlay = false;
  int displayHoldFrames = 0;
  for (;;) {
    SystemServicePumpFrame(systemUiOverlay);

    std::vector<PadInputEvent> events;
    probe.Update(events);

    for (const PadInputEvent &ev : events) {
      if (!ev.pressed) {
        continue;
      }
      bigSymbol = ev.symbol.empty() ? "?" : ev.symbol;
      bigLabel = ev.labels.empty() ? bigSymbol : ev.labels;
      if (ev.mask == 0x80000000u || ev.mask == 0x00010000u) {
        bigSymbol = "HOME";
        bigLabel = "TV Home (exits app — NOT Enter)";
      } else if (ev.mask == 0x00001000u) {
        bigSymbol = "OPT";
        bigLabel = "TV Options (Triangle 0x1000 — not center OK)";
      } else if (ev.mask == 0x00000008u) {
        bigSymbol = "OK?";
        bigLabel = "Orbis Options 0x8 — center OK candidate";
      } else if (ev.mask == ORBIS_PAD_BUTTON_CROSS || ev.mask == ORBIS_PAD_BUTTON_L3 ||
                 ev.mask == ORBIS_PAD_BUTTON_R3) {
        bigSymbol = "OK";
        bigLabel = "Pad Cross / stick click";
      }
      char detail[160];
      std::snprintf(detail, sizeof(detail), "%s  0x%08X%s", ev.source.c_str(), ev.mask,
                    systemUiOverlay ? "  [overlay]" : "");
      subLine = detail;

      char hist[256];
      std::snprintf(hist, sizeof(hist), "%s %s 0x%08X", ev.source.c_str(), ev.labels.c_str(), ev.mask);
      history.insert(history.begin(), hist);
      if (history.size() > 8) {
        history.pop_back();
      }
      displayHoldFrames = 180;
    }
    if (displayHoldFrames > 0) {
      displayHoldFrames--;
    }

    graph.WaitFlip(flip > 0 ? flip - 1 : -1);
    Color bg = {0x06, 0x0A, 0x12, 0xFF};
    Color hi = {0x44, 0xFF, 0xCC, 0xFF};
    Color mid = {0xAA, 0xDD, 0xFF, 0xFF};
    Color dim = {0x55, 0x77, 0x99, 0xFF};
    graph.FrameBufferFill(bg);

    drawCentered(&graph, faces, 28, "PS4 Remote Detector", 48, mid);
    if (displayHoldFrames == 0 && bigSymbol == "?") {
      drawCentered(&graph, faces, 20, "Center OK silent? Watch 0x8 + USER_STD + KEYBOARD lines below", 380, dim);
    }
    drawCentered(&graph, faces, 96, bigSymbol, 420, hi);
    drawCentered(&graph, faces, 48, bigLabel, 560, mid);
    drawCentered(&graph, faces, 32, subLine, 640, dim);

    graph.SetFontSize(faces, 22);
    graph.DrawText("Recent (newest first):", faces, 80, 720, dim, dim);
    for (size_t i = 0; i < history.size(); i++) {
      graph.DrawText(history[i], faces, 80, static_cast<int>(760 + i * 28), dim, dim);
    }

    graph.DrawText(probe.attachSummary().c_str(), faces, 80, 900, dim, dim);
    char userStd[96];
    std::snprintf(userStd, sizeof(userStd), "USER_STD logical=0x%08X raw_buttons=0x%08X",
                  probe.lastUserStdRaw(), probe.lastUserStdRawButtons());
    graph.DrawText(userStd, faces, 80, 940, dim, dim);
    graph.DrawText(probe.lastKeyboardHint().c_str(), faces, 80, 980, dim, dim);
    graph.DrawText("Press center OK: watch OK_RING / OK_CANDIDATE / RAW in map.csv", faces, 80, 1020, dim, dim);
    graph.DrawText("FTP: /data/ps4-remote-detector/map.csv", faces, 80, 1060, dim, dim);
    graph.SwapBuffer(flip);
    flip++;
  }
}

static void onCrashSignal(int sig) {
  char line[64];
  std::snprintf(line, sizeof(line), "crash: signal %d", sig);
  bootTrace(line);
}

extern "C" int main(void) {
  struct sigaction action {};
  action.sa_handler = onCrashSignal;
  sigemptyset(&action.sa_mask);
  for (int sig : {SIGSEGV, SIGBUS, SIGABRT, SIGILL, SIGFPE}) {
    sigaction(sig, &action, nullptr);
  }

  logSetLogLevel(LL_All);
  bootTrace("eboot: PS4 Remote Detector unified");
  bootTraceStep("00 main entered");
  return appMain();
}
