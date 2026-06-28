// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#include "Display.h"

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <Preferences.h>
#include <Wire.h>
#include <lvgl.h>

#include "AppState.h"
#include "app_config.h"
#include "generated/fluidNC_logo_png.h"
#include "generated/version.h"

namespace {

constexpr uint8_t kMinBrightness = 24;
constexpr char kPrefsNamespace[] = "fluidmon";
constexpr char kPrefsBrightnessKey[] = "bright";
constexpr char kPrefsFlipKey[] = "flip";
constexpr char kPrefsInactivityKey[] = "inact";
constexpr char kPrefsHardwareProfileKey[] = "hwProfile";
constexpr char kPrefsHardwareSetupVersionKey[] = "hwSetupVer";
constexpr uint8_t kHardwareSetupVersion = 4;

constexpr uint8_t kRotationNormal = 1;
constexpr uint8_t kRotationFlipped = 3;

constexpr uint32_t kInactivityTimeoutMs = 60000;

uint8_t active_brightness = UI_ACTIVE_BRIGHTNESS;
bool display_flipped = false;
uint8_t inactivity_mode = kInactivityDim;
uint32_t last_activity_ms = 0;
bool display_sleeping = false;
bool display_hardware_ready = false;

constexpr int16_t kDragThreshold = 10;
bool touch_down = false;
bool touch_dragged = false;
int16_t touch_origin_x = 0;
int16_t touch_origin_y = 0;

class LGFX : public lgfx::LGFX_Device {
 public:
  enum class HardwareProfile : uint8_t {
    kSt7789Cst816s,
    kIli9341Ft5x06,
    kIli9341Xpt2046,
    kSt7789Xpt2046,
  };

  struct ProfileInfo {
    HardwareProfile profile;
    uint8_t code;  // persisted CYD_PROFILE_* code
    const char* panel_name;
    const char* touch_name;
    const char* profile_name;
    uint8_t backlight_pin;
    bool is_ili9341;
    bool has_i2c_touch;
    bool supports_battery;
    uint8_t panel_offset_rotation;
    uint8_t touch_offset_rotation;
  };

  static const ProfileInfo* profileTable(uint8_t& count) {
    static constexpr ProfileInfo kProfiles[] = {
        {HardwareProfile::kSt7789Cst816s, CYD_PROFILE_ST7789_CST816S, "ST7789", "CST816S",
         "ST7789 + CST816S", CYD_TFT_BL, false, true, true, CYD_PANEL_OFFSET_ROTATION, CYD_TOUCH_OFFSET_ROTATION},
        {HardwareProfile::kIli9341Ft5x06, CYD_PROFILE_ILI9341_FT5X06, "ILI9341", "FT5x06",
         "ILI9341 + FT5x06", CYD_ALT_TFT_BL, true, true, true, CYD_PANEL_OFFSET_ROTATION, CYD_TOUCH_OFFSET_ROTATION},
        {HardwareProfile::kIli9341Xpt2046, CYD_PROFILE_ILI9341_XPT2046, "ILI9341", "XPT2046",
         "ILI9341 + XPT2046", CYD_RES_TFT_BL, true, false, false, CYD_RES_PANEL_OFFSET_ROTATION,
         CYD_RES_TOUCH_OFFSET_ROTATION},
        {HardwareProfile::kSt7789Xpt2046, CYD_PROFILE_ST7789_XPT2046, "ST7789", "XPT2046",
         "ST7789 + XPT2046", CYD_RES_TFT_BL, false, false, false, CYD_RES_ST7789_PANEL_OFFSET_ROTATION,
         CYD_RES_ST7789_TOUCH_OFFSET_ROTATION},
    };
    count = sizeof(kProfiles) / sizeof(kProfiles[0]);
    return kProfiles;
  }

  static const ProfileInfo& profileInfo(HardwareProfile profile) {
    uint8_t count = 0;
    return profileTable(count)[static_cast<uint8_t>(profile)];
  }

 private:
  lgfx::Bus_SPI bus_;
  lgfx::Light_PWM light_;
  lgfx::Panel_ST7789 panel_st7789_;
  lgfx::Panel_ILI9341 panel_ili9341_;
  lgfx::Touch_CST816S touch_cst816s_;
  lgfx::Touch_FT5x06 touch_ft5x06_;
  lgfx::Touch_XPT2046 touch_xpt2046_;
  HardwareProfile profile_ = HardwareProfile::kSt7789Cst816s;
  bool profile_detected_ = false;

