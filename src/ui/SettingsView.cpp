// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#include "SettingsView.h"

#include "../AppState.h"
#include "../Display.h"
#include "Ui.h"

namespace {

constexpr uint8_t kBrightnessStep = 24;
constexpr uint8_t kBrightnessMin = 24;

void hidePairingSuccess() {
  if (pairing_success_panel) {
    lv_obj_add_flag(pairing_success_panel, LV_OBJ_FLAG_HIDDEN);
  }
}

void onPairingMainView(lv_event_t*) {
  hidePairingSuccess();
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

int brightnessPercent() {
  return (static_cast<int>(displayBrightness()) * 100 + 127) / 255;
}

void updateBrightnessLabel() {
  if (brightness_label) {
    lv_label_set_text_fmt(brightness_label, "%d%%", brightnessPercent());
  }
}

void onBrightnessDown(lv_event_t*) {
  int value = displayBrightness();
  value -= kBrightnessStep;
  if (value < kBrightnessMin) {
    value = kBrightnessMin;
  }
  setDisplayBrightness(static_cast<uint8_t>(value));
  updateBrightnessLabel();
}

void onBrightnessUp(lv_event_t*) {
  int value = displayBrightness();
  value += kBrightnessStep;
  if (value > 255) {
    value = 255;
  }
  setDisplayBrightness(static_cast<uint8_t>(value));
  updateBrightnessLabel();
}

lv_obj_t* orientation_value = nullptr;

void updateOrientationLabel() {
  if (orientation_value) {
    lv_label_set_text(orientation_value,
                      displayFlipped() ? LV_SYMBOL_LOOP "  Flipped 180" : LV_SYMBOL_LOOP "  Normal");
  }
}

void onToggleOrientation(lv_event_t*) {
  setDisplayFlipped(!displayFlipped());
  updateOrientationLabel();
}

lv_obj_t* inactivity_segments[3] = {};

void updateInactivitySegments() {
  for (uint8_t i = 0; i < 3; ++i) {
    if (!inactivity_segments[i]) {
      continue;
    }
    bool active = (i == inactivityMode());
    lv_obj_set_style_bg_color(inactivity_segments[i], lv_color_hex(active ? 0x2563EB : 0x1B2430), LV_PART_MAIN);
    lv_obj_set_style_text_color(inactivity_segments[i], lv_color_hex(active ? 0xFFFFFF : 0x93A4B7), LV_PART_MAIN);
  }
}

void onInactivitySelect(lv_event_t* event) {
  uint8_t mode = static_cast<uint8_t>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(event)));
  setInactivityMode(mode);
  updateInactivitySegments();
}

