// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <lvgl.h>

void createUi();
void applyDro();
void setStatus(lv_color_t color);
void formatAxis(lv_obj_t* label, const char* axis, float value);
lv_obj_t* makePanel(lv_obj_t* parent);
lv_obj_t* createSmallButton(lv_obj_t* parent, const char* text, lv_event_cb_t cb, void* user_data);