  struct TouchProfile {
    int16_t pin_int;
    int16_t pin_rst;
    uint8_t i2c_port;
    uint8_t i2c_addr;
  };

  static TouchProfile cst816sTouchProfile() {
    return {CYD_TOUCH_INT, CYD_TOUCH_RST, CYD_TOUCH_I2C_PORT, CYD_TOUCH_ADDR};
  }

  static TouchProfile ft5x06TouchProfile() {
    return {CYD_ALT_TOUCH_INT, CYD_TOUCH_RST, CYD_ALT_TOUCH_I2C_PORT, CYD_ALT_TOUCH_ADDR};
  }

  static TwoWire& i2cBus(uint8_t port) {
    return port == 1 ? Wire1 : Wire;
  }

  static void resetTouchPin(int16_t pin_rst, uint16_t settle_ms) {
    if (pin_rst < 0) {
      return;
    }
    pinMode(pin_rst, OUTPUT);
    digitalWrite(pin_rst, LOW);
    delay(10);
    digitalWrite(pin_rst, HIGH);
    delay(settle_ms);
  }

  static bool readI2cRegister(const TouchProfile& profile,
                              uint8_t reg,
                              uint8_t* data,
                              size_t length,
                              bool stop_after_write = false) {
    TwoWire& bus = i2cBus(profile.i2c_port);
    bus.end();
    delay(1);
    bus.begin(CYD_TOUCH_SDA, CYD_TOUCH_SCL, 400000);
    bus.beginTransmission(profile.i2c_addr);
    bus.write(reg);
    if (bus.endTransmission(stop_after_write) != 0) {
      bus.end();
      return false;
    }

    const uint8_t read = bus.requestFrom(static_cast<int>(profile.i2c_addr), static_cast<int>(length));
    if (read != length) {
      bus.end();
      return false;
    }

    for (size_t i = 0; i < length; ++i) {
      data[i] = bus.read();
    }
    bus.end();
    return true;
  }

  static bool hasNonZeroByte(const uint8_t* data, size_t length) {
    for (size_t i = 0; i < length; ++i) {
      if (data[i] != 0x00 && data[i] != 0xFF) {
        return true;
      }
    }
    return false;
  }

  static bool probeCst816s() {
    const TouchProfile profile = cst816sTouchProfile();
    resetTouchPin(profile.pin_rst, 50);
    uint8_t data[3] = {};
    return readI2cRegister(profile, 0xA7, data, sizeof(data), true) && hasNonZeroByte(data, sizeof(data));
  }

  static void prepareResistiveTouchPins() {
    Wire.end();
    Wire1.end();
    delay(1);

    pinMode(CYD_RES_TOUCH_CS, OUTPUT);
    digitalWrite(CYD_RES_TOUCH_CS, HIGH);

    if (CYD_RES_TOUCH_SPI_HOST < 0) {
      pinMode(CYD_RES_TOUCH_SCLK, OUTPUT);
      digitalWrite(CYD_RES_TOUCH_SCLK, LOW);
      pinMode(CYD_RES_TOUCH_MOSI, OUTPUT);
      digitalWrite(CYD_RES_TOUCH_MOSI, LOW);
      pinMode(CYD_RES_TOUCH_MISO, INPUT_PULLUP);
    }
  }

  static uint32_t readPanelRegister(lgfx::IBus& bus, uint_fast16_t cmd, uint8_t dummy_bits, uint8_t read_count) {
    constexpr uint_fast8_t kDataBits = 8;
    pinMode(CYD_TFT_CS, OUTPUT);
    digitalWrite(CYD_TFT_CS, HIGH);

    bus.beginTransaction();
    bus.writeCommand(0, kDataBits);
    bus.wait();

    digitalWrite(CYD_TFT_CS, LOW);
    bus.writeCommand(cmd, kDataBits);
    bus.beginRead(dummy_bits);

    uint32_t result = 0;
    for (uint8_t i = 0; i < read_count; ++i) {
      result |= ((bus.readData(kDataBits) >> (kDataBits - 8)) & 0xFF) << (i * 8);
    }

    bus.endTransaction();
    digitalWrite(CYD_TFT_CS, HIGH);
    return result;
  }

