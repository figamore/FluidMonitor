// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <lvgl.h>

bool machineJobControlsVisible();
bool machineJobPaused();
const char* activeJobName();
int activeJobPercent();

void onJobPause(lv_event_t* event);
void onJobResume(lv_event_t* event);
void onJobAbort(lv_event_t* event);
void onJobEStop(lv_event_t* event);
