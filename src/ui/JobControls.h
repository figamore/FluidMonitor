// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <lvgl.h>

bool machineJobControlsVisible();
bool machineJobPaused();
const char* activeJobName();
int activeJobPercent();

void onJobPauseToggle(lv_event_t* event);
void onJobAbort(lv_event_t* event);

// Sets a pause/play button's icon and colour to match the current hold state.
void updateJobPauseButton(lv_obj_t* button);
