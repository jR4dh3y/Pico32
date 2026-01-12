#pragma once

#include <Arduino.h>
#include "Config.h"
#include <NimBLEDevice.h>

// ============================================
// Bluetooth Attack Module
// Ported from ESP32 Marauder BLE attacks
// ============================================

// Bluetooth spam types
enum class BTSpamType : uint8_t {
    APPLE,      // Sour Apple
    WINDOWS,    // SwiftPair
    SAMSUNG,    // Samsung spam
    GOOGLE,     // Google FastPair
    ALL         // All types
};

// Bluetooth scan modes
enum class BTMode : uint8_t {
    IDLE,
    SCAN_ALL,
    SCAN_AIRTAG,
    SCAN_FLIPPER,
    SCAN_SKIMMER,
    SPAM_APPLE,
    SPAM_WINDOWS,
    SPAM_SAMSUNG,
    SPAM_GOOGLE,
    SPAM_ALL
};

// Detected device
struct BTDevice {
    String mac;
    String name;
    int8_t rssi;
    uint32_t lastSeen;
};

class BTAttacks {
public:
    void begin();
    void update();
    void stop();
    
    // Scanning
    void startScanAll();
    void startScanAirtag();
    void startScanFlipper();
    void startScanSkimmer();
    
    // Spam attacks
    void startSpamApple();
    void startSpamWindows();
    void startSpamSamsung();
    void startSpamGoogle();
    void startSpamAll();
    
    bool isActive() const { return _mode != BTMode::IDLE; }
    
private:
    BTMode _mode = BTMode::IDLE;
    uint32_t _lastUpdate = 0;
    uint32_t _packetCount = 0;
    
    NimBLEAdvertising* _pAdvertising = nullptr;
    NimBLEScan* _pScan = nullptr;
    
    // Spam payload generators
    NimBLEAdvertisementData getApplePayload();
    NimBLEAdvertisementData getWindowsPayload();
    NimBLEAdvertisementData getSamsungPayload();
    NimBLEAdvertisementData getGooglePayload();
    
    // Helper
    void randomizeMac();
    void sendSpamPacket(BTSpamType type);
};

extern BTAttacks btAttacks;
