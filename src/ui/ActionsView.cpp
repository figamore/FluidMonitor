// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#include "ActionsView.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

#include "../AppState.h"
#include "../Colors.h"
#include "JobControls.h"
#include "JogView.h"
#include "Ui.h"

namespace {

lv_obj_t* actions_home_view = nullptr;
lv_obj_t* actions_job_view = nullptr;
lv_obj_t* job_header_title = nullptr;
lv_obj_t* job_back_btn = nullptr;
lv_obj_t* job_refresh_btn = nullptr;
lv_obj_t* job_list_view = nullptr;
lv_obj_t* job_info_view = nullptr;
lv_obj_t* job_running_view = nullptr;
lv_obj_t* job_list = nullptr;
lv_obj_t* job_info_label = nullptr;
lv_obj_t* job_running_label = nullptr;
lv_obj_t* job_progress_track = nullptr;
lv_obj_t* job_progress_fill = nullptr;

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

void setViewVisible(lv_obj_t* view, bool visible) {
  if (!view) {
    return;
  }
  if (visible) {
    lv_obj_clear_flag(view, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(view, LV_OBJ_FLAG_HIDDEN);
  }
}

int clampPercent(int percent) {
  if (percent < 0) {
    return 0;
  }
  if (percent > 100) {
    return 100;
  }
  return percent;
}

lv_obj_t* createProgressTrack(lv_obj_t* parent, int width, int height, lv_obj_t** fill) {
  lv_obj_t* track = lv_obj_create(parent);
  lv_obj_remove_style_all(track);
  lv_obj_set_size(track, width, height);
  lv_obj_set_style_bg_opa(track, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(track, lv_color_hex(Colors::kBgInactive), LV_PART_MAIN);
  lv_obj_set_style_radius(track, height / 2, LV_PART_MAIN);
  lv_obj_clear_flag(track, LV_OBJ_FLAG_SCROLLABLE);

  *fill = lv_obj_create(track);
  lv_obj_remove_style_all(*fill);
  lv_obj_set_size(*fill, 0, LV_PCT(100));
  lv_obj_align(*fill, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_set_style_bg_opa(*fill, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(*fill, lv_color_hex(Colors::kStatusSuccess), LV_PART_MAIN);
  lv_obj_set_style_radius(*fill, height / 2, LV_PART_MAIN);
  lv_obj_clear_flag(*fill, LV_OBJ_FLAG_SCROLLABLE);
  return track;
}

void setProgress(lv_obj_t* fill, int percent) {
  if (fill) {
    lv_obj_set_width(fill, LV_PCT(clampPercent(percent)));
  }
}

bool joinPath(char* dest, size_t size, const char* dir, const char* name) {
  if (!dest || size == 0 || !dir || !name || !name[0]) {
    return false;
  }
  const char* sep = dir[strlen(dir) - 1] == '/' ? "" : "/";
  const int written = snprintf(dest, size, "%s%s%s", dir, sep, name);
  return written > 0 && static_cast<size_t>(written) < size;
}

void clearJobList() {
  if (job_list) {
    lv_obj_clean(job_list);
  }
}

void onFileSelected(lv_event_t* event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }
  const size_t index = reinterpret_cast<uintptr_t>(lv_event_get_user_data(event));
  if (!job_state.files || index >= job_state.file_count) {
    return;
  }
  copyText(job_state.selected, sizeof(job_state.selected), job_state.files[index].name);
  job_state.selected_is_dir = false;
  updateActionsJobStatus();
}

void rebuildJobList() {
  clearJobList();
  if (!job_list) {
    return;
  }
  if (job_state.list_loading) {
    lv_list_add_text(job_list, "Reading SD...");
    return;
  }
  if (job_state.list_error) {
    lv_list_add_text(job_list, job_state.list_message[0] ? job_state.list_message : "List failed");
    return;
  }
  if (!job_state.files || job_state.file_count == 0) {
    lv_list_add_text(job_list, "No jobs");
    return;
  }

  bool any_file = false;
  for (size_t i = 0; i < job_state.file_count; ++i) {
    const FluidNCFileInfo& entry = job_state.files[i];
    if (entry.isDirectory()) {
      continue;
    }
    any_file = true;
    lv_obj_t* row = lv_list_add_btn(job_list, LV_SYMBOL_FILE, entry.name);
    lv_obj_add_event_cb(row, onFileSelected, LV_EVENT_CLICKED, reinterpret_cast<void*>(i));
  }
  if (!any_file) {
    lv_list_add_text(job_list, "No jobs");
  } else if (job_state.file_truncated) {
    lv_list_add_text(job_list, "More files on SD");
  }
}

void requestJobList() {
  job_state.list_loading = true;
  job_state.list_error = false;
  job_state.files = nullptr;
  job_state.file_count = 0;
  job_state.file_truncated = false;
  job_state.selected[0] = '\0';
  copyText(job_state.list_message, sizeof(job_state.list_message), "Reading SD...");
  pending_job_ui = true;
  updateActionsJobStatus();

  if (!fluidnc.requestFileList(job_state.path)) {
    job_state.list_loading = false;
    job_state.list_error = true;
    copyText(job_state.list_message, sizeof(job_state.list_message), "Not connected");
    pending_job_ui = true;
    updateActionsJobStatus();
  }
}

void onCommandAction(lv_event_t* event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }
  const char* command = static_cast<const char*>(lv_event_get_user_data(event));
  if (!command) {
    return;
  }
  cancelJog();
  fluidnc.sendLine(command);
}

void onShowJobs(lv_event_t* event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }
  setViewVisible(actions_home_view, false);
  setViewVisible(actions_job_view, true);
  if (job_state.file_count == 0 && !job_state.list_loading) {
    requestJobList();
  } else {
    updateActionsJobStatus();
  }
}

void onRunSelected(lv_event_t* event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED || job_state.selected[0] == '\0') {
    return;
  }

  char path[160];
  if (!joinPath(path, sizeof(path), job_state.path, job_state.selected)) {
    return;
  }

  cancelJog();
  if (fluidnc.runFile(path)) {
    job_state.initiated_here = true;
    job_state.active = true;
    job_state.active_percent = 0;
    copyText(job_state.active_name, sizeof(job_state.active_name), job_state.selected);
    setStatus(lv_color_hex(Colors::kStatusWarning));
  } else {
    job_state.list_error = true;
    copyText(job_state.list_message, sizeof(job_state.list_message), "Machine must be Idle");
    setStatus(lv_color_hex(Colors::kStatusError));
  }
  pending_job_ui = true;
  updateActionsJobStatus();
}

void onRefreshJobs(lv_event_t* event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }
  requestJobList();
}

