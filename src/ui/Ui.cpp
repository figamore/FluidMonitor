// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#include "Ui.h"

#include <stdio.h>

#include "../AppState.h"
#include "../BatteryMonitor.h"
#include "ActionsView.h"
#include "JogView.h"
#include "SettingsView.h"
#include "StatusView.h"

namespace {

void initStyles() {
  lv_style_init(&style_screen);
  lv_style_set_bg_color(&style_screen, lv_color_hex(0x080B0F));
  lv_style_set_text_color(&style_screen, lv_color_hex(0xEEF6F8));

  lv_style_init(&style_topbar);
  lv_style_set_bg_color(&style_topbar, lv_color_hex(0x151A20));
  lv_style_set_bg_opa(&style_topbar, LV_OPA_COVER);
  lv_style_set_border_width(&style_topbar, 0);
  lv_style_set_pad_left(&style_topbar, 10);
  lv_style_set_pad_right(&style_topbar, 10);

  lv_style_init(&style_panel);
  lv_style_set_bg_color(&style_panel, lv_color_hex(0x17202A));
  lv_style_set_bg_opa(&style_panel, LV_OPA_COVER);
  lv_style_set_border_width(&style_panel, 1);
  lv_style_set_border_color(&style_panel, lv_color_hex(0x344151));
  lv_style_set_radius(&style_panel, 8);

  lv_style_init(&style_muted);
  lv_style_set_text_color(&style_muted, lv_color_hex(0x93A4B7));
  lv_style_set_text_font(&style_muted, &lv_font_montserrat_12);

  lv_style_init(&style_button);
  lv_style_set_bg_color(&style_button, lv_color_hex(0x243241));
  lv_style_set_bg_opa(&style_button, LV_OPA_COVER);
  lv_style_set_border_width(&style_button, 1);
  lv_style_set_border_color(&style_button, lv_color_hex(0x405267));
  lv_style_set_radius(&style_button, 7);
  lv_style_set_shadow_width(&style_button, 0);
  lv_style_set_text_color(&style_button, lv_color_hex(0xF8FAFC));

  lv_style_init(&style_overlay);
  lv_style_set_bg_color(&style_overlay, lv_color_hex(0x101820));
  lv_style_set_bg_opa(&style_overlay, LV_OPA_COVER);
  lv_style_set_border_width(&style_overlay, 1);
  lv_style_set_border_color(&style_overlay, lv_color_hex(0x38BDF8));
  lv_style_set_radius(&style_overlay, 8);
  lv_style_set_pad_all(&style_overlay, 10);
}

void onMenu(lv_event_t*) {
  if (!menu_panel) {
    return;
  }
  if (lv_obj_has_flag(menu_panel, LV_OBJ_FLAG_HIDDEN)) {
    lv_obj_clear_flag(menu_panel, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(menu_panel, LV_OBJ_FLAG_HIDDEN);
  }
}

lv_obj_t* battery_indicator = nullptr;
lv_obj_t* battery_fill = nullptr;
lv_obj_t* battery_charge = nullptr;

lv_color_t batteryColor(int level) {
  if (level >= 50) {
    return lv_color_hex(0x22C55E);
  }
  if (level >= 25) {
    return lv_color_hex(0xFACC15);
  }
  return lv_color_hex(0xEF4444);
}

void updateBatteryTimer(lv_timer_t*) {
  updateBatteryIndicator();
}

void createBatteryIndicator(lv_obj_t* parent) {
  if (!batteryAvailable()) {
    return;
  }

  battery_indicator = lv_obj_create(parent);
  lv_obj_remove_style_all(battery_indicator);
  lv_obj_set_size(battery_indicator, 28, 18);
  lv_obj_clear_flag(battery_indicator, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* body = lv_obj_create(battery_indicator);
  lv_obj_remove_style_all(body);
  lv_obj_set_size(body, 20, 13);
  lv_obj_align(body, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_set_style_border_width(body, 1, LV_PART_MAIN);
  lv_obj_set_style_border_color(body, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(body, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);

  battery_fill = lv_obj_create(body);
  lv_obj_remove_style_all(battery_fill);
  lv_obj_set_size(battery_fill, 2, 11);
  lv_obj_align(battery_fill, LV_ALIGN_TOP_LEFT, 1, 1);
  lv_obj_set_style_bg_opa(battery_fill, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(battery_fill, lv_color_hex(0xEF4444), LV_PART_MAIN);
  lv_obj_clear_flag(battery_fill, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* nub = lv_obj_create(battery_indicator);
  lv_obj_remove_style_all(nub);
  lv_obj_set_size(nub, 4, 7);
  lv_obj_align(nub, LV_ALIGN_LEFT_MID, 20, 0);
  lv_obj_set_style_bg_opa(nub, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(nub, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
  lv_obj_clear_flag(nub, LV_OBJ_FLAG_SCROLLABLE);

  battery_charge = lv_label_create(battery_indicator);
  lv_label_set_text(battery_charge, LV_SYMBOL_CHARGE);
  lv_obj_set_style_text_font(battery_charge, &lv_font_montserrat_12, LV_PART_MAIN);
  lv_obj_set_style_text_color(battery_charge, lv_color_hex(0x22C55E), LV_PART_MAIN);
  lv_obj_align(battery_charge, LV_ALIGN_CENTER, -2, 0);

  updateBatteryIndicator();
  lv_timer_create(updateBatteryTimer, 1000, nullptr);
}

}  // namespace

void setStatus(lv_color_t color) {
  if (status_dot) {
    lv_obj_set_style_bg_color(status_dot, color, LV_PART_MAIN);
  }
}

void formatAxis(lv_obj_t* label, const char* axis, float value) {
  if (!label) {
    return;
  }
  char text[28];
  snprintf(text, sizeof(text), "%s %9.3f", axis, value);
  lv_label_set_text(label, text);
}

void updateBatteryIndicator() {
  if (!battery_indicator || !battery_fill || !battery_charge) {
    return;
  }

  int level = batteryLevel();
  if (level < 0) {
    lv_obj_add_flag(battery_indicator, LV_OBJ_FLAG_HIDDEN);
    return;
  }

  lv_obj_clear_flag(battery_indicator, LV_OBJ_FLAG_HIDDEN);

  int fill_w = 2;
  if (level >= 75) {
    fill_w = 18;
  } else if (level >= 50) {
    fill_w = 9;
  } else if (level >= 25) {
    fill_w = 4;
  }

  bool charging = batteryCharging();
  lv_obj_set_width(battery_fill, fill_w);
  lv_obj_set_style_bg_color(battery_fill, charging ? lv_color_hex(0x080B0F) : batteryColor(level), LV_PART_MAIN);

  if (charging) {
    lv_obj_clear_flag(battery_charge, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(battery_charge, LV_OBJ_FLAG_HIDDEN);
  }
}

lv_obj_t* makePanel(lv_obj_t* parent) {
  lv_obj_t* panel = lv_obj_create(parent);
  lv_obj_add_style(panel, &style_panel, LV_PART_MAIN);
  lv_obj_set_scrollbar_mode(panel, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
  return panel;
}

lv_obj_t* createSmallButton(lv_obj_t* parent, const char* text, lv_event_cb_t cb, void* user_data) {
  lv_obj_t* button = lv_btn_create(parent);
  lv_obj_add_style(button, &style_button, LV_PART_MAIN);
  lv_obj_set_size(button, 78, 44);
  lv_obj_add_event_cb(button, cb, LV_EVENT_CLICKED, user_data);
  lv_obj_t* label = lv_label_create(button);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_center(label);
  return button;
}

void createUi() {
  initStyles();

  lv_obj_t* screen = lv_scr_act();
  lv_obj_add_style(screen, &style_screen, LV_PART_MAIN);
  lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* root = lv_obj_create(screen);
  lv_obj_remove_style_all(root);
  lv_obj_set_size(root, LV_PCT(100), LV_PCT(100));
  lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(root, 6, LV_PART_MAIN);
  lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* topbar = lv_obj_create(root);
  lv_obj_add_style(topbar, &style_topbar, LV_PART_MAIN);
  lv_obj_set_size(topbar, LV_PCT(100), 42);
  lv_obj_set_flex_flow(topbar, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(topbar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_scrollbar_mode(topbar, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(topbar, LV_OBJ_FLAG_SCROLLABLE);

  state_label = lv_label_create(topbar);
  lv_obj_set_width(state_label, 122);
  lv_label_set_text(state_label, "State: --");
  lv_obj_set_style_text_font(state_label, &lv_font_montserrat_18, LV_PART_MAIN);
  lv_obj_set_style_text_color(state_label, lv_color_hex(0x67E8F9), LV_PART_MAIN);

  lv_obj_t* right = lv_obj_create(topbar);
  lv_obj_remove_style_all(right);
  lv_obj_set_size(right, batteryAvailable() ? 164 : 124, 34);
  lv_obj_set_flex_flow(right, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(right, 10, LV_PART_MAIN);
  lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);

  status_dot = lv_obj_create(right);
  lv_obj_remove_style_all(status_dot);
  lv_obj_set_size(status_dot, 10, 10);
  lv_obj_set_style_radius(status_dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(status_dot, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(status_dot, lv_color_hex(0x38BDF8), LV_PART_MAIN);

  lv_obj_t* menu = lv_btn_create(right);
  lv_obj_add_style(menu, &style_button, LV_PART_MAIN);
  lv_obj_set_size(menu, 96, 32);
  lv_obj_add_event_cb(menu, onMenu, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* menu_label = lv_label_create(menu);
  lv_label_set_text(menu_label, "Settings");
  lv_obj_center(menu_label);

  createBatteryIndicator(right);

  lv_obj_t* tabs = lv_tabview_create(root, LV_DIR_TOP, 30);
  lv_obj_set_size(tabs, LV_PCT(100), kScreenHeight - 42);
  lv_obj_set_style_bg_color(tabs, lv_color_hex(0x0B1014), LV_PART_MAIN);
  lv_obj_set_style_border_width(tabs, 0, LV_PART_MAIN);
  lv_obj_set_scrollbar_mode(tabs, LV_SCROLLBAR_MODE_OFF);

  lv_obj_t* tab_btns = lv_tabview_get_tab_btns(tabs);
  lv_obj_set_style_bg_color(tab_btns, lv_color_hex(0x111821), LV_PART_MAIN);
  lv_obj_set_style_text_color(tab_btns, lv_color_hex(0x9DB3C7), LV_PART_MAIN);
  lv_obj_set_style_text_color(tab_btns, lv_color_hex(0xFFFFFF), LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_bg_color(tab_btns, lv_color_hex(0x2563EB), LV_PART_ITEMS | LV_STATE_CHECKED);

  lv_obj_t* status_tab = lv_tabview_add_tab(tabs, "Status");
  lv_obj_t* jog_tab = lv_tabview_add_tab(tabs, "Jog");
  lv_obj_t* actions_tab = lv_tabview_add_tab(tabs, "Actions");
  createStatusTab(status_tab);
  createJogTab(jog_tab);
  createActionsTab(actions_tab);
  lv_tabview_set_act(tabs, 0, LV_ANIM_OFF);

  createMenuPanel(screen);
  createPairingSuccessPanel(screen);
}

void applyDro() {
  if (!pending_dro) {
    return;
  }
  pending_dro = false;

  char stateText[32];
  snprintf(stateText, sizeof(stateText), "State: %.11s", latest_dro.state[0] ? latest_dro.state : "--");
  lv_label_set_text(state_label, stateText);
  updateStatusLabels();
  lv_label_set_text(units_label, latest_dro.inch ? "Units: in" : "Units: mm");
  setStatus(lv_color_hex(0x34D399));
}
