/**
 * ESP32 Marauder TUI - Bluetooth Attacks Implementation
 * 
 * Ported from ESP32 Marauder BLE spam attacks
 * Based on ESP32 Sour Apple by RapierXbox/ECTO-1A
 */

#include "BTAttacks.h"

BTAttacks btAttacks;

void BTAttacks::begin() {
    NimBLEDevice::init("");
    _pAdvertising = NimBLEDevice::getAdvertising();
}

void BTAttacks::update() {
    if (_mode == BTMode::IDLE) return;
    // TODO: Implement scan/spam update loop
}

void BTAttacks::stop() {
    if (_pAdvertising) {
        _pAdvertising->stop();
    }
    if (_pScan) {
        _pScan->stop();
    }
    _mode = BTMode::IDLE;
}