void onBackFromJobs(lv_event_t* event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }
  // From a selected file, step back to the list; otherwise leave the browser.
  if (!machineJobControlsVisible() && job_state.selected[0] != '\0') {
    job_state.selected[0] = '\0';
    updateActionsJobStatus();
    return;
  }
  setViewVisible(actions_job_view, false);
  setViewVisible(actions_home_view, true);
}

void addAxisRow(lv_obj_t* col, char axis, lv_color_t color, const char* zero_cmd, const char* home_cmd,
                const char* goto_cmd) {
  lv_obj_t* row = lv_obj_create(col);
  lv_obj_remove_style_all(row);
  lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

  char zero_text[3] = {' ', axis, '\0'};
  char home_text[16];
  char goto_text[16];
  snprintf(home_text, sizeof(home_text), LV_SYMBOL_HOME " %c", axis);
  snprintf(goto_text, sizeof(goto_text), LV_SYMBOL_GPS " %c", axis);

  lv_obj_t* zero = createZeroButton(row, nullptr, zero_text, onCommandAction, const_cast<char*>(zero_cmd));
  lv_obj_set_size(zero, 58, 40);
  accentButton(zero, color);

  lv_obj_t* home = createSmallButton(row, home_text, onCommandAction, const_cast<char*>(home_cmd));
  lv_obj_set_size(home, 58, 40);
  accentButton(home, color);

  lv_obj_t* go = createSmallButton(row, goto_text, onCommandAction, const_cast<char*>(goto_cmd));
  lv_obj_set_size(go, 58, 40);
  accentButton(go, color);
}

lv_obj_t* createJobButton(lv_obj_t* parent, const char* text, lv_event_cb_t cb, uint32_t color) {
  lv_obj_t* button = createSmallButton(parent, text, cb, nullptr);
  lv_obj_set_size(button, 56, 40);
  accentButton(button, lv_color_hex(color));
  return button;
}

