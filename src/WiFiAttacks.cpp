/**
 * ESP32 Marauder TUI - WiFi Attacks Implementation
 * 
 * Ported from ESP32 Marauder WiFiScan with minimal dependencies
 */

#include "WiFiAttacks.h"

WiFiAttacks wifiAttacks;

void WiFiAttacks::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_wifi_set_promiscuous(false);
    _mode = WiFiMode::IDLE;
}

void WiFiAttacks::update() {
    // TODO: Implement update loop
}

void WiFiAttacks::stop() {
    esp_wifi_set_promiscuous(false);
    _mode = WiFiMode::IDLE;
}