  static bool detectIli9341Panel() {
    lgfx::Bus_SPI detect_bus;
    auto cfg = detect_bus.config();
    cfg.spi_host = HSPI_HOST;
    cfg.spi_mode = 0;
    cfg.freq_write = 8000000;
    cfg.freq_read = 8000000;
    cfg.spi_3wire = false;
    cfg.use_lock = true;
    cfg.dma_channel = 0;
    cfg.pin_sclk = CYD_TFT_SCLK;
    cfg.pin_mosi = CYD_TFT_MOSI;
    cfg.pin_miso = CYD_TFT_MISO;
    cfg.pin_dc = CYD_TFT_DC;
    detect_bus.config(cfg);
    detect_bus.init();

    const uint32_t id1 = readPanelRegister(detect_bus, 0xDA, 0, 1);
    const uint32_t id = readPanelRegister(detect_bus, 0x04, 1, 4);
    detect_bus.release();

    // ST7789 reports 0x85 in RDDID; ILI9341 reports 0x00 for RDID1.
    Serial.printf("Display panel probe: RDID1=0x%02lX RDDID=0x%08lX\n",
                  static_cast<unsigned long>(id1 & 0xFF),
                  static_cast<unsigned long>(id));
    if ((id & 0xFF) == 0x85) {
      return false;
    }
    return (id1 & 0xFF) == 0x00;
  }

  static HardwareProfile resistiveProfileForPanel() {
    return detectIli9341Panel() ? HardwareProfile::kIli9341Xpt2046 : HardwareProfile::kSt7789Xpt2046;
  }

  void configureBus() {
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
  }

  template <typename Panel>
  void configurePanel(Panel& panel) {
    auto cfg = panel.config();
    cfg.pin_cs = CYD_TFT_CS;
    cfg.pin_rst = CYD_TFT_RST;
    cfg.pin_busy = -1;
    cfg.panel_width = 240;
    cfg.panel_height = 320;
    cfg.memory_width = 240;
    cfg.memory_height = 320;
    cfg.offset_x = 0;
    cfg.offset_y = 0;
    cfg.offset_rotation = profileInfo(profile_).panel_offset_rotation;
    cfg.dummy_read_pixel = 8;
    cfg.dummy_read_bits = 1;
    cfg.readable = true;
    cfg.invert = CYD_PANEL_INVERT;
    cfg.rgb_order = CYD_PANEL_RGB_ORDER;
    cfg.dlen_16bit = false;
    cfg.bus_shared = false;
    panel.config(cfg);
  }

  void configureLight(uint8_t pin_bl) {
    auto cfg = light_.config();
    cfg.pin_bl = pin_bl;
    cfg.invert = CYD_BACKLIGHT_INVERT;
    cfg.freq = 44100;
    cfg.pwm_channel = 7;
    light_.config(cfg);
  }

  template <typename Touch>
  void configureTouch(Touch& touch, const TouchProfile& profile) {
    auto cfg = touch.config();
    cfg.x_min = 0;
    cfg.x_max = 240;
    cfg.y_min = 0;
    cfg.y_max = 320;
    cfg.pin_int = profile.pin_int;
    cfg.pin_rst = profile.pin_rst;
    cfg.bus_shared = false;
    cfg.offset_rotation = profileInfo(profile_).touch_offset_rotation;
    cfg.i2c_port = profile.i2c_port;
    cfg.i2c_addr = profile.i2c_addr;
    cfg.pin_sda = CYD_TOUCH_SDA;
    cfg.pin_scl = CYD_TOUCH_SCL;
    cfg.freq = 400000;
    touch.config(cfg);
  }

  bool detectFt5x06() {
    configureTouch(touch_ft5x06_, ft5x06TouchProfile());
    return touch_ft5x06_.init();
  }

