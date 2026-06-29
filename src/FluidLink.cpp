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

void copyText(char* dest, size_t size, const char* src) {
  if (!dest || size == 0) {
    return;
  }
  if (!src) {
    dest[0] = '\0';
    return;
  }
  strncpy(dest, src, size - 1);
  dest[size - 1] = '\0';
}

bool stateStartsWith(const char* prefix) {
  return strncmp(latest_dro.state, prefix, strlen(prefix)) == 0;
}

void onStatus(const FluidNCStatus& status) {
  copyText(latest_dro.state, sizeof(latest_dro.state), status.state);
  latest_dro.work = toTriplet(status.work);
  latest_dro.machine = toTriplet(status.machine);
  latest_dro.wco = toTriplet(status.wco);
  latest_dro.inch = status.inch;
  copyText(latest_dro.file_name, sizeof(latest_dro.file_name), status.fileName);
  latest_dro.file_percent = status.filePercent;
  latest_dro.file_active = status.fileActive;

  if (!stateStartsWith("Alarm")) {
    alarm_reason[0] = '\0';
    alarm_code = 0;
  }

  job_state.active = status.fileActive || stateStartsWith("Run") || stateStartsWith("Hold");
  job_state.paused = stateStartsWith("Hold");
  job_state.active_percent = status.fileActive ? status.filePercent : -1;
  if (status.fileName[0]) {
    copyText(job_state.active_name, sizeof(job_state.active_name), status.fileName);
  } else if (!job_state.active) {
    job_state.active_name[0] = '\0';
    job_state.active_percent = -1;
    job_state.initiated_here = false;
  }

  pending_dro = true;
}

void onFileList(const char* path, const FluidNCFileInfo* files, size_t count, bool truncated) {
  copyText(job_state.path, sizeof(job_state.path), path && path[0] ? path : "/sd");
  job_state.files = files;
  job_state.file_count = count;
  job_state.file_truncated = truncated;
  job_state.list_loading = false;
  job_state.list_error = false;
  job_state.list_message[0] = '\0';
  job_state.selected[0] = '\0';
  job_state.selected_is_dir = false;

  pending_job_ui = true;
}

void onFileListError(const char* path, const char* status) {
  copyText(job_state.path, sizeof(job_state.path), path && path[0] ? path : "/sd");
  job_state.files = nullptr;
  job_state.file_count = 0;
  job_state.file_truncated = false;
  job_state.list_loading = false;
  job_state.list_error = true;
  copyText(job_state.list_message, sizeof(job_state.list_message), status && status[0] ? status : "List failed");
  pending_job_ui = true;
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
  fluidnc.onAlarm([](int code, const char* description) {
    alarm_code = code;
    copyText(alarm_reason, sizeof(alarm_reason), description && description[0] ? description : "Machine alarm");
    pending_dro = true;
  });
  fluidnc.onStatus(onStatus);
  fluidnc.onFileList(onFileList);
  fluidnc.onFileListError(onFileListError);

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
