// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#include "Display.h"

#include <LovyanGFX.hpp>
#include <Preferences.h>
#include <lvgl.h>

#include "AppState.h"
#include "app_config.h"

namespace {

constexpr uint8_t kMinBrightness = 24;
constexpr char kPrefsNamespace[] = "fluidmon";
constexpr char kPrefsBrightnessKey[] = "bright";
constexpr char kPrefsFlipKey[] = "flip";
constexpr char kPrefsInactivityKey[] = "inact";

constexpr uint8_t kRotationNormal = 1;
constexpr uint8_t kRotationFlipped = 3;

constexpr uint32_t kInactivityTimeoutMs = 60000;

uint8_t active_brightness = UI_ACTIVE_BRIGHTNESS;
bool display_flipped = false;
uint8_t inactivity_mode = kInactivityDim;
uint32_t last_activity_ms = 0;
bool display_sleeping = false;

constexpr int16_t kDragThreshold = 10;
bool touch_down = false;
bool touch_dragged = false;
int16_t touch_origin_x = 0;
int16_t touch_origin_y = 0;

class LGFX : public lgfx::LGFX_Device {
  lgfx::Bus_SPI bus_;
  lgfx::Light_PWM light_;
#if CYD_PANEL_TYPE == CYD_PANEL_ST7789
  lgfx::Panel_ST7789 panel_;
#else
  lgfx::Panel_ILI9341 panel_;
#endif
#if CYD_TOUCH_TYPE == CYD_TOUCH_CST816S
  lgfx::Touch_CST816S touch_;
#else
  lgfx::Touch_FT5x06 touch_;
#endif

 public:
  LGFX() {
    {
      auto cfg = bus_.config();
      cfg.spi_host = HSPI_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 55000000;
      cfg.freq_read = 16000000;
      cfg.spi_3wire = false;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = CYD_TFT_SCLK;
      cfg.pin_mosi = CYD_TFT_MOSI;
      cfg.pin_miso = CYD_TFT_MISO;
      cfg.pin_dc = CYD_TFT_DC;
      bus_.config(cfg);
      panel_.setBus(&bus_);
    }
    {
      auto cfg = panel_.config();
      cfg.pin_cs = CYD_TFT_CS;
      cfg.pin_rst = CYD_TFT_RST;
      cfg.pin_busy = -1;
      cfg.panel_width = 240;
      cfg.panel_height = 320;
      cfg.memory_width = 240;
      cfg.memory_height = 320;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = CYD_PANEL_OFFSET_ROTATION;
      cfg.dummy_read_pixel = 8;
      cfg.dummy_read_bits = 1;
      cfg.readable = true;
      cfg.invert = CYD_PANEL_INVERT;
      cfg.rgb_order = CYD_PANEL_RGB_ORDER;
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;
      panel_.config(cfg);
    }
    {
      auto cfg = light_.config();
      cfg.pin_bl = CYD_TFT_BL;
      cfg.invert = CYD_BACKLIGHT_INVERT;
      cfg.freq = 44100;
      cfg.pwm_channel = 7;
      light_.config(cfg);
      panel_.setLight(&light_);
    }
    {
      auto cfg = touch_.config();
      cfg.x_min = 0;
      cfg.x_max = 240;
      cfg.y_min = 0;
      cfg.y_max = 320;
      cfg.pin_int = CYD_TOUCH_INT;
      cfg.pin_rst = CYD_TOUCH_RST;
      cfg.bus_shared = false;
      cfg.offset_rotation = CYD_TOUCH_OFFSET_ROTATION;
      cfg.i2c_port = CYD_TOUCH_I2C_PORT;
      cfg.i2c_addr = CYD_TOUCH_ADDR;
      cfg.pin_sda = CYD_TOUCH_SDA;
      cfg.pin_scl = CYD_TOUCH_SCL;
      cfg.freq = 400000;
      touch_.config(cfg);
      panel_.setTouch(&touch_);
    }
    setPanel(&panel_);
  }
};

LGFX gfx;
lv_disp_draw_buf_t draw_buf;
lv_color_t draw_buf_1[kScreenWidth * kLvglBufferLines];

uint8_t dimBrightness() {
  uint8_t value = active_brightness / 5;
  return value < 8 ? 8 : value;
}

// returns true if it woke the display
bool noteActivity() {
  last_activity_ms = millis();
  if (display_sleeping) {
    display_sleeping = false;
    gfx.setBrightness(active_brightness);
    return true;
  }
  return false;
}

void flushDisplay(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
  int32_t width = area->x2 - area->x1 + 1;
  int32_t height = area->y2 - area->y1 + 1;
  gfx.startWrite();
  gfx.setAddrWindow(area->x1, area->y1, width, height);
  gfx.writePixels(reinterpret_cast<lgfx::rgb565_t*>(color_p), width * height);
  gfx.endWrite();
  lv_disp_flush_ready(disp);
}

void readTouch(lv_indev_drv_t*, lv_indev_data_t* data) {
  uint16_t x = 0;
  uint16_t y = 0;
  if (gfx.getTouch(&x, &y)) {
    // swallow the wake tap so it doesn't also press a control
    if (noteActivity()) {
      suppress_touch_until_release = true;
    }
    if (!touch_down) {
      touch_down = true;
      touch_dragged = false;
      touch_origin_x = x;
      touch_origin_y = y;
    } else if (abs(x - touch_origin_x) > kDragThreshold || abs(y - touch_origin_y) > kDragThreshold) {
      touch_dragged = true;
    }
    if (suppress_touch_until_release) {
      data->state = LV_INDEV_STATE_REL;
      return;
    }
    data->state = LV_INDEV_STATE_PR;
    data->point.x = x;
    data->point.y = y;
  } else {
    suppress_touch_until_release = false;
    touch_down = false;
    data->state = LV_INDEV_STATE_REL;
  }
}

}  // namespace

