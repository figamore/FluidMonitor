// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#include "JogView.h"

#include "../AppState.h"
#include "Ui.h"

namespace {

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
                              lv_color_hex(i == selected_jog_step ? 0xF59E0B : 0x243241),
                              LV_PART_MAIN);
    lv_obj_set_style_text_color(jog_step_buttons[i],
                                lv_color_hex(i == selected_jog_step ? 0xFFFFFF : 0xCBD5E1),
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
    setStatus(lv_color_hex(0xF87171));
    return;
  }
  if (!fluidnc.isIdle()) {
    setStatus(lv_color_hex(0xF59E0B));
    return;
  }
  cancelJog();

  const float feedrate = selectedJogDistance() * 300.0f;
  if (fluidnc.jogContinuous(axis, direction, feedrate)) {
    jog_active = true;
  }
}

void onJogButton(lv_event_t* event) {
  const lv_event_code_t code = lv_event_get_code(event);
  const uintptr_t data = reinterpret_cast<uintptr_t>(lv_event_get_user_data(event));
  const char axis = static_cast<char>((data >> 8) & 0xff);
  const int direction = (data & 0xff) ? 1 : -1;

  if (code == LV_EVENT_PRESSED) {
    startJog(axis, direction);
  } else if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST || code == LV_EVENT_CANCEL) {
    cancelJog();
  }
}

lv_obj_t* createJogButton(lv_obj_t* parent, const char* text, char axis, int direction, lv_color_t color) {
  lv_obj_t* button = lv_btn_create(parent);
  lv_obj_add_style(button, &style_button, LV_PART_MAIN);
  lv_obj_set_size(button, 70, 48);
  lv_obj_set_style_bg_color(button, lv_color_hex(0x1B2430), LV_PART_MAIN);
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

  lv_obj_t* y_plus = createJogButton(pad, "Y" LV_SYMBOL_UP, 'Y', 1, lv_color_hex(0x34D399));
  lv_obj_align(y_plus, LV_ALIGN_TOP_MID, -42, 0);
  lv_obj_t* z_plus = createJogButton(pad, "Z" LV_SYMBOL_UP, 'Z', 1, lv_color_hex(0x60A5FA));
  lv_obj_align(z_plus, LV_ALIGN_TOP_RIGHT, -12, 0);

  lv_obj_t* x_minus = createJogButton(pad, "X" LV_SYMBOL_LEFT, 'X', -1, lv_color_hex(0xF87171));
  lv_obj_align(x_minus, LV_ALIGN_CENTER, -122, 0);

  lv_obj_t* x_plus = createJogButton(pad, "X" LV_SYMBOL_RIGHT, 'X', 1, lv_color_hex(0xF87171));
  lv_obj_align(x_plus, LV_ALIGN_CENTER, 38, 0);

  lv_obj_t* y_minus = createJogButton(pad, "Y" LV_SYMBOL_DOWN, 'Y', -1, lv_color_hex(0x34D399));
  lv_obj_align(y_minus, LV_ALIGN_BOTTOM_MID, -42, 0);
  lv_obj_t* z_minus = createJogButton(pad, "Z" LV_SYMBOL_DOWN, 'Z', -1, lv_color_hex(0x60A5FA));
  lv_obj_align(z_minus, LV_ALIGN_BOTTOM_RIGHT, -12, 0);

  updateJogStepButtons();
}
