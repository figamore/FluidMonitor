// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#include "ActionsView.h"

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

}  // namespace

void createActionsTab(lv_obj_t* tab) {
  lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(tab, 10, LV_PART_MAIN);
  lv_obj_set_style_pad_row(tab, 10, LV_PART_MAIN);
  lv_obj_set_scrollbar_mode(tab, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* row_one = lv_obj_create(tab);
  lv_obj_remove_style_all(row_one);
  lv_obj_set_size(row_one, LV_PCT(100), 48);
  lv_obj_set_flex_flow(row_one, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row_one, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(row_one, LV_OBJ_FLAG_SCROLLABLE);

  createSmallButton(row_one, "Zero X", onCommandAction, const_cast<char*>("G10L20P0X0"));
  createSmallButton(row_one, "Zero Y", onCommandAction, const_cast<char*>("G10L20P0Y0"));
  createSmallButton(row_one, "Zero Z", onCommandAction, const_cast<char*>("G10L20P0Z0"));

  lv_obj_t* row_two = lv_obj_create(tab);
  lv_obj_remove_style_all(row_two);
  lv_obj_set_size(row_two, LV_PCT(100), 48);
  lv_obj_set_flex_flow(row_two, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row_two, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(row_two, 10, LV_PART_MAIN);
  lv_obj_clear_flag(row_two, LV_OBJ_FLAG_SCROLLABLE);

  createSmallButton(row_two, "Zero All", onCommandAction, const_cast<char*>("G10L20P0X0Y0Z0"));
  createSmallButton(row_two, "Home All", onCommandAction, const_cast<char*>("$H"));
}
