// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#include "JogView.h"

#include "../AppState.h"
#include "../Colors.h"
#include "../Display.h"
#include "Ui.h"

namespace {

lv_obj_t* jog_lock_overlay = nullptr;

float selectedJogDistance() {
  static const float distances[] = {0.1f, 1.0f, 10.0f, 100.0f};
  return distances[selected_jog_step < 4 ? selected_jog_step : 1];
}

void updateJogStepButtons() {
  for (uint8_t i = 0; i < 4; ++i) {
    if (!jog_step_buttons[i]) {
      continue;
    }
    lv_obj_set_style_bg_color(jog_step_buttons[i],
                              lv_color_hex(i == selected_jog_step ? Colors::kBgAccent : Colors::kBgButton),
                              LV_PART_MAIN);
    lv_obj_set_style_text_color(jog_step_buttons[i],
                                lv_color_hex(i == selected_jog_step ? Colors::kTextWhite : Colors::kTextTertiary),
                                LV_PART_MAIN);
  }
}

void selectJogStep(uint8_t index) {
  if (index >= 4) {
    return;
  }
  selected_jog_step = index;
  updateJogStepButtons();
}

void onJogStep(lv_event_t* event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
    return;
  }
  selectJogStep(static_cast<uint8_t>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(event))));
}

void startJog(char axis, int direction) {
  if (!fluidnc.isConnected()) {
    setStatus(lv_color_hex(Colors::kStatusError));
    return;
  }
  if (!fluidnc.isIdle()) {
    setStatus(lv_color_hex(Colors::kStatusWarning));
    return;
  }
  cancelJog();

  const float feedrate = selectedJogDistance() * 300.0f;
  if (fluidnc.jogContinuous(axis, direction, feedrate)) {
    jog_active = true;
  }
}

constexpr uint32_t kJogStartDelayMs = 120;
lv_timer_t* jog_start_timer = nullptr;
char pending_jog_axis = 0;
int pending_jog_dir = 0;

void clearPendingJog() {
  if (jog_start_timer) {
    lv_timer_del(jog_start_timer);
    jog_start_timer = nullptr;
  }
}

void onJogStartTimer(lv_timer_t*) {
  jog_start_timer = nullptr;
  if (touchDragging()) {
    return;
  }
  startJog(pending_jog_axis, pending_jog_dir);
}

void onJogButton(lv_event_t* event) {
  const lv_event_code_t code = lv_event_get_code(event);
  const uintptr_t data = reinterpret_cast<uintptr_t>(lv_event_get_user_data(event));
  const char axis = static_cast<char>((data >> 8) & 0xff);
  const int direction = (data & 0xff) ? 1 : -1;

  // wait briefly so a swipe (which scrolls and drops the press) never starts motion
  if (code == LV_EVENT_PRESSED) {
    pending_jog_axis = axis;
    pending_jog_dir = direction;
    clearPendingJog();
    jog_start_timer = lv_timer_create(onJogStartTimer, kJogStartDelayMs, nullptr);
    lv_timer_set_repeat_count(jog_start_timer, 1);
  } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST || code == LV_EVENT_CANCEL) {
    clearPendingJog();
    cancelJog();
  }
}

