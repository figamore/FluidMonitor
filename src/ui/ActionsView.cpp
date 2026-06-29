// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#include "ActionsView.h"

#include <cstdio>

#include "../AppState.h"
#include "JogView.h"
#include "Ui.h"

namespace {

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

}  // namespace

void createActionsTab(lv_obj_t* tab) {
  lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(tab, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(tab, 8, LV_PART_MAIN);
  lv_obj_set_style_pad_column(tab, 8, LV_PART_MAIN);
  lv_obj_set_scrollbar_mode(tab, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* left = lv_obj_create(tab);
  lv_obj_remove_style_all(left);
  lv_obj_set_size(left, LV_PCT(66), LV_PCT(100));
  lv_obj_set_flex_flow(left, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(left, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);

  addAxisRow(left, 'X', lv_color_hex(0xF87171), "G10L20P0X0", "$HX", "G90G0X0");
  addAxisRow(left, 'Y', lv_color_hex(0x34D399), "G10L20P0Y0", "$HY", "G90G0Y0");
  addAxisRow(left, 'Z', lv_color_hex(0x60A5FA), "G10L20P0Z0", "$HZ", "G90G0Z0");

  lv_obj_t* right = lv_obj_create(tab);
  lv_obj_remove_style_all(right);
  lv_obj_set_size(right, LV_PCT(30), LV_PCT(100));
  lv_obj_set_flex_flow(right, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(right, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(right, 10, LV_PART_MAIN);
  lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* zero_all = createSmallButton(right, "Zero All", onCommandAction, const_cast<char*>("G10L20P0X0Y0Z0"));
  lv_obj_set_width(zero_all, LV_PCT(100));
  lv_obj_set_flex_grow(zero_all, 1);
  lv_obj_t* home_all = createSmallButton(right, LV_SYMBOL_HOME " All", onCommandAction, const_cast<char*>("$H"));
  lv_obj_set_width(home_all, LV_PCT(100));
  lv_obj_set_flex_grow(home_all, 1);
}