bool touchDragging() {
  return touch_dragged;
}

void initDisplay() {
  gfx.init();

  Preferences prefs;
  if (prefs.begin(kPrefsNamespace, true)) {
    active_brightness = prefs.getUChar(kPrefsBrightnessKey, UI_ACTIVE_BRIGHTNESS);
    display_flipped = prefs.getBool(kPrefsFlipKey, false);
    inactivity_mode = prefs.getUChar(kPrefsInactivityKey, kInactivityDim);
    prefs.end();
  }
  if (active_brightness < kMinBrightness) {
    active_brightness = kMinBrightness;
  }
  if (inactivity_mode > kInactivityDisplayOff) {
    inactivity_mode = kInactivityDim;
  }
  last_activity_ms = millis();
  gfx.setRotation(display_flipped ? kRotationFlipped : kRotationNormal);
  gfx.setBrightness(active_brightness);

  lv_init();
  lv_disp_draw_buf_init(&draw_buf, draw_buf_1, nullptr, kScreenWidth * kLvglBufferLines);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = kScreenWidth;
  disp_drv.ver_res = kScreenHeight;
  disp_drv.flush_cb = flushDisplay;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = readTouch;
  indev_drv.scroll_limit = 6;
  lv_indev_drv_register(&indev_drv);
}

void setDisplayBrightness(uint8_t value) {
  if (value < kMinBrightness) {
    value = kMinBrightness;
  }
  active_brightness = value;
  display_sleeping = false;
  last_activity_ms = millis();
  gfx.setBrightness(active_brightness);

  Preferences prefs;
  if (prefs.begin(kPrefsNamespace, false)) {
    prefs.putUChar(kPrefsBrightnessKey, active_brightness);
    prefs.end();
  }
}

uint8_t displayBrightness() {
  return active_brightness;
}

void setDisplayFlipped(bool flipped) {
  display_flipped = flipped;
  gfx.setRotation(display_flipped ? kRotationFlipped : kRotationNormal);

  Preferences prefs;
  if (prefs.begin(kPrefsNamespace, false)) {
    prefs.putBool(kPrefsFlipKey, display_flipped);
    prefs.end();
  }

  lv_obj_invalidate(lv_scr_act());
}

bool displayFlipped() {
  return display_flipped;
}

void setInactivityMode(uint8_t mode) {
  if (mode > kInactivityDisplayOff) {
    mode = kInactivityDim;
  }
  inactivity_mode = mode;
  display_sleeping = false;
  last_activity_ms = millis();
  gfx.setBrightness(active_brightness);

  Preferences prefs;
  if (prefs.begin(kPrefsNamespace, false)) {
    prefs.putUChar(kPrefsInactivityKey, inactivity_mode);
    prefs.end();
  }
}

uint8_t inactivityMode() {
  return inactivity_mode;
}

void pollInactivity(bool machine_allows_sleep) {
  if (inactivity_mode == kInactivityDisplayOn || !machine_allows_sleep) {
    noteActivity();
    return;
  }
  if (display_sleeping) {
    return;
  }
  if (millis() - last_activity_ms < kInactivityTimeoutMs) {
    return;
  }
  display_sleeping = true;
  gfx.setBrightness(inactivity_mode == kInactivityDisplayOff ? 0 : dimBrightness());
}
