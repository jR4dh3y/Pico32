#pragma once

#include <Arduino.h>
#include "Config.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <LinkedList.h>

// ============================================
// WiFi Attack Module
// Ported from ESP32 Marauder WiFiScan
// ============================================

// Access Point structure
struct AccessPoint {
    String essid;
    uint8_t bssid[6];
    uint8_t channel;
    int8_t rssi;
    bool selected;
};

// Station structure  
struct Station {
    uint8_t mac[6];
    uint8_t bssid[6];  // Associated AP
    int8_t rssi;
    bool selected;
};

// SSID for beacon spam
struct SSID {
    String name;
    bool selected;
};