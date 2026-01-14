/**
 * ESP32 Marauder TUI
 * 
 * Stripped-down WiFi/Bluetooth attack platform
 * Controlled via serial terminal interface
 */

#include <Arduino.h>
#include "Config.h"
#include "SerialTUI.h"
#include "WiFiAttacks.h"
#include "BTAttacks.h"

void setup() {
    tui.begin();
    wifiAttacks.begin();
    btAttacks.begin();
    
    Serial.println("ESP32 Marauder TUI initialized");
}

void loop() {
    tui.update();
    wifiAttacks.update();
    btAttacks.update();
    
    // TODO: Handle menu actions
}