void createInactivitySegment(lv_obj_t* parent, uint8_t index, const char* text, bool divider) {
  lv_obj_t* segment = lv_btn_create(parent);
  lv_obj_remove_style_all(segment);
  lv_obj_set_size(segment, 0, LV_PCT(100));
  lv_obj_set_flex_grow(segment, 1);
  lv_obj_set_style_bg_opa(segment, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_radius(segment, 0, LV_PART_MAIN);
  if (divider) {
    lv_obj_set_style_border_width(segment, 1, LV_PART_MAIN);
    lv_obj_set_style_border_side(segment, LV_BORDER_SIDE_LEFT, LV_PART_MAIN);
    lv_obj_set_style_border_color(segment, lv_color_hex(0x344151), LV_PART_MAIN);
  }
  lv_obj_add_event_cb(segment, onInactivitySelect, LV_EVENT_CLICKED, reinterpret_cast<void*>(static_cast<uintptr_t>(index)));

  lv_obj_t* label = lv_label_create(segment);
  lv_label_set_text(label, text);
  lv_obj_center(label);
  inactivity_segments[index] = segment;
}

lv_obj_t* createSection(lv_obj_t* parent, const char* icon, const char* title) {
  lv_obj_t* card = makePanel(parent);
  lv_obj_set_size(card, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(card, 8, LV_PART_MAIN);
  lv_obj_set_style_pad_row(card, 6, LV_PART_MAIN);

  lv_obj_t* heading = lv_label_create(card);
  lv_label_set_text_fmt(heading, "%s  %s", icon, title);
  lv_obj_set_style_text_font(heading, &lv_font_montserrat_14, LV_PART_MAIN);
  lv_obj_set_style_text_color(heading, lv_color_hex(0x67E8F9), LV_PART_MAIN);
  return card;
}

lv_obj_t* createStepButton(lv_obj_t* parent, const char* symbol, lv_event_cb_t cb) {
  lv_obj_t* button = lv_btn_create(parent);
  lv_obj_add_style(button, &style_button, LV_PART_MAIN);
  lv_obj_set_size(button, 42, 34);
  lv_obj_add_event_cb(button, cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* label = lv_label_create(button);
  lv_label_set_text(label, symbol);
  lv_obj_center(label);
  return button;
}

}  // namespace

void setPairButtonText(const char* text) {
  if (pair_label) {
    lv_label_set_text(pair_label, text);
  }
}

void updatePeerLabel() {
  if (!peer_label) {
    return;
  }
  FluidNCMachine machine;
  if (fluidnc.getMachine(0, machine)) {
    lv_label_set_text_fmt(peer_label, "%s", machine.label().c_str());
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

void createSettingsTab(lv_obj_t* tab) {
  lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(tab, 8, LV_PART_MAIN);
  lv_obj_set_style_pad_row(tab, 8, LV_PART_MAIN);
  lv_obj_set_scroll_dir(tab, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(tab, LV_SCROLLBAR_MODE_OFF);

  lv_obj_t* connection = createSection(tab, LV_SYMBOL_WIFI, "Connection");

  peer_label = lv_label_create(connection);
  lv_obj_add_style(peer_label, &style_muted, LV_PART_MAIN);
  lv_obj_set_style_text_font(peer_label, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_set_style_text_color(peer_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_width(peer_label, LV_PCT(100));
  lv_label_set_long_mode(peer_label, LV_LABEL_LONG_DOT);
  lv_label_set_text(peer_label, "No paired machine");

  lv_obj_t* conn_buttons = lv_obj_create(connection);
  lv_obj_remove_style_all(conn_buttons);
  lv_obj_set_size(conn_buttons, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(conn_buttons, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(conn_buttons, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(conn_buttons, LV_OBJ_FLAG_SCROLLABLE);

  pair_button = lv_btn_create(conn_buttons);
  lv_obj_add_style(pair_button, &style_button, LV_PART_MAIN);
  lv_obj_set_size(pair_button, 138, 40);
  lv_obj_add_event_cb(pair_button, onPair, LV_EVENT_CLICKED, nullptr);
  pair_label = lv_label_create(pair_button);
  lv_label_set_text(pair_label, LV_SYMBOL_BLUETOOTH "  Pair");
  lv_obj_center(pair_label);

  lv_obj_t* forget = lv_btn_create(conn_buttons);
  lv_obj_add_style(forget, &style_button, LV_PART_MAIN);
  lv_obj_set_size(forget, 138, 40);
  lv_obj_add_event_cb(forget, onForget, LV_EVENT_CLICKED, nullptr);
  lv_obj_t* forget_label = lv_label_create(forget);
  lv_label_set_text(forget_label, LV_SYMBOL_TRASH "  Forget");
  lv_obj_center(forget_label);

  lv_obj_t* display = createSection(tab, LV_SYMBOL_EYE_OPEN, "Display");

  lv_obj_t* brightness_row = lv_obj_create(display);
  lv_obj_remove_style_all(brightness_row);
  lv_obj_set_size(brightness_row, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(brightness_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(brightness_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(brightness_row, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* brightness_caption = lv_label_create(brightness_row);
  lv_label_set_text(brightness_caption, "Brightness");
  lv_obj_add_style(brightness_caption, &style_muted, LV_PART_MAIN);
  lv_obj_set_style_text_font(brightness_caption, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_set_style_text_color(brightness_caption, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

  lv_obj_t* brightness_controls = lv_obj_create(brightness_row);
  lv_obj_remove_style_all(brightness_controls);
  lv_obj_set_size(brightness_controls, 158, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(brightness_controls, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(brightness_controls, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(brightness_controls, 8, LV_PART_MAIN);
  lv_obj_clear_flag(brightness_controls, LV_OBJ_FLAG_SCROLLABLE);

  createStepButton(brightness_controls, LV_SYMBOL_MINUS, onBrightnessDown);

  brightness_label = lv_label_create(brightness_controls);
  lv_obj_set_width(brightness_label, 48);
  lv_obj_set_style_text_align(brightness_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_text_font(brightness_label, &lv_font_montserrat_16, LV_PART_MAIN);
  updateBrightnessLabel();

  createStepButton(brightness_controls, LV_SYMBOL_PLUS, onBrightnessUp);

  lv_obj_t* orientation_row = lv_obj_create(display);
  lv_obj_remove_style_all(orientation_row);
  lv_obj_set_size(orientation_row, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(orientation_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(orientation_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(orientation_row, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* orientation_caption = lv_label_create(orientation_row);
  lv_label_set_text(orientation_caption, "Orientation");
  lv_obj_add_style(orientation_caption, &style_muted, LV_PART_MAIN);
  lv_obj_set_style_text_font(orientation_caption, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_set_style_text_color(orientation_caption, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

  lv_obj_t* orientation_button = lv_btn_create(orientation_row);
  lv_obj_add_style(orientation_button, &style_button, LV_PART_MAIN);
  lv_obj_set_size(orientation_button, 158, 34);
  lv_obj_add_event_cb(orientation_button, onToggleOrientation, LV_EVENT_CLICKED, nullptr);
  orientation_value = lv_label_create(orientation_button);
  lv_obj_center(orientation_value);
  updateOrientationLabel();

  lv_obj_t* inactivity_row = lv_obj_create(display);
  lv_obj_remove_style_all(inactivity_row);
  lv_obj_set_size(inactivity_row, LV_PCT(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(inactivity_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(inactivity_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(inactivity_row, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* inactivity_caption = lv_label_create(inactivity_row);
  lv_label_set_text(inactivity_caption, "Inactivity");
  lv_obj_add_style(inactivity_caption, &style_muted, LV_PART_MAIN);
  lv_obj_set_style_text_font(inactivity_caption, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_set_style_text_color(inactivity_caption, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

  lv_obj_t* inactivity_selector = lv_obj_create(inactivity_row);
  lv_obj_remove_style_all(inactivity_selector);
  lv_obj_set_size(inactivity_selector, 168, 34);
  lv_obj_set_flex_flow(inactivity_selector, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_radius(inactivity_selector, 8, LV_PART_MAIN);
  lv_obj_set_style_clip_corner(inactivity_selector, true, LV_PART_MAIN);
  lv_obj_set_style_border_width(inactivity_selector, 1, LV_PART_MAIN);
  lv_obj_set_style_border_color(inactivity_selector, lv_color_hex(0x344151), LV_PART_MAIN);
  lv_obj_set_style_pad_all(inactivity_selector, 0, LV_PART_MAIN);
  lv_obj_clear_flag(inactivity_selector, LV_OBJ_FLAG_SCROLLABLE);

  createInactivitySegment(inactivity_selector, kInactivityDisplayOn, "On", false);
  createInactivitySegment(inactivity_selector, kInactivityDim, "Dim", true);
  createInactivitySegment(inactivity_selector, kInactivityDisplayOff, "Off", true);
  updateInactivitySegments();

  lv_obj_t* about = createSection(tab, LV_SYMBOL_GPS, "About");

  units_label = lv_label_create(about);
  lv_label_set_text(units_label, "Units: mm");
  lv_obj_add_style(units_label, &style_muted, LV_PART_MAIN);
  lv_obj_set_style_text_font(units_label, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_set_style_text_color(units_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

  lv_obj_t* about_app = lv_label_create(about);
  lv_label_set_text(about_app, "FluidMonitor " LV_SYMBOL_BULLET " ESP-NOW DRO");
  lv_obj_add_style(about_app, &style_muted, LV_PART_MAIN);

#if FLUIDMONITOR_ENABLE_SHUTDOWN
  lv_obj_t* shutdown = lv_btn_create(tab);
  lv_obj_add_style(shutdown, &style_button, LV_PART_MAIN);
  lv_obj_set_size(shutdown, LV_PCT(100), 42);
  lv_obj_set_style_bg_color(shutdown, lv_color_hex(0x7F1D1D), LV_PART_MAIN);
  lv_obj_set_style_border_color(shutdown, lv_color_hex(0xB91C1C), LV_PART_MAIN);
  lv_obj_add_event_cb(shutdown, onShutdown, LV_EVENT_CLICKED, nullptr);

  lv_obj_t* shutdown_label = lv_label_create(shutdown);
  lv_label_set_text(shutdown_label, LV_SYMBOL_POWER "  Shutdown");
  lv_obj_set_style_text_font(shutdown_label, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_center(shutdown_label);
#endif

  updatePeerLabel();
}

void createPairingSuccessPanel(lv_obj_t* screen) {
  pairing_success_panel = lv_obj_create(screen);
  lv_obj_add_style(pairing_success_panel, &style_overlay, LV_PART_MAIN);
  lv_obj_set_size(pairing_success_panel, 250, 130);
  lv_obj_align(pairing_success_panel, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_flex_flow(pairing_success_panel, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(pairing_success_panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(pairing_success_panel, 12, LV_PART_MAIN);
  lv_obj_set_scrollbar_mode(pairing_success_panel, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(pairing_success_panel, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(pairing_success_panel, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t* title = lv_label_create(pairing_success_panel);
  lv_label_set_text(title, LV_SYMBOL_OK "  Paired successfully");
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
