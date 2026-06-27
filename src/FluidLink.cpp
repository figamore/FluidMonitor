// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#include "FluidLink.h"

#include <string.h>

#include "AppState.h"
#include "ui/SettingsView.h"
#include "ui/Ui.h"

namespace {

AxisTriplet toTriplet(const FluidNCPosition& pos) {
  AxisTriplet out;
  out.x = pos.x;
  out.y = pos.y;
  out.z = pos.z;
  out.valid = pos.valid;
  return out;
}

void onStatus(const FluidNCStatus& status) {
  strncpy(latest_dro.state, status.state, sizeof(latest_dro.state) - 1);
  latest_dro.state[sizeof(latest_dro.state) - 1] = '\0';
  latest_dro.work = toTriplet(status.work);
  latest_dro.machine = toTriplet(status.machine);
  latest_dro.wco = toTriplet(status.wco);
  latest_dro.inch = status.inch;
  pending_dro = true;
}

}  // namespace

void initFluidLink() {
  fluidnc.onPairingStarted([]() {
    setStatus(lv_color_hex(0x38BDF8));
    setPairButtonText("Cancel");
  });
  fluidnc.onPairingCancelled([]() {
    setStatus(lv_color_hex(0xF59E0B));
    setPairButtonText("Pair");
  });
  fluidnc.onPairingFailed([]() {
    setStatus(lv_color_hex(0xF87171));
    setPairButtonText("Pair");
  });
  fluidnc.onPaired([](const FluidNCMachine&) {
    setStatus(lv_color_hex(0x34D399));
    setPairButtonText("Pair");
    updatePeerLabel();
    showPairingSuccess();
  });
  fluidnc.onConnected([](const FluidNCMachine&) {
    setStatus(lv_color_hex(0x34D399));
  });
  fluidnc.onDisconnected([](const FluidNCMachine&) {
    jog_active = false;
    setStatus(lv_color_hex(0xF59E0B));
  });
  fluidnc.onSendFailed([]() {
    setStatus(lv_color_hex(0xF87171));
  });
  fluidnc.onStatus(onStatus);

  if (!fluidnc.begin()) {
    setStatus(lv_color_hex(0xF87171));
    return;
  }
  setStatus(lv_color_hex(0x34D399));
  updatePeerLabel();
}

void pollFluidLink() {
  fluidnc.poll();
}
