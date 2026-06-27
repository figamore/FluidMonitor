// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <Arduino.h>
#include <FluidNCEspNowClient.h>
#include <lvgl.h>

constexpr int kScreenWidth = 320;
constexpr int kScreenHeight = 240;
constexpr size_t kLvglBufferLines = 40;

struct AxisTriplet {
  float x = 0;
  float y = 0;
  float z = 0;
  bool valid = false;
};

struct DroStatus {
  AxisTriplet work;
  AxisTriplet machine;
  AxisTriplet wco;
  bool inch = false;
  char state[16] = {};
};

extern FluidNCEspNowClient fluidnc;

extern lv_style_t style_screen;
extern lv_style_t style_topbar;
extern lv_style_t style_panel;
extern lv_style_t style_muted;
extern lv_style_t style_button;
extern lv_style_t style_overlay;

extern lv_obj_t* status_dot;
extern lv_obj_t* peer_label;
extern lv_obj_t* state_label;
extern lv_obj_t* status_x_label;
extern lv_obj_t* status_y_label;
extern lv_obj_t* status_z_label;
extern lv_obj_t* coord_work_button;
extern lv_obj_t* coord_machine_button;
extern lv_obj_t* units_label;
extern lv_obj_t* pair_button;
extern lv_obj_t* pair_label;
extern lv_obj_t* brightness_label;
extern lv_obj_t* pairing_success_panel;
extern lv_obj_t* jog_step_buttons[4];

extern DroStatus latest_dro;
extern volatile bool pending_dro;
extern bool suppress_touch_until_release;
extern uint8_t selected_jog_step;
extern bool jog_active;
extern bool status_show_machine;
