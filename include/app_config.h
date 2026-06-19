// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

// Cheap Yellow Display capacitive profiles. Defaults match the FluidDial-tested
// JC2432W328C-style board: ST7789 + CST816S + backlight on GPIO27.

#define CYD_PANEL_ILI9341 1
#define CYD_PANEL_ST7789 2
#define CYD_TOUCH_FT5X06 1
#define CYD_TOUCH_CST816S 2

#define CYD_PANEL_TYPE CYD_PANEL_ST7789
#define CYD_TOUCH_TYPE CYD_TOUCH_CST816S

#define CYD_TFT_SCLK 14
#define CYD_TFT_MOSI 13
#define CYD_TFT_MISO 12
#define CYD_TFT_CS 15
#define CYD_TFT_DC 2
#define CYD_TFT_RST -1
#define CYD_TFT_BL 27
#define CYD_BACKLIGHT_INVERT 0
#define CYD_PANEL_INVERT 0
#define CYD_PANEL_RGB_ORDER 0
#define CYD_PANEL_OFFSET_ROTATION 0

#define CYD_TOUCH_SDA 33
#define CYD_TOUCH_SCL 32
#define CYD_TOUCH_INT -1
#define CYD_TOUCH_RST 25
#define CYD_TOUCH_ADDR 0x15
#define CYD_TOUCH_I2C_PORT 0
#define CYD_TOUCH_OFFSET_ROTATION 0

#define CYD_BOARD_CAPACITIVE \
  ((CYD_TOUCH_TYPE == CYD_TOUCH_CST816S) || (CYD_TOUCH_TYPE == CYD_TOUCH_FT5X06))

#define CYD_BATTERY_ADC CYD_BOARD_CAPACITIVE
#define CYD_BATTERY_ADC_PIN 39
#define CYD_BATTERY_ADC_MULTIPLIER_NUM 1534
#define CYD_BATTERY_ADC_MULTIPLIER_DEN 1000

#define UI_ACTIVE_BRIGHTNESS 255