void createActionsHome(lv_obj_t* tab) {
  actions_home_view = lv_obj_create(tab);
  lv_obj_remove_style_all(actions_home_view);
  lv_obj_set_size(actions_home_view, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(actions_home_view, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(actions_home_view, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(actions_home_view, 8, LV_PART_MAIN);
  lv_obj_clear_flag(actions_home_view, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* left = lv_obj_create(actions_home_view);
  lv_obj_remove_style_all(left);
  lv_obj_set_size(left, LV_PCT(66), LV_PCT(100));
  lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(left, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);

  addAxisRow(left, 'X', lv_color_hex(0xF87171), "G10L20P0X0", "$HX", "G90G0X0");
  addAxisRow(left, 'Y', lv_color_hex(0x34D399), "G10L20P0Y0", "$HY", "G90G0Y0");
  addAxisRow(left, 'Z', lv_color_hex(Colors::kAxisZ), "G10L20P0Z0", "$HZ", "G90G0Z0");

  lv_obj_t* right = lv_obj_create(actions_home_view);
  lv_obj_remove_style_all(right);
  lv_obj_set_size(right, LV_PCT(30), LV_PCT(100));
  lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(right, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(right, 8, LV_PART_MAIN);
  lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* zero_all = createSmallButton(right, "Zero All", onCommandAction, const_cast<char*>("G10L20P0X0Y0Z0"));
  lv_obj_set_width(zero_all, LV_PCT(100));
  lv_obj_set_flex_grow(zero_all, 1);
  lv_obj_t* home_all = createSmallButton(right, LV_SYMBOL_HOME " All", onCommandAction, const_cast<char*>("$H"));
  lv_obj_set_width(home_all, LV_PCT(100));
  lv_obj_set_flex_grow(home_all, 1);
  lv_obj_t* run_job = createSmallButton(right, "Run Job", onShowJobs, nullptr);
  lv_obj_set_width(run_job, LV_PCT(100));
  lv_obj_set_flex_grow(run_job, 1);
}

void createJobListView(lv_obj_t* parent) {
  job_list_view = lv_obj_create(parent);
  lv_obj_remove_style_all(job_list_view);
  lv_obj_set_size(job_list_view, LV_PCT(100), LV_PCT(100));
  lv_obj_clear_flag(job_list_view, LV_OBJ_FLAG_SCROLLABLE);

  job_list = lv_list_create(job_list_view);
  lv_obj_set_size(job_list, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(job_list, lv_color_hex(Colors::kBgPanel), LV_PART_MAIN);
  lv_obj_set_style_border_width(job_list, 1, LV_PART_MAIN);
  lv_obj_set_style_border_color(job_list, lv_color_hex(Colors::kBorder), LV_PART_MAIN);
  lv_obj_set_scrollbar_mode(job_list, LV_SCROLLBAR_MODE_AUTO);
}

void createJobInfoView(lv_obj_t* parent) {
  job_info_view = makePanel(parent);
  lv_obj_set_size(job_info_view, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(job_info_view, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(job_info_view, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(job_info_view, 14, LV_PART_MAIN);
  lv_obj_set_style_pad_row(job_info_view, 18, LV_PART_MAIN);
  lv_obj_add_flag(job_info_view, LV_OBJ_FLAG_HIDDEN);

  job_info_label = lv_label_create(job_info_view);
  lv_obj_set_width(job_info_label, LV_PCT(100));
  lv_label_set_long_mode(job_info_label, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(job_info_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_text_font(job_info_label, &lv_font_montserrat_18, LV_PART_MAIN);
  lv_obj_set_style_text_color(job_info_label, lv_color_hex(Colors::kTextPrimary), LV_PART_MAIN);

  lv_obj_t* run = createSmallButton(job_info_view, LV_SYMBOL_PLAY " Run", onRunSelected, nullptr);
  lv_obj_set_size(run, 132, 44);
  accentButton(run, lv_color_hex(Colors::kStatusSuccess));
}

void createJobRunningView(lv_obj_t* parent) {
  job_running_view = makePanel(parent);
  lv_obj_set_size(job_running_view, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(job_running_view, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(job_running_view, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(job_running_view, 12, LV_PART_MAIN);
  lv_obj_set_style_pad_row(job_running_view, 14, LV_PART_MAIN);
  lv_obj_add_flag(job_running_view, LV_OBJ_FLAG_HIDDEN);

  job_running_label = lv_label_create(job_running_view);
  lv_obj_set_width(job_running_label, LV_PCT(100));
  lv_label_set_long_mode(job_running_label, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(job_running_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_text_font(job_running_label, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_set_style_text_color(job_running_label, lv_color_hex(Colors::kTextPrimary), LV_PART_MAIN);

  job_progress_track = createProgressTrack(job_running_view, 230, 12, &job_progress_fill);

  lv_obj_t* controls = lv_obj_create(job_running_view);
  lv_obj_remove_style_all(controls);
  lv_obj_set_size(controls, LV_PCT(100), 42);
  lv_obj_set_flex_flow(controls, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(controls, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(controls, LV_OBJ_FLAG_SCROLLABLE);

  createJobButton(controls, LV_SYMBOL_PAUSE, onJobPause, Colors::kStatusWarning);
  createJobButton(controls, LV_SYMBOL_PLAY, onJobResume, Colors::kStatusSuccess);
  createJobButton(controls, LV_SYMBOL_STOP, onJobAbort, Colors::kStatusError);
  createJobButton(controls, LV_SYMBOL_WARNING, onJobEStop, Colors::kStatusDanger);
}

void createJobHeader(lv_obj_t* parent) {
  lv_obj_t* header = lv_obj_create(parent);
  lv_obj_remove_style_all(header);
  lv_obj_set_size(header, LV_PCT(100), 32);
  lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(header, 6, LV_PART_MAIN);
  lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

  job_back_btn = createSmallButton(header, LV_SYMBOL_LEFT, onBackFromJobs, nullptr);
  lv_obj_set_size(job_back_btn, 46, 30);

  job_header_title = lv_label_create(header);
  lv_obj_set_flex_grow(job_header_title, 1);
  lv_label_set_long_mode(job_header_title, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(job_header_title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_text_font(job_header_title, &lv_font_montserrat_14, LV_PART_MAIN);
  lv_obj_set_style_text_color(job_header_title, lv_color_hex(Colors::kTextSecondary), LV_PART_MAIN);

  job_refresh_btn = createSmallButton(header, LV_SYMBOL_REFRESH, onRefreshJobs, nullptr);
  lv_obj_set_size(job_refresh_btn, 46, 30);
}

void createJobView(lv_obj_t* tab) {
  actions_job_view = lv_obj_create(tab);
  lv_obj_remove_style_all(actions_job_view);
  lv_obj_set_size(actions_job_view, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(actions_job_view, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(actions_job_view, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(actions_job_view, 6, LV_PART_MAIN);
  lv_obj_clear_flag(actions_job_view, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(actions_job_view, LV_OBJ_FLAG_HIDDEN);

  createJobHeader(actions_job_view);

  lv_obj_t* body = lv_obj_create(actions_job_view);
  lv_obj_remove_style_all(body);
  lv_obj_set_width(body, LV_PCT(100));
  lv_obj_set_flex_grow(body, 1);
  lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);

  createJobListView(body);
  createJobInfoView(body);
  createJobRunningView(body);
  updateActionsJobStatus();
}

}  // namespace

void createActionsTab(lv_obj_t* tab) {
  lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(tab, 8, LV_PART_MAIN);
  lv_obj_set_scrollbar_mode(tab, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);

  createActionsHome(tab);
  createJobView(tab);
}

void updateActionsJobStatus() {
  const bool running = machineJobControlsVisible();
  const bool selected = !running && job_state.selected[0] != '\0';
  const bool listing = !running && !selected;

  setViewVisible(job_list_view, listing);
  setViewVisible(job_info_view, selected);
  setViewVisible(job_running_view, running);
  setViewVisible(job_refresh_btn, listing);

  if (running) {
    if (job_header_title) {
      lv_label_set_text(job_header_title, activeJobName());
    }
    if (job_running_label) {
      lv_label_set_text_fmt(job_running_label, "%s  %d%%", activeJobName(), activeJobPercent());
    }
    setProgress(job_progress_fill, activeJobPercent());
    pending_job_ui = false;
    return;
  }

  if (selected) {
    if (job_header_title) {
      lv_label_set_text(job_header_title, job_state.path);
    }
    if (job_info_label) {
      lv_label_set_text(job_info_label, job_state.selected);
    }
    pending_job_ui = false;
    return;
  }

  if (job_header_title) {
    lv_label_set_text(job_header_title, job_state.path);
  }
  if (pending_job_ui) {
    rebuildJobList();
  }
  pending_job_ui = false;
}
