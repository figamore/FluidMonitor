// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <lvgl.h>

void createSettingsTab(lv_obj_t* tab);
void createPairingSuccessPanel(lv_obj_t* screen);
void showPairingSuccess();
void updatePeerLabel();
void setPairButtonText(const char* text);
