// Copyright 2026 Figamore (https://github.com/figamore)
// SPDX-License-Identifier: GPL-3.0-only

#include "EspNowLinkClient.h"

#include <string.h>

#include "AppState.h"
#include "StatusParser.h"
#include "ui/SettingsView.h"
#include "ui/Ui.h"

void sendLine(const char* line) {
  if (!line || !espnow.isConnected()) {
    return;
  }
  espnow.write(reinterpret_cast<const uint8_t*>(line), strlen(line));
  espnow.write('\n');
}

void initSecureLink() {
  EspNowLinkConfig cfg;
  cfg.role = EspNowLinkRole::Client;
  cfg.hostname = "cyd-dro";
  cfg.nvsNamespace = "fluidespnow";
  cfg.labels.pairingWindow = "fluidnc-espnow-pairing-window-v1";
  cfg.labels.pairingSession = "fluidnc-espnow-pairing-session-v1";
  cfg.labels.pmk = "fluiddial-espnow-pmk-v1";

  espnow.onEvent([](const EspNowLinkEventInfo& event) {
    switch (event.type) {
      case EspNowLinkEvent::Started:
        setStatus(lv_color_hex(0x34D399));
        break;
      case EspNowLinkEvent::PairingStarted:
        setStatus(lv_color_hex(0x38BDF8));
        setPairButtonText("Cancel");
        break;
      case EspNowLinkEvent::PairingCancelled:
        setStatus(lv_color_hex(0xF59E0B));
        setPairButtonText("Pair");
        break;
      case EspNowLinkEvent::PairingFailed:
        setStatus(lv_color_hex(0xF87171));
        setPairButtonText("Pair");
        break;
      case EspNowLinkEvent::Paired:
        setStatus(lv_color_hex(0x34D399));
        setPairButtonText("Pair");
        updatePeerLabel();
        showPairingSuccess();
        break;
      case EspNowLinkEvent::Connected:
        setStatus(lv_color_hex(0x34D399));
        break;
      case EspNowLinkEvent::Disconnected:
        jog_active = false;
        setStatus(lv_color_hex(0xF59E0B));
        break;
      case EspNowLinkEvent::SendFailed:
        setStatus(lv_color_hex(0xF87171));
        break;
      default:
        break;
    }
  });

  espnow.onReceive([](const uint8_t* data, size_t len, const uint8_t*) {
    consumeSecureBytes(data, len);
  });

  if (!espnow.begin(cfg)) {
    setStatus(lv_color_hex(0xF87171));
    return;
  }
  updatePeerLabel();
}

void pollSecureLink(uint32_t now) {
  espnow.poll();

  static uint32_t last_status_ms = 0;
  if (espnow.isConnected() && now - last_status_ms >= 250) {
    last_status_ms = now;
    espnow.write('?');
  }
}
