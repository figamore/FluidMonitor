// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#include "JobControls.h"

#include <string.h>

#include "../AppState.h"
#include "../Colors.h"
#include "JogView.h"
#include "Ui.h"

namespace {

void markSendResult(bool ok) {
  setStatus(lv_color_hex(ok ? Colors::kStatusWarning : Colors::kStatusError));
}

}  // namespace

bool machineJobControlsVisible() {
  return job_state.active || latest_dro.file_active;
}

bool machineJobPaused() {
  return job_state.paused || strncmp(latest_dro.state, "Hold", 4) == 0;
}

const char* activeJobName() {
  if (job_state.active_name[0]) {
    return job_state.active_name;
  }
  if (latest_dro.file_name[0]) {
    return latest_dro.file_name;
  }
  return "External job";
}

int activeJobPercent() {
  int percent = job_state.active_percent;
  if (percent < 0) {
    percent = latest_dro.file_percent;
  }
  if (percent < 0) {
    return 0;
  }
  if (percent > 100) {
    return 100;
  }
  return percent;
}

void onJobPause(lv_event_t* event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }
  cancelJog();
  markSendResult(fluidnc.feedHold());
}

void onJobResume(lv_event_t* event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }
  cancelJog();
  markSendResult(fluidnc.cycleStart());
}

void onJobAbort(lv_event_t* event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }
  cancelJog();
  markSendResult(fluidnc.stopFile());
}

void onJobEStop(lv_event_t* event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }
  cancelJog();
  markSendResult(fluidnc.softReset());
}
