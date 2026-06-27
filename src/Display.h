// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <stdint.h>

void initDisplay();

// Backlight brightness, 0-255. Setting it persists the value and keeps it
// applied across touch activity wake-ups. displayBrightness() returns the
// active value.
void setDisplayBrightness(uint8_t value);
uint8_t displayBrightness();

// Screen orientation. When flipped, the display (and touch) are rotated 180
// degrees. The choice persists to NVS and is reapplied on boot.
void setDisplayFlipped(bool flipped);
bool displayFlipped();