  void configureResistiveTouch(HardwareProfile profile) {
    prepareResistiveTouchPins();

    auto cfg = touch_xpt2046_.config();
    cfg.x_min = CYD_RES_TOUCH_X_MIN;
    cfg.x_max = CYD_RES_TOUCH_X_MAX;
    cfg.y_min = CYD_RES_TOUCH_Y_MIN;
    cfg.y_max = CYD_RES_TOUCH_Y_MAX;
    cfg.pin_int = CYD_RES_TOUCH_INT;
    cfg.bus_shared = false;
    cfg.spi_host = CYD_RES_TOUCH_SPI_HOST;
    cfg.pin_sclk = CYD_RES_TOUCH_SCLK;
    cfg.pin_mosi = CYD_RES_TOUCH_MOSI;
    cfg.pin_miso = CYD_RES_TOUCH_MISO;
    cfg.pin_cs = CYD_RES_TOUCH_CS;
    cfg.offset_rotation = profileInfo(profile).touch_offset_rotation;
    touch_xpt2046_.config(cfg);
  }

  HardwareProfile detectProfile() {
    profile_detected_ = true;
#if CYD_HARDWARE_PROFILE == CYD_PROFILE_ST7789_CST816S
    return HardwareProfile::kSt7789Cst816s;
#elif CYD_HARDWARE_PROFILE == CYD_PROFILE_ILI9341_FT5X06
    return HardwareProfile::kIli9341Ft5x06;
#elif CYD_HARDWARE_PROFILE == CYD_PROFILE_ILI9341_XPT2046
    return HardwareProfile::kIli9341Xpt2046;
#elif CYD_HARDWARE_PROFILE == CYD_PROFILE_ST7789_XPT2046
    return HardwareProfile::kSt7789Xpt2046;
#else
    if (detectFt5x06()) {
      return HardwareProfile::kIli9341Ft5x06;
    }
    if (probeCst816s()) {
      return HardwareProfile::kSt7789Cst816s;
    }
    profile_detected_ = false;
    return resistiveProfileForPanel();
#endif
  }

  void applyProfile(HardwareProfile profile) {
    profile_ = profile;
    configureBus();

    if (profile_ == HardwareProfile::kIli9341Ft5x06) {
      configurePanel(panel_ili9341_);
      configureLight(profileInfo(profile_).backlight_pin);
      configureTouch(touch_ft5x06_, ft5x06TouchProfile());
      panel_ili9341_.setBus(&bus_);
      panel_ili9341_.setLight(&light_);
      panel_ili9341_.setTouch(&touch_ft5x06_);
      setPanel(&panel_ili9341_);
      return;
    }

    if (profile_ == HardwareProfile::kIli9341Xpt2046) {
      configurePanel(panel_ili9341_);
      configureLight(profileInfo(profile_).backlight_pin);
      configureResistiveTouch(profile_);
      panel_ili9341_.setBus(&bus_);
      panel_ili9341_.setLight(&light_);
      panel_ili9341_.setTouch(&touch_xpt2046_);
      setPanel(&panel_ili9341_);
      return;
    }

    if (profile_ == HardwareProfile::kSt7789Xpt2046) {
      configurePanel(panel_st7789_);
      configureLight(profileInfo(profile_).backlight_pin);
      configureResistiveTouch(profile_);
      panel_st7789_.setBus(&bus_);
      panel_st7789_.setLight(&light_);
      panel_st7789_.setTouch(&touch_xpt2046_);
      setPanel(&panel_st7789_);
      return;
    }

    configurePanel(panel_st7789_);
    configureLight(profileInfo(profile_).backlight_pin);
    configureTouch(touch_cst816s_, cst816sTouchProfile());
    panel_st7789_.setBus(&bus_);
    panel_st7789_.setLight(&light_);
    panel_st7789_.setTouch(&touch_cst816s_);
    setPanel(&panel_st7789_);
  }

 public:
  LGFX() {
    applyProfile(profile_);
  }

  void autoConfigure() {
    applyProfile(detectProfile());
  }

  void setHardwareProfile(HardwareProfile profile) {
    profile_detected_ = true;
    applyProfile(profile);
  }

  bool setTouchProfile(HardwareProfile profile) {
    if (profile == HardwareProfile::kIli9341Xpt2046 || profile == HardwareProfile::kSt7789Xpt2046) {
      configureResistiveTouch(profile);
      getPanel()->setTouch(&touch_xpt2046_);
    } else if (profile == HardwareProfile::kIli9341Ft5x06) {
      configureTouch(touch_ft5x06_, ft5x06TouchProfile());
      getPanel()->setTouch(&touch_ft5x06_);
    } else {
      configureTouch(touch_cst816s_, cst816sTouchProfile());
      getPanel()->setTouch(&touch_cst816s_);
    }
    const bool touch_init_ok = getPanel()->initTouch();
    Serial.printf("Hardware setup: touch driver init for %s: %s\n",
                  profileInfo(profile).profile_name,
                  touch_init_ok ? "ok" : "failed");
    return touch_init_ok;
  }