lv_obj_t* createJogButton(lv_obj_t* parent, const char* text, char axis, int direction, lv_color_t color) {
  lv_obj_t* button = lv_btn_create(parent);
  lv_obj_add_style(button, &style_button, LV_PART_MAIN);
  lv_obj_set_size(button, 70, 48);
  lv_obj_set_style_bg_color(button, lv_color_hex(Colors::kBgInactive), LV_PART_MAIN);

  lv_obj_set_style_bg_color(button, lv_color_hex(Colors::kBgAccentPressed), LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_border_color(button, color, LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_border_width(button, 2, LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_outline_color(button, color, LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_outline_width(button, 3, LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_outline_opa(button, LV_OPA_50, LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_transform_width(button, -3, LV_PART_MAIN | LV_STATE_PRESSED);
  lv_obj_set_style_transform_height(button, -3, LV_PART_MAIN | LV_STATE_PRESSED);

  lv_obj_add_event_cb(button,
                      onJogButton,
                      LV_EVENT_ALL,
                      reinterpret_cast<void*>((static_cast<uintptr_t>(axis) << 8) | (direction > 0 ? 1 : 0)));

  lv_obj_t* label = lv_label_create(button);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_24, LV_PART_MAIN);
  lv_obj_set_style_text_color(label, color, LV_PART_MAIN);
  lv_obj_center(label);
  return button;
}

}  // namespace

void cancelJog() {
  if (jog_active || fluidnc.isConnected()) {
    fluidnc.jogCancel();
  }
  jog_active = false;
}

void createJogTab(lv_obj_t* tab) {
  lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(tab, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_row(tab, 8, LV_PART_MAIN);
  lv_obj_set_scrollbar_mode(tab, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(tab, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* step_row = lv_obj_create(tab);
  lv_obj_remove_style_all(step_row);
  lv_obj_set_size(step_row, LV_PCT(100), 34);
  lv_obj_set_flex_flow(step_row, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(step_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(step_row, LV_OBJ_FLAG_SCROLLABLE);

  static const char* labels[] = {"0.1", "1", "10", "100"};
  for (uint8_t i = 0; i < 4; ++i) {
    jog_step_buttons[i] = lv_btn_create(step_row);
    lv_obj_add_style(jog_step_buttons[i], &style_button, LV_PART_MAIN);
    lv_obj_set_size(jog_step_buttons[i], 76, 32);
    lv_obj_add_event_cb(jog_step_buttons[i], onJogStep, LV_EVENT_CLICKED, reinterpret_cast<void*>(static_cast<uintptr_t>(i)));
    lv_obj_t* label = lv_label_create(jog_step_buttons[i]);
    lv_label_set_text(label, labels[i]);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_center(label);
  }

  lv_obj_t* pad = lv_obj_create(tab);
  lv_obj_remove_style_all(pad);
  lv_obj_set_size(pad, LV_PCT(100), 118);
  lv_obj_clear_flag(pad, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t* y_plus = createJogButton(pad, "Y" LV_SYMBOL_UP, 'Y', 1, lv_color_hex(Colors::kStatusSuccess));
  lv_obj_align(y_plus, LV_ALIGN_TOP_MID, -42, 0);
  lv_obj_t* z_plus = createJogButton(pad, "Z" LV_SYMBOL_UP, 'Z', 1, lv_color_hex(Colors::kAxisZ));
  lv_obj_align(z_plus, LV_ALIGN_TOP_RIGHT, -12, 0);

  lv_obj_t* x_minus = createJogButton(pad, "X" LV_SYMBOL_LEFT, 'X', -1, lv_color_hex(Colors::kStatusError));
  lv_obj_align(x_minus, LV_ALIGN_CENTER, -122, 0);

  lv_obj_t* x_plus = createJogButton(pad, "X" LV_SYMBOL_RIGHT, 'X', 1, lv_color_hex(Colors::kStatusError));
  lv_obj_align(x_plus, LV_ALIGN_CENTER, 38, 0);

  lv_obj_t* y_minus = createJogButton(pad, "Y" LV_SYMBOL_DOWN, 'Y', -1, lv_color_hex(Colors::kStatusSuccess));
  lv_obj_align(y_minus, LV_ALIGN_BOTTOM_MID, -42, 0);
  lv_obj_t* z_minus = createJogButton(pad, "Z" LV_SYMBOL_DOWN, 'Z', -1, lv_color_hex(Colors::kAxisZ));
  lv_obj_align(z_minus, LV_ALIGN_BOTTOM_RIGHT, -12, 0);

  jog_lock_overlay = lv_obj_create(tab);
  lv_obj_remove_style_all(jog_lock_overlay);
  lv_obj_add_flag(jog_lock_overlay, LV_OBJ_FLAG_IGNORE_LAYOUT);
  lv_obj_add_flag(jog_lock_overlay, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_size(jog_lock_overlay, LV_PCT(100), LV_PCT(100));
  lv_obj_align(jog_lock_overlay, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_style_bg_color(jog_lock_overlay, lv_color_hex(Colors::kBg), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(jog_lock_overlay, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_clear_flag(jog_lock_overlay, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(jog_lock_overlay, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t* lock_label = lv_label_create(jog_lock_overlay);
  lv_label_set_text(lock_label, LV_SYMBOL_PLAY "  Job in progress");
  lv_obj_set_style_text_font(lock_label, &lv_font_montserrat_18, LV_PART_MAIN);
  lv_obj_set_style_text_color(lock_label, lv_color_hex(Colors::kTextMuted), LV_PART_MAIN);
  lv_obj_center(lock_label);

  updateJogStepButtons();
}

void setJogLocked(bool locked) {
  if (!jog_lock_overlay) {
    return;
  }
  if (locked) {
    cancelJog();
    lv_obj_clear_flag(jog_lock_overlay, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(jog_lock_overlay, LV_OBJ_FLAG_HIDDEN);
  }
}
