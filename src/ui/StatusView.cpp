// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#include "StatusView.h"

#include "../AppState.h"
#include "Ui.h"

namespace {

void updateStatusModeButtons() {
  if (coord_work_button) {
    lv_obj_set_style_bg_color(coord_work_button,
                              lv_color_hex(status_show_machine ? 0x243241 : 0x2563EB),
                              LV_PART_MAIN);
  }
  if (coord_machine_button) {
    lv_obj_set_style_bg_color(coord_machine_button,
                              lv_color_hex(status_show_machine ? 0x2563EB : 0x243241),
                              LV_PART_MAIN);
  }
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
  lv_obj_set_size(modes, 82, LV_PCT(100));
  lv_obj_set_flex_flow(modes, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(modes, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(modes, 10, LV_PART_MAIN);
  lv_obj_clear_flag(modes, LV_OBJ_FLAG_SCROLLABLE);

  coord_work_button = createSmallButton(modes, "Work", onStatusWork, nullptr);
  coord_machine_button = createSmallButton(modes, "Mach", onStatusMachine, nullptr);

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