  static bool hardwareProfileFromCode(uint8_t code, HardwareProfile& profile) {
    uint8_t count = 0;
    const ProfileInfo* table = profileTable(count);
    for (uint8_t i = 0; i < count; ++i) {
      if (table[i].code == code) {
        profile = table[i].profile;
        return true;
      }
    }
    return false;
  }

  HardwareProfile hardwareProfile() const {
    return profile_;
  }

  bool profileDetected() const {
    return profile_detected_;
  }

  bool hasI2cTouch() const {
    return profileInfo(profile_).has_i2c_touch;
  }

  bool supportsBatteryMonitor() const {
    return profileInfo(profile_).supports_battery;
  }

  uint8_t backlightPin() const {
    return profileInfo(profile_).backlight_pin;
  }

  const char* panelName() const {
    return profileInfo(profile_).panel_name;
  }

  const char* touchName() const {
    return profileInfo(profile_).touch_name;
  }

  const char* profileName() const {
    return profileInfo(profile_).profile_name;
  }
};

using HardwareProfile = LGFX::HardwareProfile;

LGFX gfx;
lv_disp_draw_buf_t draw_buf;
lv_color_t draw_buf_1[kScreenWidth * kLvglBufferLines];

void applyRotation() {
  gfx.setRotation(display_flipped ? kRotationFlipped : kRotationNormal);
}

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

bool loadSavedHardwareProfile(HardwareProfile& profile) {
  Preferences prefs;
  if (!prefs.begin(kPrefsNamespace, true)) {
    return false;
  }
  const bool has_key = prefs.isKey(kPrefsHardwareProfileKey);
  const uint8_t version = prefs.getUChar(kPrefsHardwareSetupVersionKey, 0);
  const uint8_t code = prefs.getUChar(kPrefsHardwareProfileKey, CYD_PROFILE_AUTO);
  prefs.end();
  return has_key && version == kHardwareSetupVersion && LGFX::hardwareProfileFromCode(code, profile);
}

void saveHardwareProfile(HardwareProfile profile) {
  Preferences prefs;
  if (prefs.begin(kPrefsNamespace, false)) {
    prefs.putUChar(kPrefsHardwareProfileKey, LGFX::profileInfo(profile).code);
    prefs.putUChar(kPrefsHardwareSetupVersionKey, kHardwareSetupVersion);
    prefs.end();
  }
}

void clearSavedHardwareProfile() {
  Preferences prefs;
  if (prefs.begin(kPrefsNamespace, false)) {
    prefs.remove(kPrefsHardwareProfileKey);
    prefs.remove(kPrefsHardwareSetupVersionKey);
    prefs.end();
  }
}

// BOOT held at power-up forces the saved profile to be discarded.
bool hardwareProfileResetRequested() {
  pinMode(0, INPUT_PULLUP);
  delay(20);
  return digitalRead(0) == LOW;
}

const char* setupTouchKind(HardwareProfile profile) {
  return LGFX::profileInfo(profile).has_i2c_touch ? "Capacitive" : "Resistive";
}

void drawHardwareSetupPrompt(HardwareProfile profile) {
  const uint16_t bg = gfx.color565(4, 8, 14);
  const uint16_t panel = gfx.color565(8, 47, 73);
  const uint16_t cyan = gfx.color565(103, 232, 249);
  const uint16_t teal = gfx.color565(45, 212, 191);
  const uint16_t yellow = gfx.color565(250, 204, 21);

  gfx.fillScreen(bg);
  gfx.setTextDatum(middle_center);
  gfx.setTextSize(1);
  gfx.setTextColor(TFT_WHITE, bg);
  gfx.setFont(&lgfx::fonts::FreeSansBold18pt7b);
  gfx.drawString("Tap the center", kScreenWidth / 2, 26);

  const int16_t cx = kScreenWidth / 2;
  const int16_t cy = 116;
  gfx.fillCircle(cx, cy, 56, panel);
  gfx.drawCircle(cx, cy, 57, cyan);
  gfx.drawCircle(cx, cy, 40, cyan);
  gfx.drawCircle(cx, cy, 24, teal);
  gfx.fillCircle(cx, cy, 11, yellow);
  gfx.drawFastHLine(cx - 70, cy, 140, cyan);
  gfx.drawFastVLine(cx, cy - 70, 140, cyan);

  gfx.setTextColor(yellow, bg);
  gfx.setFont(&lgfx::fonts::FreeSansBold9pt7b);
  gfx.drawString("Tap repeatedly until detected", kScreenWidth / 2, 210);

  gfx.setTextDatum(top_left);
  gfx.setTextColor(gfx.color565(148, 163, 184), bg);
  gfx.setFont(&lgfx::fonts::Font0);
  gfx.drawString(setupTouchKind(profile), 6, 222);
  gfx.drawString("touch", 6, 231);

  gfx.setFont(&lgfx::fonts::Font0);
}

constexpr uint32_t kSetupTouchWindowMs = 1500;

bool waitForSetupTouch(uint32_t timeout_ms) {
  const uint32_t start_ms = millis();
  while (millis() - start_ms < timeout_ms) {
    uint16_t x = 0;
    uint16_t y = 0;
    if (gfx.getTouch(&x, &y)) {
      return true;
    }
    delay(25);
  }
  return false;
}

constexpr uint8_t kSetupCycleCount = 4;

HardwareProfile resistiveProfileForSamePanel(HardwareProfile profile) {
  return LGFX::profileInfo(profile).is_ili9341 ? HardwareProfile::kIli9341Xpt2046
                                               : HardwareProfile::kSt7789Xpt2046;
}

HardwareProfile alternateCapacitiveProfile(HardwareProfile profile) {
  return profile == HardwareProfile::kIli9341Ft5x06 ? HardwareProfile::kSt7789Cst816s
                                                    : HardwareProfile::kIli9341Ft5x06;
}

HardwareProfile setupProfileAt(uint8_t index, HardwareProfile first_profile) {
  const HardwareProfile resistive_profile = resistiveProfileForSamePanel(first_profile);
  const bool first_is_capacitive = LGFX::profileInfo(first_profile).has_i2c_touch;
  const HardwareProfile first_capacitive = first_is_capacitive ? first_profile : HardwareProfile::kSt7789Cst816s;
  const HardwareProfile second_capacitive = alternateCapacitiveProfile(first_capacitive);

  if (first_is_capacitive) {
    const HardwareProfile order[] = {first_capacitive, resistive_profile, second_capacitive, resistive_profile};
    return order[index % kSetupCycleCount];
  }

  const HardwareProfile order[] = {resistive_profile, first_capacitive, resistive_profile, second_capacitive};
  return order[index % kSetupCycleCount];
}

HardwareProfile runGuidedHardwareSetup(HardwareProfile first_profile) {
  bool has_last_wait_profile = false;
  HardwareProfile last_wait_profile = first_profile;

  pinMode(CYD_TFT_BL, OUTPUT);
  digitalWrite(CYD_TFT_BL, CYD_BACKLIGHT_INVERT ? LOW : HIGH);
  pinMode(CYD_ALT_TFT_BL, OUTPUT);
  digitalWrite(CYD_ALT_TFT_BL, CYD_BACKLIGHT_INVERT ? LOW : HIGH);

  gfx.init();
  applyRotation();
  gfx.setBrightness(active_brightness);

  while (true) {
    for (uint8_t i = 0; i < kSetupCycleCount; ++i) {
      const HardwareProfile profile = setupProfileAt(i, first_profile);
      if (has_last_wait_profile && profile == last_wait_profile) {
        continue;
      }

      Serial.printf("Hardware setup: trying %s\n", LGFX::profileInfo(profile).profile_name);
      if (!gfx.setTouchProfile(profile)) {
        continue;
      }

      drawHardwareSetupPrompt(profile);
      has_last_wait_profile = true;
      last_wait_profile = profile;
      if (waitForSetupTouch(kSetupTouchWindowMs)) {
        const uint16_t detected_bg = gfx.color565(7, 12, 18);
        gfx.fillScreen(detected_bg);
        gfx.setTextDatum(middle_center);
        gfx.setTextSize(1);
        gfx.setTextColor(gfx.color565(52, 211, 153), detected_bg);
        gfx.setFont(&lgfx::fonts::FreeSansBold18pt7b);
        gfx.drawString("Touch detected", kScreenWidth / 2, 100);
        gfx.setTextColor(TFT_WHITE, detected_bg);
        gfx.setFont(&lgfx::fonts::FreeSans9pt7b);
        gfx.drawString("Saving setup", kScreenWidth / 2, 140);
        gfx.setFont(&lgfx::fonts::Font0);
        delay(700);
        return profile;
      }
    }
  }
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

bool displaySupportsBattery() {
  return display_hardware_ready && gfx.supportsBatteryMonitor();
}

void drawSplash() {
  gfx.fillScreen(TFT_BLACK);

  const bool can_draw_logo = kFluidNCLogoPixelCount == kFluidNCLogoWidth * kFluidNCLogoHeight &&
                             kFluidNCLogoWidth <= kScreenWidth &&
                             kFluidNCLogoHeight <= kScreenHeight;
  if (can_draw_logo) {
    const int32_t x = (kScreenWidth - kFluidNCLogoWidth) / 2;
    const int32_t y = (kScreenHeight - kFluidNCLogoHeight) / 2;
    gfx.pushImage(x,
                  y,
                  kFluidNCLogoWidth,
                  kFluidNCLogoHeight,
                  reinterpret_cast<const lgfx::rgb565_t*>(kFluidNCLogoPixels));

    gfx.setTextColor(gfx.color565(150, 158, 170), TFT_BLACK);
    gfx.setTextDatum(middle_center);
    gfx.setTextSize(1);
    gfx.drawString((String("Firmware v") + kAppVersion).c_str(), kScreenWidth / 2, y + kFluidNCLogoHeight + 18);
  }

  if (!can_draw_logo) {
    gfx.setTextColor(TFT_WHITE, TFT_BLACK);
    gfx.setTextDatum(middle_center);
    gfx.setTextSize(3);
    gfx.drawString("FluidNC", kScreenWidth / 2, kScreenHeight / 2);
    gfx.setTextColor(gfx.color565(150, 158, 170), TFT_BLACK);
    gfx.setTextSize(1);
    gfx.drawString((String("v") + kAppVersion).c_str(), kScreenWidth / 2, kScreenHeight / 2 + 34);
  }

  delay(UI_SPLASH_MS);
  gfx.fillScreen(TFT_BLACK);
}

void initDisplay() {
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

  display_hardware_ready = false;
  bool display_already_initialized = false;
#if CYD_HARDWARE_PROFILE == CYD_PROFILE_AUTO
  if (hardwareProfileResetRequested()) {
    Serial.println("Display profile: BOOT held, clearing saved hardware profile");
    clearSavedHardwareProfile();
  }

  HardwareProfile saved_profile;
  if (loadSavedHardwareProfile(saved_profile)) {
    gfx.setHardwareProfile(saved_profile);
    Serial.println("Display profile: loaded saved hardware profile");
  } else {
    gfx.autoConfigure();
    const bool has_capacitive_hint = gfx.profileDetected();
    const HardwareProfile first_profile = gfx.hardwareProfile();
    if (!has_capacitive_hint) {
      gfx.setHardwareProfile(first_profile);
    }
    Serial.printf("Display profile: touch setup required%s\n",
                  has_capacitive_hint ? " (capacitive hint found)" : "");
    const HardwareProfile selected_profile = runGuidedHardwareSetup(first_profile);
    saveHardwareProfile(selected_profile);
    gfx.setHardwareProfile(selected_profile);
    gfx.init();
    applyRotation();
    gfx.setBrightness(active_brightness);
    display_already_initialized = true;
  }
#else
  gfx.autoConfigure();
#endif

  const bool display_ok = display_already_initialized || gfx.init();
  Serial.printf("Display profile: %s, panel=%s touch=%s, init %s\n",
                gfx.profileName(),
                gfx.panelName(),
                gfx.touchName(),
                display_ok ? "ok" : "failed");
  applyRotation();
  gfx.setBrightness(active_brightness);

#if UI_SPLASH_MS > 0
  Serial.println("Display splash: FluidNC logo");
  drawSplash();
#endif

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

  display_hardware_ready = display_ok;
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
  applyRotation();

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
