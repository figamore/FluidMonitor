// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <stdint.h>

void initSecureLink();
void pollSecureLink(uint32_t now);
void sendLine(const char* line);
