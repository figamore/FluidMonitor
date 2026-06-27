// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#include "StatusView.h"

#include "../AppState.h"
#include "Ui.h"

namespace {

void styleSegment(lv_obj_t* segment, bool active) {
  if (!segment) {
    return;
  }
  lv_obj_set_style_bg_color(segment, lv_color_hex(active ? 0x2563EB : 0x1B2430), LV_PART_MAIN);
  lv_obj_set_style_text_color(segment, lv_color_hex(active ? 0xFFFFFF : 0x93A4B7), LV_PART_MAIN);
}

void updateStatusModeButtons() {
  styleSegment(coord_work_button, !status_show_machine);
  styleSegment(coord_machine_button, status_show_machine);
}

// One half of the Work/Mach segmented toggle. The buttons sit flush inside a
// rounded, clipped container so together they read as a single control.
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
    lv_obj_set_style_border_color(segment, lv_color_hex(0x344151), LV_PART_MAIN);
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
  lv_obj_set_style_bg_color(modes, lv_color_hex(0x111821), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(modes, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(modes, 1, LV_PART_MAIN);
  lv_obj_set_style_border_color(modes, lv_color_hex(0x344151), LV_PART_MAIN);
  lv_obj_set_style_pad_all(modes, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_row(modes, 0, LV_PART_MAIN);
  lv_obj_clear_flag(modes, LV_OBJ_FLAG_SCROLLABLE);

  coord_work_button = createSegment(modes, LV_SYMBOL_EDIT "\nWork", onStatusWork, false);
  coord_machine_button = createSegment(modes, LV_SYMBOL_GPS "\nMach", onStatusMachine, true);

  lv_obj_t* panel = makePanel(tab);
  lv_obj_set_size(panel, 210, LV_PCT(100));
  lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_left(panel, 14, LV_PART_MAIN);
  lv_obj_set_style_pad_right(panel, 10, LV_PART_MAIN);
  lv_obj_set_style_pad_top(panel, 8, LV_PART_MAIN);
  lv_obj_set_style_pad_row(panel, 6, LV_PART_MAIN);

  status_x_label = lv_label_create(panel);
  status_y_label = lv_label_create(panel);
  status_z_label = lv_label_create(panel);
  lv_obj_set_width(status_x_label, LV_PCT(100));
  lv_obj_set_width(status_y_label, LV_PCT(100));
  lv_obj_set_width(status_z_label, LV_PCT(100));
  lv_obj_set_style_text_font(status_x_label, &lv_font_montserrat_32, LV_PART_MAIN);
  lv_obj_set_style_text_font(status_y_label, &lv_font_montserrat_32, LV_PART_MAIN);
  lv_obj_set_style_text_font(status_z_label, &lv_font_montserrat_32, LV_PART_MAIN);
  formatAxis(status_x_label, "X", 0);
  formatAxis(status_y_label, "Y", 0);
  formatAxis(status_z_label, "Z", 0);
  selectStatusMode(false);
}
