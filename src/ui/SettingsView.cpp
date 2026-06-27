// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#include "SettingsView.h"

#include "../AppState.h"
#include "Ui.h"

namespace {

void hideMenu() {
  if (menu_panel) {
    lv_obj_add_flag(menu_panel, LV_OBJ_FLAG_HIDDEN);
  }
}

void hidePairingSuccess() {
  if (pairing_success_panel) {
    lv_obj_add_flag(pairing_success_panel, LV_OBJ_FLAG_HIDDEN);
  }
}

void onCloseMenu(lv_event_t*) {
  hideMenu();
}

void onPairingMainView(lv_event_t*) {
  hidePairingSuccess();
  hideMenu();
}

void onPair(lv_event_t*) {
  if (fluidnc.isPairing()) {
    fluidnc.cancelPairing();
    return;
  }
  fluidnc.startPairing();
}

void onForget(lv_event_t*) {
  fluidnc.forgetAllMachines();
  setStatus(lv_color_hex(0xF59E0B));
  if (peer_label) {
    lv_label_set_text(peer_label, "No paired machine");
  }
}

}  // namespace

void setPairButtonText(const char* text) {
  if (pair_label) {
    lv_label_set_text(pair_label, text);
  }
}

void updatePeerLabel() {
  FluidNCMachine machine;
  if (fluidnc.getMachine(0, machine)) {
    lv_label_set_text(peer_label, machine.label().c_str());
  } else {
    lv_label_set_text(peer_label, "No paired machine");
  }
}

void showPairingSuccess() {
  if (pairing_success_panel) {
    lv_obj_clear_flag(pairing_success_panel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(pairing_success_panel);
  }
}

void createMenuPanel(lv_obj_t* screen) {
  menu_panel = lv_obj_create(screen);
  lv_obj_add_style(menu_panel, &style_overlay, LV_PART_MAIN);
#if FLUIDMONITOR_ENABLE_SHUTDOWN
  lv_obj_set_size(menu_panel, 282, 232);
#else
  lv_obj_set_size(menu_panel, 282, 184);
#endif
  lv_obj_align(menu_panel, LV_ALIGN_CENTER, 0, 8);
  lv_obj_set_flex_flow(menu_panel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(menu_panel, 8, LV_PART_MAIN);
  lv_obj_set_scrollbar_mode(menu_panel, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(menu_panel, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(menu_panel, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t* header = lv_obj_create(menu_panel);
  lv_obj_remove_style_all(header);
  lv_obj_set_size(header, LV_PCT(100), 26);
  lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_t* title = lv_label_create(header);
  lv_label_set_text(title, "ESP-NOW Settings");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_16, LV_PART_MAIN);

  lv_obj_t* close = lv_btn_create(header);
  lv_obj_add_style(close, &style_button, LV_PART_MAIN);
  lv_obj_set_size(close, 40, 28);
  lv_obj_add_event_cb(close, onCloseMenu, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* close_label = lv_label_create(close);
  lv_label_set_text(close_label, "X");
  lv_obj_center(close_label);

  lv_obj_t* info = makePanel(menu_panel);
  lv_obj_set_size(info, LV_PCT(100), 58);
  lv_obj_set_flex_flow(info, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(info, 8, LV_PART_MAIN);
  lv_obj_set_style_pad_row(info, 4, LV_PART_MAIN);

  peer_label = lv_label_create(info);
  lv_label_set_text(peer_label, "No paired machine");
  lv_obj_add_style(peer_label, &style_muted, LV_PART_MAIN);
  lv_obj_set_width(peer_label, LV_PCT(100));
  lv_label_set_long_mode(peer_label, LV_LABEL_LONG_DOT);

  units_label = lv_label_create(info);
  lv_label_set_text(units_label, "Units: mm");
  lv_obj_add_style(units_label, &style_muted, LV_PART_MAIN);

  lv_obj_t* actions = lv_obj_create(menu_panel);
  lv_obj_remove_style_all(actions);
  lv_obj_set_size(actions, LV_PCT(100), 48);
  lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(actions, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  pair_button = lv_btn_create(actions);
  lv_obj_add_style(pair_button, &style_button, LV_PART_MAIN);
  lv_obj_set_size(pair_button, 124, 42);
  lv_obj_add_event_cb(pair_button, onPair, LV_EVENT_CLICKED, nullptr);
  pair_label = lv_label_create(pair_button);
  lv_label_set_text(pair_label, "Pair");
  lv_obj_center(pair_label);

  lv_obj_t* forget = lv_btn_create(actions);
  lv_obj_add_style(forget, &style_button, LV_PART_MAIN);
  lv_obj_set_size(forget, 124, 42);
  lv_obj_add_event_cb(forget, onForget, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* forget_label = lv_label_create(forget);
  lv_label_set_text(forget_label, "Forget");
  lv_obj_center(forget_label);

#if FLUIDMONITOR_ENABLE_SHUTDOWN
  lv_obj_t* shutdown = lv_btn_create(menu_panel);
  lv_obj_add_style(shutdown, &style_button, LV_PART_MAIN);
  lv_obj_set_size(shutdown, LV_PCT(100), 42);
  lv_obj_add_event_cb(shutdown, onShutdown, LV_EVENT_CLICKED, nullptr);

  lv_obj_t* shutdown_label = lv_label_create(shutdown);
  lv_label_set_text(shutdown_label, "Shutdown");
  lv_obj_set_style_text_font(shutdown_label, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_center(shutdown_label);
#endif
}

void createPairingSuccessPanel(lv_obj_t* screen) {
  pairing_success_panel = lv_obj_create(screen);
  lv_obj_add_style(pairing_success_panel, &style_overlay, LV_PART_MAIN);
  lv_obj_set_size(pairing_success_panel, 250, 122);
  lv_obj_align(pairing_success_panel, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_flex_flow(pairing_success_panel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(pairing_success_panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(pairing_success_panel, 12, LV_PART_MAIN);
  lv_obj_set_scrollbar_mode(pairing_success_panel, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(pairing_success_panel, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(pairing_success_panel, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t* title = lv_label_create(pairing_success_panel);
  lv_label_set_text(title, "Paired successfully");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, LV_PART_MAIN);
  lv_obj_set_style_text_color(title, lv_color_hex(0x34D399), LV_PART_MAIN);

  lv_obj_t* action = lv_btn_create(pairing_success_panel);
  lv_obj_add_style(action, &style_button, LV_PART_MAIN);
  lv_obj_set_size(action, 150, 42);
  lv_obj_add_event_cb(action, onPairingMainView, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* label = lv_label_create(action);
  lv_label_set_text(label, "Main View");
  lv_obj_center(label);
}
