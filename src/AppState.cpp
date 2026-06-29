// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#include "AppState.h"

FluidNCEspNowClient fluidnc("cyd-dro");

lv_style_t style_screen;
lv_style_t style_topbar;
lv_style_t style_panel;
lv_style_t style_muted;
lv_style_t style_button;
lv_style_t style_overlay;

lv_obj_t* status_dot = nullptr;
lv_obj_t* peer_label = nullptr;
lv_obj_t* state_label = nullptr;
lv_obj_t* status_x_label = nullptr;
lv_obj_t* status_y_label = nullptr;
lv_obj_t* status_z_label = nullptr;
lv_obj_t* coord_work_button = nullptr;
lv_obj_t* coord_machine_button = nullptr;
lv_obj_t* units_label = nullptr;
lv_obj_t* pair_button = nullptr;
lv_obj_t* pair_label = nullptr;
lv_obj_t* brightness_label = nullptr;
lv_obj_t* pairing_success_panel = nullptr;
lv_obj_t* jog_step_buttons[4] = {};

DroStatus latest_dro = {};
JobState job_state = {};
char alarm_reason[64] = {};
int alarm_code = 0;
volatile bool pending_dro = false;
volatile bool pending_job_ui = false;
bool suppress_touch_until_release = false;
uint8_t selected_jog_step = 1;
bool jog_active = false;
bool status_show_machine = false;
