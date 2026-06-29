// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#include "StatusView.h"

#include "../AppState.h"
#include "../Colors.h"
#include "JobControls.h"
#include "Ui.h"

namespace {

lv_obj_t* status_panel = nullptr;
lv_obj_t* status_job_controls = nullptr;
lv_obj_t* status_job_progress_row = nullptr;
lv_obj_t* status_job_progress = nullptr;
lv_obj_t* status_job_track = nullptr;
lv_obj_t* status_job_fill = nullptr;

void styleSegment(lv_obj_t* segment, bool active) {
  if (!segment) {
    return;
  }
  lv_obj_set_style_bg_color(segment, lv_color_hex(active ? Colors::kBgAccent : Colors::kBgInactive), LV_PART_MAIN);
  lv_obj_set_style_text_color(segment, lv_color_hex(active ? Colors::kTextWhite : Colors::kTextMuted), LV_PART_MAIN);
}

void updateStatusModeButtons() {
  styleSegment(coord_work_button, !status_show_machine);
  styleSegment(coord_machine_button, status_show_machine);
}

lv_obj_t* createSegment(lv_obj_t* parent, const char* text, lv_event_cb_t cb, bool divider) {
  lv_obj_t* segment = lv_btn_create(parent);
  lv_obj_remove_style_all(segment);
  lv_obj_set_size(segment, LV_PCT(100), 0);
  lv_obj_set_flex_grow(segment, 1);
  lv_obj_set_style_bg_opa(segment, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_radius(segment, 0, LV_PART_MAIN);
  if (divider) {
    lv_obj_set_style_border_width(segment, 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(segment, LV_BORDER_SIDE_TOP, LV_PART_MAIN);
    lv_obj_set_style_border_color(segment, lv_color_hex(Colors::kBorder), LV_PART_MAIN);
  }
  lv_obj_add_event_cb(segment, cb, LV_EVENT_CLICKED, nullptr);

  lv_obj_t* label = lv_label_create(segment);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_center(label);
  return segment;
}

void selectStatusMode(bool machine) {
  status_show_machine = machine;
  updateStatusModeButtons();
  updateStatusLabels();
}

void onStatusWork(lv_event_t* event) {
  if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
    selectStatusMode(false);
  }
}

void onStatusMachine(lv_event_t* event) {
  if (lv_event_get_code(event) == LV_EVENT_CLICKED) {
    selectStatusMode(true);
  }
}

lv_obj_t* createStatusJobButton(lv_obj_t* parent, const char* text, lv_event_cb_t cb, uint32_t color) {
  lv_obj_t* button = createSmallButton(parent, text, cb, nullptr);
  lv_obj_set_size(button, 38, 30);
  accentButton(button, lv_color_hex(color));
  return button;
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

}  // namespace

void updateStatusLabels() {
  const AxisTriplet& axes = status_show_machine ? latest_dro.machine : latest_dro.work;
  if (!axes.valid) {
    return;
  }
  formatAxis(status_x_label, "X", axes.x);
  formatAxis(status_y_label, "Y", axes.y);
  formatAxis(status_z_label, "Z", axes.z);
}

void updateStatusJobControls() {
  const bool visible = machineJobControlsVisible();
  if (status_job_controls) {
    if (visible) {
      lv_obj_clear_flag(status_job_controls, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(status_job_controls, LV_OBJ_FLAG_HIDDEN);
    }
  }
  if (status_job_progress_row) {
    if (visible) {
      lv_obj_clear_flag(status_job_progress_row, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(status_job_progress_row, LV_OBJ_FLAG_HIDDEN);
    }
  }
  if (status_job_progress) {
    if (visible) {
      lv_obj_clear_flag(status_job_progress, LV_OBJ_FLAG_HIDDEN);
      lv_label_set_text_fmt(status_job_progress, "%d%%", activeJobPercent());
    } else {
      lv_obj_add_flag(status_job_progress, LV_OBJ_FLAG_HIDDEN);
    }
  }
  if (status_job_track) {
    if (visible) {
      lv_obj_clear_flag(status_job_track, LV_OBJ_FLAG_HIDDEN);
      setProgress(status_job_fill, activeJobPercent());
    } else {
      lv_obj_add_flag(status_job_track, LV_OBJ_FLAG_HIDDEN);
    }
  }

  const lv_font_t* axis_font = visible ? &lv_font_montserrat_24 : &lv_font_montserrat_32;
  lv_obj_set_style_text_font(status_x_label, axis_font, LV_PART_MAIN);
  lv_obj_set_style_text_font(status_y_label, axis_font, LV_PART_MAIN);
  lv_obj_set_style_text_font(status_z_label, axis_font, LV_PART_MAIN);
  if (status_panel) {
    lv_obj_set_style_pad_row(status_panel, visible ? 3 : 6, LV_PART_MAIN);
  }
}

void createStatusTab(lv_obj_t* tab) {
  lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_all(tab, 8, LV_PART_MAIN);
  lv_obj_set_style_pad_column(tab, 8, LV_PART_MAIN);
  lv_obj_set_scrollbar_mode(tab, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* modes = lv_obj_create(tab);
  lv_obj_remove_style_all(modes);
  lv_obj_set_size(modes, 84, LV_PCT(100));
  lv_obj_set_flex_flow(modes, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_radius(modes, 10, LV_PART_MAIN);
  lv_obj_set_style_clip_corner(modes, true, LV_PART_MAIN);
  lv_obj_set_style_bg_color(modes, lv_color_hex(Colors::kBgTertiary), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(modes, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(modes, 1, LV_PART_MAIN);
  lv_obj_set_style_border_color(modes, lv_color_hex(Colors::kBorder), LV_PART_MAIN);
  lv_obj_set_style_pad_all(modes, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_row(modes, 0, LV_PART_MAIN);
  lv_obj_clear_flag(modes, LV_OBJ_FLAG_SCROLLABLE);

  coord_work_button = createSegment(modes, LV_SYMBOL_EDIT "\nWork", onStatusWork, false);
  coord_machine_button = createSegment(modes, LV_SYMBOL_GPS "\nMach", onStatusMachine, true);

  status_panel = makePanel(tab);
  lv_obj_set_size(status_panel, 210, LV_PCT(100));
  lv_obj_set_flex_flow(status_panel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_left(status_panel, 14, LV_PART_MAIN);
  lv_obj_set_style_pad_right(status_panel, 10, LV_PART_MAIN);
  lv_obj_set_style_pad_top(status_panel, 8, LV_PART_MAIN);
  lv_obj_set_style_pad_row(status_panel, 6, LV_PART_MAIN);

  status_x_label = lv_label_create(status_panel);
  status_y_label = lv_label_create(status_panel);
  status_z_label = lv_label_create(status_panel);
  lv_obj_set_width(status_x_label, LV_PCT(100));
  lv_obj_set_width(status_y_label, LV_PCT(100));
  lv_obj_set_width(status_z_label, LV_PCT(100));
  lv_obj_set_style_text_font(status_x_label, &lv_font_montserrat_32, LV_PART_MAIN);
  lv_obj_set_style_text_font(status_y_label, &lv_font_montserrat_32, LV_PART_MAIN);
  lv_obj_set_style_text_font(status_z_label, &lv_font_montserrat_32, LV_PART_MAIN);

  status_job_progress_row = lv_obj_create(status_panel);
  lv_obj_remove_style_all(status_job_progress_row);
  lv_obj_set_size(status_job_progress_row, LV_PCT(100), 16);
  lv_obj_set_flex_flow(status_job_progress_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(status_job_progress_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(status_job_progress_row, LV_OBJ_FLAG_SCROLLABLE);

  status_job_track = createProgressTrack(status_job_progress_row, 122, 8, &status_job_fill);

  status_job_progress = lv_label_create(status_job_progress_row);
  lv_obj_set_width(status_job_progress, 44);
  lv_obj_set_style_text_font(status_job_progress, &lv_font_montserrat_12, LV_PART_MAIN);
  lv_obj_set_style_text_color(status_job_progress, lv_color_hex(Colors::kTextSecondary), LV_PART_MAIN);

  status_job_controls = lv_obj_create(status_panel);
  lv_obj_remove_style_all(status_job_controls);
  lv_obj_set_size(status_job_controls, LV_PCT(100), 32);
  lv_obj_set_flex_flow(status_job_controls, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(status_job_controls, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(status_job_controls, LV_OBJ_FLAG_SCROLLABLE);
  createStatusJobButton(status_job_controls, LV_SYMBOL_PAUSE, onJobPause, Colors::kStatusWarning);
  createStatusJobButton(status_job_controls, LV_SYMBOL_PLAY, onJobResume, Colors::kStatusSuccess);
  createStatusJobButton(status_job_controls, LV_SYMBOL_STOP, onJobAbort, Colors::kStatusError);
  createStatusJobButton(status_job_controls, LV_SYMBOL_WARNING, onJobEStop, Colors::kStatusDanger);

  formatAxis(status_x_label, "X", 0);
  formatAxis(status_y_label, "Y", 0);
  formatAxis(status_z_label, "Z", 0);
  updateStatusJobControls();
  selectStatusMode(false);
}
