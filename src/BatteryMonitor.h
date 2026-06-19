// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

void initBatteryMonitor();
bool batteryAvailable();
int batteryAdcMillivolts();
int batteryMillivolts();
int batteryLevel();
bool batteryCharging();
