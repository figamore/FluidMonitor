// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#include "StatusParser.h"

#include <stdio.h>
#include <string.h>

#include "AppState.h"

namespace {

char status_line[192] = {};
size_t status_line_len = 0;

bool parseTriplet(const char* line, const char* tag, AxisTriplet& out) {
  const char* pos = strstr(line, tag);
  if (!pos) {
    return false;
  }
  pos += strlen(tag);
  AxisTriplet parsed;
  if (sscanf(pos, "%f,%f,%f", &parsed.x, &parsed.y, &parsed.z) != 3) {
    return false;
  }
  parsed.valid = true;
  out = parsed;
  return true;
}

void parseStatusLine(const char* line) {
  if (!line || line[0] != '<') {
    return;
  }

  const char* state_end = strpbrk(line + 1, "|>");
  if (!state_end) {
    return;
  }

  DroStatus parsed = latest_dro;
  size_t state_len = state_end - (line + 1);
  if (state_len >= sizeof(parsed.state)) {
    state_len = sizeof(parsed.state) - 1;
  }
  memcpy(parsed.state, line + 1, state_len);
  parsed.state[state_len] = '\0';

  const bool has_wpos = parseTriplet(line, "WPos:", parsed.work);
  const bool has_mpos = parseTriplet(line, "MPos:", parsed.machine);
  const bool has_wco = parseTriplet(line, "WCO:", parsed.wco);

  if (has_mpos && parsed.wco.valid) {
    parsed.work.x = parsed.machine.x - parsed.wco.x;
    parsed.work.y = parsed.machine.y - parsed.wco.y;
    parsed.work.z = parsed.machine.z - parsed.wco.z;
    parsed.work.valid = true;
  }
  if (has_wpos && parsed.wco.valid) {
    parsed.machine.x = parsed.work.x + parsed.wco.x;
    parsed.machine.y = parsed.work.y + parsed.wco.y;
    parsed.machine.z = parsed.work.z + parsed.wco.z;
    parsed.machine.valid = true;
  }
  if (!has_wpos && !has_mpos) {
    if (has_wco) {
      latest_dro = parsed;
    }
    return;
  }

  parsed.inch = strstr(line, "|Inch") != nullptr;
  latest_dro = parsed;
  pending_dro = true;
}

}  // namespace

void consumeSecureBytes(const uint8_t* data, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    char c = (char)data[i];
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      status_line[status_line_len] = '\0';
      parseStatusLine(status_line);
      status_line_len = 0;
      continue;
    }
    if (status_line_len + 1 < sizeof(status_line)) {
      status_line[status_line_len++] = c;
    } else {
      status_line_len = 0;
    }
  }
}
