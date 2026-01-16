#pragma once

// ============================================
// ESP32 Marauder TUI Configuration
// Target: ESP32 Dev Module (no PSRAM)
// ============================================

#define MARAUDER_TUI
#define VERSION "0.1.3"
#define HARDWARE_NAME "ESP32 Dev Module"

// Hardware capabilities
#define HAS_BT          // Bluetooth enabled
// #define HAS_SCREEN   // No display
// #define HAS_SD       // No SD card
// #define HAS_GPS      // No GPS
// #define HAS_BATTERY  // No battery management
// #define HAS_LED      // No NeoPixel

// WiFi settings
#define DEFAULT_CHANNEL 1
#define MAX_CHANNEL 14

// Serial settings
#define SERIAL_BAUD 115200

// TUI settings
#define TUI_REFRESH_MS 100
#define TUI_WIDTH 40

// Memory constraints (no PSRAM)
#define MAX_APS 50
#define MAX_STATIONS 50
#define MAX_SSIDS 20
#define MAC_HISTORY_LEN 512
                                                            
