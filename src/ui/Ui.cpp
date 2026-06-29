// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#include "Ui.h"

#include <Arduino.h>
#include <stdio.h>

#include "../AppState.h"
#include "../BatteryMonitor.h"
#include "../Colors.h"
#include "app_config.h"
#include "ActionsView.h"
#include "JogView.h"
#include "SettingsView.h"
#include "StatusView.h"

namespace {

#if FLUIDMONITOR_ENABLE_SHUTDOWN
constexpr uint8_t kShutdownCountdownSteps = 10;

lv_obj_t* shutdown_overlay = nullptr;
lv_obj_t* shutdown_countdown_label = nullptr;
lv_timer_t* shutdown_timer = nullptr;
uint32_t shutdown_started_ms = 0;
bool shutdown_button_last_read_pressed = false;
uint32_t shutdown_last_tap_ms = 0;
uint32_t shutdown_last_press_edge_ms = 0;
uint32_t shutdown_last_debug_status_ms = 0;

void startShutdownUi();

#if FLUIDMONITOR_SHUTDOWN_DEBUG
const char* shutdownLevelName(bool pressed) {
  return pressed ? "LOW/pressed" : "HIGH/released";
}
#endif

void releaseShutdownKey() {
#if FLUIDMONITOR_SHUTDOWN_PULLUP
  pinMode(FLUIDMONITOR_SHUTDOWN_GPIO, INPUT_PULLUP);
#else
  pinMode(FLUIDMONITOR_SHUTDOWN_GPIO, INPUT);
#endif
#if FLUIDMONITOR_SHUTDOWN_DEBUG
  Serial.printf("Shutdown GPIO %d released to %s\n",
                FLUIDMONITOR_SHUTDOWN_GPIO,
                FLUIDMONITOR_SHUTDOWN_PULLUP ? "INPUT_PULLUP" : "INPUT");
#endif
}

void holdShutdownKey() {
#if FLUIDMONITOR_SHUTDOWN_DEBUG
  Serial.printf("Shutdown GPIO %d driven LOW with OUTPUT_OPEN_DRAIN\n", FLUIDMONITOR_SHUTDOWN_GPIO);
#endif
  digitalWrite(FLUIDMONITOR_SHUTDOWN_GPIO, LOW);
  pinMode(FLUIDMONITOR_SHUTDOWN_GPIO, OUTPUT_OPEN_DRAIN);
}

bool readShutdownButtonPressed() {
  return digitalRead(FLUIDMONITOR_SHUTDOWN_GPIO) == LOW;
}

void syncShutdownButtonState(const char* reason) {
  const bool pressed = readShutdownButtonPressed();
  shutdown_button_last_read_pressed = pressed;
  const uint32_t now = millis();
  shutdown_last_tap_ms = 0;
  shutdown_last_press_edge_ms = 0;
  shutdown_last_debug_status_ms = now;
#if FLUIDMONITOR_SHUTDOWN_DEBUG
  Serial.printf("Shutdown GPIO state sync (%s): %s\n", reason, shutdownLevelName(pressed));
#endif
}

void handleShutdownTap(uint32_t now) {
  if (shutdown_last_press_edge_ms != 0 &&
      now - shutdown_last_press_edge_ms < FLUIDMONITOR_SHUTDOWN_TAP_LOCKOUT_MS) {
#if FLUIDMONITOR_SHUTDOWN_DEBUG
    Serial.printf("Shutdown GPIO press pulse ignored: lockout delta=%ums\n",
                  now - shutdown_last_press_edge_ms);
#endif
    shutdown_last_press_edge_ms = now;
    return;
  }

  shutdown_last_press_edge_ms = now;

  if (shutdown_last_tap_ms != 0 &&
      now - shutdown_last_tap_ms <= FLUIDMONITOR_SHUTDOWN_DOUBLE_TAP_MS) {
#if FLUIDMONITOR_SHUTDOWN_DEBUG
    Serial.printf("Shutdown GPIO double tap detected: delta=%ums\n", now - shutdown_last_tap_ms);
#endif
    shutdown_last_tap_ms = 0;
    startShutdownUi();
    return;
  }

  shutdown_last_tap_ms = now;
#if FLUIDMONITOR_SHUTDOWN_DEBUG
  Serial.printf("Shutdown GPIO first tap at %ums\n", now);
#endif
}

void closeShutdownOverlay() {
  if (shutdown_timer) {
    lv_timer_del(shutdown_timer);
    shutdown_timer = nullptr;
  }
  if (shutdown_overlay) {
    lv_obj_del(shutdown_overlay);
    shutdown_overlay = nullptr;
  }
  shutdown_countdown_label = nullptr;
}

void abortShutdown(lv_event_t*) {
  releaseShutdownKey();
  delay(5);
  syncShutdownButtonState("abort");
  closeShutdownOverlay();
}

void updateShutdownCountdown(lv_timer_t*) {
  if (!shutdown_countdown_label) {
    return;
  }

  const uint32_t elapsed_ms = millis() - shutdown_started_ms;
  const uint32_t step_ms = FLUIDMONITOR_SHUTDOWN_HOLD_MS / kShutdownCountdownSteps;
  if (elapsed_ms < FLUIDMONITOR_SHUTDOWN_HOLD_MS) {
    uint8_t count = (FLUIDMONITOR_SHUTDOWN_HOLD_MS - elapsed_ms + step_ms - 1) / step_ms;
    if (count > kShutdownCountdownSteps) {
      count = kShutdownCountdownSteps;
    }
    if (count < 1) {
      count = 1;
    }
    lv_label_set_text_fmt(shutdown_countdown_label, "%u", count);
    return;
  }

  lv_label_set_text(shutdown_countdown_label, "...");
}

void startShutdownUi() {
  if (shutdown_overlay) {
    return;
  }

  shutdown_started_ms = millis();
  holdShutdownKey();

  shutdown_overlay = lv_obj_create(lv_scr_act());
  lv_obj_add_style(shutdown_overlay, &style_overlay, LV_PART_MAIN);
  lv_obj_set_size(shutdown_overlay, 250, 176);
  lv_obj_align(shutdown_overlay, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_flex_flow(shutdown_overlay, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(shutdown_overlay, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_row(shutdown_overlay, 12, LV_PART_MAIN);
  lv_obj_set_scrollbar_mode(shutdown_overlay, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(shutdown_overlay, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_move_foreground(shutdown_overlay);

  lv_obj_t* title = lv_label_create(shutdown_overlay);
  lv_label_set_text(title, "Shutting down");
  lv_obj_set_style_text_color(title, lv_color_hex(Colors::kStatusInfo), LV_PART_MAIN);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, LV_PART_MAIN);

  shutdown_countdown_label = lv_label_create(shutdown_overlay);
  lv_label_set_text(shutdown_countdown_label, "10");
  lv_obj_set_style_text_color(shutdown_countdown_label, lv_color_hex(Colors::kTextPrimary), LV_PART_MAIN);
  lv_obj_set_style_text_font(shutdown_countdown_label, &lv_font_montserrat_32, LV_PART_MAIN);
  lv_obj_set_style_text_align(shutdown_countdown_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_size(shutdown_countdown_label, LV_PCT(100), 42);

  lv_obj_t* abort = lv_btn_create(shutdown_overlay);
  lv_obj_add_style(abort, &style_button, LV_PART_MAIN);
  lv_obj_set_size(abort, 120, 40);
  lv_obj_add_event_cb(abort, abortShutdown, LV_EVENT_CLICKED, nullptr);

  lv_obj_t* abort_label = lv_label_create(abort);
  lv_label_set_text(abort_label, "Abort");
  lv_obj_center(abort_label);

  shutdown_timer = lv_timer_create(updateShutdownCountdown, 100, nullptr);
  updateShutdownCountdown(nullptr);
}
#endif

void initStyles() {
  lv_style_init(&style_screen);
  lv_style_set_bg_color(&style_screen, lv_color_hex(Colors::kBg));
  lv_style_set_text_color(&style_screen, lv_color_hex(Colors::kText));

  lv_style_init(&style_topbar);
  lv_style_set_bg_color(&style_topbar, lv_color_hex(Colors::kBgTopbar));
  lv_style_set_bg_opa(&style_topbar, LV_OPA_COVER);
  lv_style_set_border_width(&style_topbar, 0);
  lv_style_set_pad_left(&style_topbar, 10);
  lv_style_set_pad_right(&style_topbar, 10);

  lv_style_init(&style_panel);
  lv_style_set_bg_color(&style_panel, lv_color_hex(Colors::kBgPanel));
  lv_style_set_bg_opa(&style_panel, LV_OPA_COVER);
  lv_style_set_border_width(&style_panel, 1);
  lv_style_set_border_color(&style_panel, lv_color_hex(Colors::kBorder));
  lv_style_set_radius(&style_panel, 8);

  lv_style_init(&style_muted);
  lv_style_set_text_color(&style_muted, lv_color_hex(Colors::kTextMuted));
  lv_style_set_text_font(&style_muted, &lv_font_montserrat_12);

  lv_style_init(&style_button);
  lv_style_set_bg_color(&style_button, lv_color_hex(Colors::kBgButton));
  lv_style_set_bg_opa(&style_button, LV_OPA_COVER);
  lv_style_set_border_width(&style_button, 1);
  lv_style_set_border_color(&style_button, lv_color_hex(Colors::kBorderButton));
  lv_style_set_radius(&style_button, 7);
  lv_style_set_shadow_width(&style_button, 0);
  lv_style_set_text_color(&style_button, lv_color_hex(Colors::kTextPrimary));

  lv_style_init(&style_overlay);
  lv_style_set_bg_color(&style_overlay, lv_color_hex(Colors::kBgOverlay));
  lv_style_set_bg_opa(&style_overlay, LV_OPA_COVER);
  lv_style_set_border_width(&style_overlay, 1);
  lv_style_set_border_color(&style_overlay, lv_color_hex(Colors::kBorderOverlay));
  lv_style_set_radius(&style_overlay, 8);
  lv_style_set_pad_all(&style_overlay, 10);
}

lv_obj_t* battery_indicator = nullptr;
lv_obj_t* battery_fill = nullptr;
lv_obj_t* battery_charge = nullptr;
lv_obj_t* topbar_dro = nullptr;
lv_obj_t* topbar_dro_x = nullptr;
lv_obj_t* topbar_dro_y = nullptr;
lv_obj_t* topbar_dro_z = nullptr;
bool topbar_jog_mode = false;

constexpr uint8_t kJogTabIndex = 1;

lv_color_t batteryColor(int level) {
  if (level >= 50) {
    return lv_color_hex(Colors::kBatteryOk);
  }
  if (level >= 25) {
    return lv_color_hex(Colors::kBatteryWarning);
  }
  return lv_color_hex(Colors::kBatteryLow);
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
  lv_obj_set_size(battery_indicator, 30, 18);
  lv_obj_clear_flag(battery_indicator, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* body = lv_obj_create(battery_indicator);
  lv_obj_remove_style_all(body);
  lv_obj_set_size(body, 22, 14);
  lv_obj_align(body, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_set_style_border_width(body, 1, LV_PART_MAIN);
  lv_obj_set_style_border_color(body, lv_color_hex(Colors::kTextPrimary), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(body, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);

  battery_fill = lv_obj_create(body);
  lv_obj_remove_style_all(battery_fill);
  lv_obj_set_size(battery_fill, 3, 10);
  lv_obj_align(battery_fill, LV_ALIGN_TOP_LEFT, 1, 1);
  lv_obj_set_style_bg_opa(battery_fill, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(battery_fill, lv_color_hex(Colors::kBatteryLow), LV_PART_MAIN);
  lv_obj_clear_flag(battery_fill, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* nub = lv_obj_create(battery_indicator);
  lv_obj_remove_style_all(nub);
  lv_obj_set_size(nub, 4, 6);
  lv_obj_align(nub, LV_ALIGN_LEFT_MID, 22, 0);
  lv_obj_set_style_bg_opa(nub, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(nub, lv_color_hex(Colors::kTextPrimary), LV_PART_MAIN);
  lv_obj_clear_flag(nub, LV_OBJ_FLAG_SCROLLABLE);

  battery_charge = lv_label_create(battery_indicator);
  lv_label_set_text(battery_charge, LV_SYMBOL_CHARGE);
  lv_obj_set_style_text_font(battery_charge, &lv_font_montserrat_10, LV_PART_MAIN);
  lv_obj_set_style_text_color(battery_charge, lv_color_hex(Colors::kBatteryOk), LV_PART_MAIN);
  lv_obj_align(battery_charge, LV_ALIGN_CENTER, -4, 0);

  updateBatteryIndicator();
  lv_timer_create(updateBatteryTimer, 1000, nullptr);
}

void formatTopbarDroAxis(lv_obj_t* label, char axis, float value) {
  if (!label) {
    return;
  }

  char text[16];
  snprintf(text, sizeof(text), "%c%8.3f", axis, value);
  lv_label_set_text(label, text);
}

void updateTopbarDro() {
  if (!latest_dro.work.valid) {
    return;
  }

  formatTopbarDroAxis(topbar_dro_x, 'X', latest_dro.work.x);
  formatTopbarDroAxis(topbar_dro_y, 'Y', latest_dro.work.y);
  formatTopbarDroAxis(topbar_dro_z, 'Z', latest_dro.work.z);
}

void setTopbarJogMode(bool show_dro) {
  if (topbar_jog_mode == show_dro) {
    return;
  }
  topbar_jog_mode = show_dro;

  if (state_label) {
    if (show_dro) {
      lv_obj_add_flag(state_label, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_clear_flag(state_label, LV_OBJ_FLAG_HIDDEN);
    }
  }

  if (topbar_dro) {
    if (show_dro) {
      updateTopbarDro();
      lv_obj_clear_flag(topbar_dro, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(topbar_dro, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

void onTabChanged(lv_event_t* event) {
  if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) {
    return;
  }

  lv_obj_t* tabs = lv_event_get_target(event);
  setTopbarJogMode(lv_tabview_get_tab_act(tabs) == kJogTabIndex);
}

void onTabButtonChanged(lv_event_t* event) {
  if (lv_event_get_code(event) != LV_EVENT_VALUE_CHANGED) {
    return;
  }

  uint32_t selected = LV_BTNMATRIX_BTN_NONE;
  uint32_t* param = static_cast<uint32_t*>(lv_event_get_param(event));
  if (param) {
    selected = *param;
  } else {
    selected = lv_btnmatrix_get_selected_btn(lv_event_get_target(event));
  }
  setTopbarJogMode(selected == kJogTabIndex);
}

void syncTopbarTabTimer(lv_timer_t* timer) {
  lv_obj_t* tabs = static_cast<lv_obj_t*>(timer->user_data);
  if (!tabs) {
    return;
  }
  setTopbarJogMode(lv_tabview_get_tab_act(tabs) == kJogTabIndex);
}

lv_obj_t* createTopbarDroAxis(lv_obj_t* parent, char axis) {
  lv_obj_t* label = lv_label_create(parent);
  lv_obj_set_width(label, 70);
  lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN);
  lv_obj_set_style_text_color(label, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
  formatTopbarDroAxis(label, axis, 0);
  return label;
}

}  // namespace

void initShutdownControl() {
#if FLUIDMONITOR_ENABLE_SHUTDOWN
  releaseShutdownKey();
  delay(5);
  syncShutdownButtonState("init");
#if FLUIDMONITOR_SHUTDOWN_DEBUG
  Serial.printf("Shutdown GPIO debug: gpio=%d active=LOW pullup=%u lockout=%ums double_tap=%ums hold=%ums\n",
                FLUIDMONITOR_SHUTDOWN_GPIO,
                FLUIDMONITOR_SHUTDOWN_PULLUP,
                FLUIDMONITOR_SHUTDOWN_TAP_LOCKOUT_MS,
                FLUIDMONITOR_SHUTDOWN_DOUBLE_TAP_MS,
                FLUIDMONITOR_SHUTDOWN_HOLD_MS);
#endif
#endif
}

void pollShutdownControl() {
#if FLUIDMONITOR_ENABLE_SHUTDOWN
  if (shutdown_overlay) {
    return;
  }

  const uint32_t now = millis();
  const bool pressed = readShutdownButtonPressed();
#if FLUIDMONITOR_SHUTDOWN_DEBUG
  if (now - shutdown_last_debug_status_ms >= FLUIDMONITOR_SHUTDOWN_DEBUG_STATUS_MS) {
    shutdown_last_debug_status_ms = now;
    const uint32_t tap_age = shutdown_last_tap_ms == 0 ? 0 : now - shutdown_last_tap_ms;
    Serial.printf("Shutdown GPIO status: raw=%s last_tap_age=%ums\n",
                  shutdownLevelName(pressed),
                  tap_age);
  }
#endif

  if (pressed != shutdown_button_last_read_pressed) {
    shutdown_button_last_read_pressed = pressed;
#if FLUIDMONITOR_SHUTDOWN_DEBUG
    Serial.printf("Shutdown GPIO raw transition: %s at %ums\n",
                  shutdownLevelName(pressed),
                  now);
#endif
    if (pressed) {
      handleShutdownTap(now);
    }
    return;
  }
#endif
}

void onShutdown(lv_event_t*) {
#if FLUIDMONITOR_ENABLE_SHUTDOWN
  startShutdownUi();
#endif
}

void setStatus(lv_color_t color) {
  if (status_dot) {
    lv_obj_set_style_bg_color(status_dot, color, LV_PART_MAIN);
  }
}

void setStateLabel(const char* text) {
  if (state_label) {
    lv_label_set_text(state_label, text);
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

  int fill_w = 3;
  if (level >= 75) {
    fill_w = 18;
  } else if (level >= 50) {
    fill_w = 11;
  } else if (level >= 25) {
    fill_w = 7;
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

static void addPressFeedback(lv_obj_t* button) {
  lv_obj_set_style_bg_color(button, lv_color_hex(Colors::kBgButtonPressed), LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_transform_width(button, -2, LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_transform_height(button, -2, LV_PART_MAIN | LV_STATE_PRESSED);
}

void accentButton(lv_obj_t* button, lv_color_t color) {
  lv_obj_set_style_text_color(button, color, LV_PART_MAIN);
  lv_obj_set_style_border_color(button, color, LV_PART_MAIN);
  lv_obj_set_style_border_width(button, 2, LV_PART_MAIN);
}

lv_obj_t* createSmallButton(lv_obj_t* parent, const char* text, lv_event_cb_t cb, void* user_data) {
  lv_obj_t* button = lv_btn_create(parent);
  lv_obj_add_style(button, &style_button, LV_PART_MAIN);
  lv_obj_set_size(button, 78, 44);
  addPressFeedback(button);
  lv_obj_add_event_cb(button, cb, LV_EVENT_CLICKED, user_data);
  lv_obj_t* label = lv_label_create(button);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_16, LV_PART_MAIN);
  lv_obj_center(label);
  return button;
}

static lv_obj_t* makeRowLabel(lv_obj_t* row, const char* text) {
  lv_obj_t* label = lv_label_create(row);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_16, LV_PART_MAIN);
  return label;
}

lv_obj_t* createZeroButton(lv_obj_t* parent, const char* leading, const char* trailing, lv_event_cb_t cb,
                           void* user_data) {
  lv_obj_t* button = lv_btn_create(parent);
  lv_obj_add_style(button, &style_button, LV_PART_MAIN);
  lv_obj_set_size(button, 78, 44);
  addPressFeedback(button);
  lv_obj_add_event_cb(button, cb, LV_EVENT_CLICKED, user_data);

  lv_obj_t* row = lv_obj_create(button);
  lv_obj_remove_style_all(row);
  lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_center(row);

  if (leading) {
    makeRowLabel(row, leading);
  }

  // slashed zero: a "/" laid over a "0"
  lv_obj_t* zero_box = lv_obj_create(row);
  lv_obj_remove_style_all(zero_box);
  lv_obj_set_size(zero_box, 11, 20);
  lv_obj_clear_flag(zero_box, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_center(makeRowLabel(zero_box, "0"));
  lv_obj_center(makeRowLabel(zero_box, "/"));

  makeRowLabel(row, trailing);
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

  lv_obj_t* left = lv_obj_create(topbar);
  lv_obj_remove_style_all(left);
  lv_obj_set_size(left, 238, 34);
  lv_obj_set_flex_flow(left, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(left, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(left, 6, LV_PART_MAIN);
  lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);

  status_dot = lv_obj_create(left);
  lv_obj_remove_style_all(status_dot);
  lv_obj_set_size(status_dot, 12, 12);
  lv_obj_set_style_radius(status_dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(status_dot, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(status_dot, lv_color_hex(0x38BDF8), LV_PART_MAIN);

  state_label = lv_label_create(left);
  lv_label_set_text(state_label, "Disconnected");
  lv_obj_set_style_text_font(state_label, &lv_font_montserrat_18, LV_PART_MAIN);
  lv_obj_set_style_text_color(state_label, lv_color_hex(Colors::kTextCyan), LV_PART_MAIN);

  topbar_dro = lv_obj_create(left);
  lv_obj_remove_style_all(topbar_dro);
  lv_obj_set_size(topbar_dro, 220, 18);
  lv_obj_set_flex_flow(topbar_dro, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(topbar_dro, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(topbar_dro, 4, LV_PART_MAIN);
  lv_obj_clear_flag(topbar_dro, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(topbar_dro, LV_OBJ_FLAG_HIDDEN);

  topbar_dro_x = createTopbarDroAxis(topbar_dro, 'X');
  topbar_dro_y = createTopbarDroAxis(topbar_dro, 'Y');
  topbar_dro_z = createTopbarDroAxis(topbar_dro, 'Z');

  lv_obj_t* right = lv_obj_create(topbar);
  lv_obj_remove_style_all(right);
  lv_obj_set_size(right, batteryAvailable() ? 62 : 12, 34);
  lv_obj_set_flex_flow(right, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(right, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);

  createBatteryIndicator(right);

  lv_obj_t* tabs = lv_tabview_create(root, LV_DIR_TOP, 32);
  lv_obj_set_size(tabs, LV_PCT(100), kScreenHeight - 42);
  lv_obj_set_style_bg_color(tabs, lv_color_hex(Colors::kBgSecondary), LV_PART_MAIN);
  lv_obj_set_style_border_width(tabs, 0, LV_PART_MAIN);
  lv_obj_set_scrollbar_mode(tabs, LV_SCROLLBAR_MODE_OFF);
  lv_obj_add_event_cb(tabs, onTabChanged, LV_EVENT_VALUE_CHANGED, nullptr);

  lv_obj_t* tab_btns = lv_tabview_get_tab_btns(tabs);
  lv_obj_set_style_bg_color(tab_btns, lv_color_hex(Colors::kBgTertiary), LV_PART_MAIN);
  lv_obj_set_style_text_color(tab_btns, lv_color_hex(Colors::kTextSecondary), LV_PART_MAIN);
  lv_obj_set_style_text_color(tab_btns, lv_color_hex(Colors::kTextWhite), LV_PART_ITEMS | LV_STATE_CHECKED);
  lv_obj_set_style_bg_color(tab_btns, lv_color_hex(Colors::kBgAccent), LV_PART_ITEMS | LV_STATE_CHECKED);

  lv_obj_t* status_tab = lv_tabview_add_tab(tabs, LV_SYMBOL_GPS " Status");
  lv_obj_t* jog_tab = lv_tabview_add_tab(tabs, LV_SYMBOL_LOOP " Jog");
  lv_obj_t* actions_tab = lv_tabview_add_tab(tabs, LV_SYMBOL_LIST " Actions");
  lv_obj_t* settings_tab = lv_tabview_add_tab(tabs, LV_SYMBOL_SETTINGS " Settings");
  lv_obj_add_event_cb(tab_btns, onTabButtonChanged, LV_EVENT_VALUE_CHANGED, nullptr);
  createStatusTab(status_tab);
  createJogTab(jog_tab);
  createActionsTab(actions_tab);
  createSettingsTab(settings_tab);

  lv_btnmatrix_set_btn_width(tab_btns, 0, 8);   // Status
  lv_btnmatrix_set_btn_width(tab_btns, 1, 7);   // Jog
  lv_btnmatrix_set_btn_width(tab_btns, 2, 9);   // Actions
  lv_btnmatrix_set_btn_width(tab_btns, 3, 10);  // Settings

  lv_tabview_set_act(tabs, 0, LV_ANIM_OFF);
  lv_timer_create(syncTopbarTabTimer, 75, tabs);

  createPairingSuccessPanel(screen);
}

void applyDro() {
  const bool apply_dro = pending_dro;
  const bool apply_job_ui = pending_job_ui;
  if (!apply_dro && !apply_job_ui) {
    return;
  }

  if (apply_dro) {
    pending_dro = false;

    char stateText[32];
    snprintf(stateText, sizeof(stateText), "%.11s", latest_dro.state[0] ? latest_dro.state : "Disconnected");
    lv_label_set_text(state_label, stateText);
    updateStatusLabels();
    updateStatusJobControls();
    updateTopbarDro();
    lv_label_set_text(units_label, latest_dro.inch ? "Units: in" : "Units: mm");
    setStatus(lv_color_hex(0x34D399));
  }

  updateActionsJobStatus();
}
