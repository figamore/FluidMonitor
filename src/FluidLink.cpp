// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#include "FluidLink.h"

#include <string.h>

#include "AppState.h"
#include "Colors.h"
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
    setStatus(lv_color_hex(Colors::kStatusInfo));
    setStateLabel(LV_SYMBOL_SHUFFLE " Pairing Mode");
    setPairButtonText(LV_SYMBOL_CLOSE "  Cancel");
  });
  fluidnc.onPairingCancelled([]() {
    setStatus(lv_color_hex(Colors::kStatusWarning));
    setStateLabel("State: --");
    setPairButtonText(LV_SYMBOL_SHUFFLE "  Pair");
  });
  fluidnc.onPairingFailed([]() {
    setStatus(lv_color_hex(Colors::kStatusError));
    setStateLabel("Pairing failed");
    setPairButtonText(LV_SYMBOL_SHUFFLE "  Pair");
  });
  fluidnc.onPaired([](const FluidNCMachine&) {
    setStatus(lv_color_hex(Colors::kStatusSuccess));
    setPairButtonText(LV_SYMBOL_SHUFFLE "  Pair");
    updatePeerLabel();
    showPairingSuccess();
  });
  fluidnc.onConnected([](const FluidNCMachine&) {
    setStatus(lv_color_hex(Colors::kStatusSuccess));
  });
  fluidnc.onDisconnected([](const FluidNCMachine&) {
    jog_active = false;
    setStatus(lv_color_hex(Colors::kStatusWarning));
  });
  fluidnc.onSendFailed([]() {
    setStatus(lv_color_hex(Colors::kStatusError));
  });
  fluidnc.onStatus(onStatus);

  if (!fluidnc.begin()) {
    setStatus(lv_color_hex(Colors::kStatusError));
    return;
  }
  setStatus(lv_color_hex(Colors::kStatusSuccess));
  updatePeerLabel();
}

void pollFluidLink() {
  fluidnc.poll();
}

bool machineAllowsSleep() {
  return !fluidnc.isConnected() || fluidnc.isIdle();
}
