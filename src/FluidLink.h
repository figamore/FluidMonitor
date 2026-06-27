// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

void initFluidLink();
void pollFluidLink();

// True when the display may sleep: the machine is idle, or none is connected.
bool machineAllowsSleep();
