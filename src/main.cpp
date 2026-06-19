// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#include <Arduino.h>
#include <WiFi.h>
#include <lvgl.h>

#include "Display.h"
#include "EspNowLinkClient.h"
#include "ui/Ui.h"

void setup() {
  Serial.begin(115200);
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  initDisplay();
  createUi();
  initSecureLink();
}

void loop() {
  static uint32_t last_tick_ms = millis();
  uint32_t now = millis();
  uint32_t elapsed = now - last_tick_ms;
  last_tick_ms = now;
  lv_tick_inc(elapsed);

  pollSecureLink(now);
  applyDro();
  lv_timer_handler();
  delay(5);
}
