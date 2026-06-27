// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <stdint.h>

void initDisplay();

bool touchDragging();

void setDisplayBrightness(uint8_t value);
uint8_t displayBrightness();

void setDisplayFlipped(bool flipped);
bool displayFlipped();

enum DisplayInactivityMode : uint8_t {
  kInactivityDisplayOn = 0,
  kInactivityDim = 1,
  kInactivityDisplayOff = 2,
};

void setInactivityMode(uint8_t mode);
uint8_t inactivityMode();

void pollInactivity(bool machine_allows_sleep);
