# CC1101 Module Integration Plan

## Overview

This plan outlines the integration of a **CC1101 sub-GHz RF transceiver module** into the ESP32 Marauder TUI project. The CC1101 enables transmission and reception in the sub-1 GHz frequency bands (300-928 MHz), adding powerful RF attack and analysis capabilities alongside the existing WiFi and Bluetooth functionality.

---

## Hardware Requirements

### CC1101 Module Specifications
- **Frequency Bands**: 315 MHz, 433 MHz, 868 MHz, 915 MHz
- **Modulation**: OOK, ASK, 2-FSK, GFSK, 4-FSK/MSK
- **Interface**: SPI (4-wire)
- **Data Rate**: Up to 600 kbps
- **Operating Voltage**: 1.8V - 3.6V (3.3V compatible with ESP32)

### Recommended Wiring (Default SPI)

| CC1101 Pin | ESP32 Pin | Description        |
|------------|-----------|-------------------|
| VCC        | 3.3V      | Power supply       |
| GND        | GND       | Ground             |
| MOSI       | GPIO 23   | SPI Master Out     |
| MISO       | GPIO 19   | SPI Master In      |
| SCK        | GPIO 18   | SPI Clock          |
| CSN        | GPIO 5    | Chip Select        |
| GDO0       | GPIO 4    | General Digital I/O (interrupt) |
| GDO2       | GPIO 2    | General Digital I/O (optional) |

> **Note**: Pins are configurable in `Config.h`. Avoid GPIO 12 (boot issues on some boards).

---

## Phase 1: Foundation & Hardware Abstraction

### 1.1 Configuration Updates (`Config.h`)

Add CC1101 hardware definitions:

```cpp
// CC1101 Hardware Enable
// #define HAS_CC1101          // Uncomment when CC1101 is connected

// CC1101 Pin Configuration
#ifdef HAS_CC1101
    #define CC1101_CSN_PIN      5
    #define CC1101_GDO0_PIN     4
    #define CC1101_GDO2_PIN     2
    #define CC1101_MOSI_PIN     23
    #define CC1101_MISO_PIN     19
    #define CC1101_SCK_PIN      18
#endif

// CC1101 Settings
#define CC1101_DEFAULT_FREQ     433.92  // Default frequency in MHz
#define CC1101_MAX_PACKET_SIZE  64
#define CC1101_RX_BUFFER_SIZE   256
#define MAX_SAVED_SIGNALS       10
```

### 1.2 Library Dependencies (`platformio.ini`)

Add CC1101 library:

```ini
lib_deps = 
    h2zero/NimBLE-Arduino@^1.4.0
    bblanchon/ArduinoJson@^6.21.2
    https://github.com/ivanseidel/LinkedList.git
    SPI                                           ; Built-in
    https://github.com/LSatan/SmartRC-CC1101-Driver-Lib.git  ; CC1101 driver
```

### 1.3 Create CC1101 Driver (`src/CC1101Radio.h` & `src/CC1101Radio.cpp`)

**Header File: `CC1101Radio.h`**

```cpp
#pragma once

#include <Arduino.h>
#include "Config.h"

#ifdef HAS_CC1101

#include <ELECHOUSE_CC1101_SRC_DRV.h>

// Frequency presets for common devices
enum class RFPreset : uint8_t {
    PRESET_315,     // 315 MHz (US garage doors)
    PRESET_433,     // 433.92 MHz (EU/Common)
    PRESET_868,     // 868 MHz (EU)
    PRESET_915,     // 915 MHz (US)
    PRESET_CUSTOM
};

// Modulation types
enum class RFModulation : uint8_t {
    MOD_ASK_OOK,
    MOD_2FSK,
    MOD_GFSK,
    MOD_4FSK_MSK
};

// Captured signal structure
struct CapturedSignal {
    float frequency;
    RFModulation modulation;
    uint8_t data[CC1101_MAX_PACKET_SIZE];
    uint8_t dataLength;
    int8_t rssi;
    char name[16];
    bool valid;
};

class CC1101Radio {
public:
    CC1101Radio();
    
    // Initialization
    bool init();
    bool isInitialized() const { return _initialized; }
    
    // Configuration
    void setFrequency(float mhz);
    float getFrequency() const { return _frequency; }
    void setPreset(RFPreset preset);
    void setModulation(RFModulation mod);
    void setBandwidth(float bw);
    void setDataRate(float rate);
    void setTxPower(int8_t dbm);
    
    // Reception
    bool startRx();
    void stopRx();
    bool isReceiving() const { return _rxActive; }
    int available();
    int readPacket(uint8_t* buffer, uint8_t maxLen);
    int8_t getLastRSSI() const { return _lastRSSI; }
    
    // Transmission
    bool transmit(const uint8_t* data, uint8_t len);
    bool transmitRepeat(const uint8_t* data, uint8_t len, uint16_t repeatCount, uint16_t delayMs);
    
    // Signal capture/replay
    bool captureSignal(CapturedSignal* signal, uint32_t timeoutMs);
    bool replaySignal(const CapturedSignal* signal);
    bool replaySignal(const CapturedSignal* signal, uint16_t repeatCount);
    
    // Raw signal operations
    bool startRawCapture();
    void stopRawCapture();
    bool transmitRaw(const uint8_t* timings, uint16_t count);
    
    // Scanning
    float scanForSignal(float startFreq, float endFreq, float stepMhz);
    
    // Status
    void printStatus();
    
private:
    bool _initialized;
    bool _rxActive;
    float _frequency;
    RFModulation _modulation;
    int8_t _lastRSSI;
    
    void configureForModulation(RFModulation mod);
};

extern CC1101Radio cc1101;

#endif // HAS_CC1101
```

---

## Phase 2: Core RF Attacks (`src/RFAttacks.h` & `src/RFAttacks.cpp`)

### 2.1 RFAttacks Class

**Header File: `RFAttacks.h`**

```cpp
#pragma once

#include <Arduino.h>
#include "Config.h"

#ifdef HAS_CC1101

#include "CC1101Radio.h"

// Stored signals
struct SignalStorage {
    CapturedSignal signals[MAX_SAVED_SIGNALS];
    uint8_t count;
};

class RFAttacks {
public:
    RFAttacks();
    
    // Scanning
    void frequencyScan(float startMhz, float endMhz, float stepMhz);
    void frequencyAnalyzer();
    
    // Signal Operations
    bool captureSignal(uint32_t timeoutMs = 10000);
    bool replayLastSignal();
    bool replaySignal(uint8_t index);
    void listSavedSignals();
    void clearSignals();
    
    // Protocol-Specific Attacks
    void bruteForceGarageDoor(uint8_t bits, float frequency);
    void jammerStart(float frequency);
    void jammerStop();
    
    // Common Device Attacks
    void sendTestSignal();
    void sendCustomPayload(const uint8_t* payload, uint8_t len, float freq);
    
    // Tesla Charge Port (PoC)
    void teslaChargePortOpen();
    
    // Settings
    void setFrequency(float mhz);
    float getFrequency() const;
    void setPreset(RFPreset preset);
    
    // Status
    bool isRunning() const { return _running; }
    void stop();
    
    // Signal storage
    SignalStorage storage;
    CapturedSignal lastCaptured;
    
private:
    bool _running;
    bool _jamming;
    
    void printSignalInfo(const CapturedSignal* sig);
};

extern RFAttacks rfAttacks;

#endif // HAS_CC1101
```

### 2.2 Attack Categories

| Category | Features | Priority |
|----------|----------|----------|
| **Frequency Scanning** | Sweep 300-928 MHz, signal strength display | High |
| **Signal Capture** | Store up to 10 signals with metadata | High |
| **Signal Replay** | Replay stored signals with optional repeat | High |
| **Frequency Analyzer** | Real-time spectrum view (basic) | Medium |
| **Jamming** | Continuous transmission on target freq | Medium |
| **Brute Force** | Generate all possible codes (garage doors) | Medium |
| **Protocol Decode** | Common protocols (garage, doorbell, etc.) | Low |
| **Raw Capture** | Bit-level signal recording | Low |

---

## Phase 3: Menu Integration

### 3.1 Menu Actions (`MenuDefs.h`)

Add new menu actions to the `MenuAction` enum:

```cpp
enum class MenuAction : uint8_t {
    // ... existing actions ...
    
    // CC1101 Sub-GHz
    RF_SCAN_FREQ,           // Frequency scanner
    RF_ANALYZER,            // Spectrum analyzer
    RF_CAPTURE,             // Capture signal
    RF_REPLAY_LAST,         // Replay last captured
    RF_REPLAY_SELECT,       // Select and replay from storage
    RF_LIST_SIGNALS,        // List stored signals
    RF_CLEAR_SIGNALS,       // Clear all stored signals
    RF_JAMMER,              // Start/stop jammer
    RF_BRUTEFORCE,          // Brute force attack
    RF_TEST_SIGNAL,         // Send test signal
    RF_SET_FREQ,            // Set frequency
    RF_SET_PRESET,          // Set frequency preset
    RF_TESLA_CHARGE,        // Tesla charge port PoC
};
```

### 3.2 Menu Structure

```cpp
// RF Capture submenu
const MenuItem rfCaptureMenu[] = {
    {"Capture Signal", MenuAction::RF_CAPTURE, nullptr, 0},
    {"Replay Last", MenuAction::RF_REPLAY_LAST, nullptr, 0},
    {"Select & Replay", MenuAction::RF_REPLAY_SELECT, nullptr, 0},
    {"List Signals", MenuAction::RF_LIST_SIGNALS, nullptr, 0},
    {"Clear All", MenuAction::RF_CLEAR_SIGNALS, nullptr, 0},
    {"< Back", MenuAction::BACK, nullptr, 0}
};

// RF Attack submenu
const MenuItem rfAttackMenu[] = {
    {"Jammer", MenuAction::RF_JAMMER, nullptr, 0},
    {"Brute Force", MenuAction::RF_BRUTEFORCE, nullptr, 0},
    {"Tesla Charge", MenuAction::RF_TESLA_CHARGE, nullptr, 0},
    {"Test Signal", MenuAction::RF_TEST_SIGNAL, nullptr, 0},
    {"< Back", MenuAction::BACK, nullptr, 0}
};

// RF Settings submenu
const MenuItem rfSettingsMenu[] = {
    {"Set Frequency", MenuAction::RF_SET_FREQ, nullptr, 0},
    {"Presets >", MenuAction::SUBMENU, rfPresetMenu, 5},
    {"< Back", MenuAction::BACK, nullptr, 0}
};

// RF Preset submenu
const MenuItem rfPresetMenu[] = {
    {"315 MHz (US)", MenuAction::RF_SET_PRESET, nullptr, 0},
    {"433 MHz (EU)", MenuAction::RF_SET_PRESET, nullptr, 0},
    {"868 MHz (EU)", MenuAction::RF_SET_PRESET, nullptr, 0},
    {"915 MHz (US)", MenuAction::RF_SET_PRESET, nullptr, 0},
    {"< Back", MenuAction::BACK, nullptr, 0}
};

// Main RF submenu
const MenuItem rfMenu[] = {
    {"Freq Scanner", MenuAction::RF_SCAN_FREQ, nullptr, 0},
    {"Analyzer", MenuAction::RF_ANALYZER, nullptr, 0},
    {"Capture >", MenuAction::SUBMENU, rfCaptureMenu, 6},
    {"Attack >", MenuAction::SUBMENU, rfAttackMenu, 5},
    {"Settings >", MenuAction::SUBMENU, rfSettingsMenu, 3},
    {"< Back", MenuAction::BACK, nullptr, 0}
};

// Updated main menu
const MenuItem mainMenu[] = {
    {"WiFi", MenuAction::SUBMENU, wifiMenu, 6},
    {"Bluetooth", MenuAction::SUBMENU, btMenu, 6},
#ifdef HAS_CC1101
    {"Sub-GHz", MenuAction::SUBMENU, rfMenu, 6},
#endif
    {"Targets", MenuAction::SUBMENU, targetsMenu, 7},
    {"Settings", MenuAction::SUBMENU, settingsMenu, 2},
    {"Reboot", MenuAction::REBOOT, nullptr, 0}
};
```

### 3.3 Updated Menu Tree

```
ESP32 Marauder TUI v2.0
├── WiFi
│   └── ... (existing)
├── Bluetooth
│   └── ... (existing)
├── Sub-GHz [NEW]
│   ├── Freq Scanner
│   ├── Analyzer
│   ├── Capture >
│   │   ├── Capture Signal
│   │   ├── Replay Last
│   │   ├── Select & Replay
│   │   ├── List Signals
│   │   └── Clear All
│   ├── Attack >
│   │   ├── Jammer
│   │   ├── Brute Force
│   │   ├── Tesla Charge
│   │   └── Test Signal
│   └── Settings >
│       ├── Set Frequency
│       └── Presets >
│           ├── 315 MHz (US)
│           ├── 433 MHz (EU)
│           ├── 868 MHz (EU)
│           └── 915 MHz (US)
├── Targets
│   └── ... (existing)
├── Settings
│   └── ... (existing)
└── Reboot
```

---

## Phase 4: Main Integration (`main.cpp`)

### 4.1 Global Instance

```cpp
#ifdef HAS_CC1101
#include "RFAttacks.h"
RFAttacks rfAttacks;
#endif
```

### 4.2 Action Handler Extensions

Add to `handleAction()`:

```cpp
#ifdef HAS_CC1101
    case MenuAction::RF_SCAN_FREQ:
        Serial.println(F("\n[*] Starting Frequency Scanner..."));
        rfAttacks.frequencyScan(300.0, 928.0, 0.5);
        break;
        
    case MenuAction::RF_ANALYZER:
        Serial.println(F("\n[*] Starting Frequency Analyzer..."));
        Serial.println(F("[!] Press any key to stop"));
        rfAttacks.frequencyAnalyzer();
        break;
        
    case MenuAction::RF_CAPTURE:
        Serial.println(F("\n[*] Waiting for signal... (10s timeout)"));
        if (rfAttacks.captureSignal(10000)) {
            Serial.println(F("[+] Signal captured!"));
        } else {
            Serial.println(F("[-] No signal detected"));
        }
        break;
        
    case MenuAction::RF_REPLAY_LAST:
        Serial.println(F("\n[*] Replaying last signal..."));
        rfAttacks.replayLastSignal();
        break;
        
    case MenuAction::RF_JAMMER:
        if (rfAttacks.isRunning()) {
            rfAttacks.stop();
            Serial.println(F("[*] Jammer stopped"));
        } else {
            Serial.println(F("\n[!] Starting jammer on current frequency"));
            rfAttacks.jammerStart(rfAttacks.getFrequency());
        }
        break;
        
    case MenuAction::RF_TESLA_CHARGE:
        Serial.println(F("\n[*] Sending Tesla charge port signal..."));
        rfAttacks.teslaChargePortOpen();
        break;
#endif
```

---

## Phase 5: TUI Enhancements

### 5.1 Status Bar Update

Show current RF frequency in status bar when CC1101 is active:

```cpp
void SerialTUI::drawStatusBar() {
    Serial.print(ANSI::FG_GRAY);
    Serial.print(F("WiFi CH:"));
    Serial.print(currentChannel);
    
#ifdef HAS_CC1101
    if (cc1101.isInitialized()) {
        Serial.print(F(" | RF:"));
        Serial.print(cc1101.getFrequency(), 2);
        Serial.print(F("MHz"));
    }
#endif
    
    Serial.println(ANSI::RESET);
}
```

### 5.2 Signal Strength Visualization

For frequency analyzer:

```cpp
void printRSSIBar(int8_t rssi) {
    // Map RSSI (-120 to -30) to bar length (0 to 20)
    int barLen = map(constrain(rssi, -120, -30), -120, -30, 0, 20);
    Serial.print(F("["));
    for (int i = 0; i < 20; i++) {
        Serial.print(i < barLen ? '#' : '-');
    }
    Serial.print(F("] "));
    Serial.print(rssi);
    Serial.println(F(" dBm"));
}
```

---

## Phase 6: File Structure

After implementation, the project structure will be:

```
marauder_tui/
├── platformio.ini          # Updated with CC1101 library
├── README.md               # Updated documentation
└── src/
    ├── main.cpp            # Updated with RF actions
    ├── Config.h            # Updated with CC1101 pins
    ├── MenuDefs.h          # Updated with RF menus
    ├── SerialTUI.h         # Optional status bar update
    ├── SerialTUI.cpp
    ├── WiFiAttacks.h
    ├── WiFiAttacks.cpp
    ├── BTAttacks.h
    ├── BTAttacks.cpp
    ├── CC1101Radio.h       # NEW - Hardware abstraction
    ├── CC1101Radio.cpp     # NEW
    ├── RFAttacks.h         # NEW - Attack implementations
    └── RFAttacks.cpp       # NEW
```

---

## Phase 7: Implementation Timeline

| Phase | Description | Estimated Effort | Dependencies |
|-------|-------------|------------------|--------------|
| **1** | Foundation & Hardware Abstraction | 2-3 hours | CC1101 module |
| **2** | Core RF Attacks | 4-6 hours | Phase 1 |
| **3** | Menu Integration | 1-2 hours | Phase 2 |
| **4** | Main Integration | 1 hour | Phase 3 |
| **5** | TUI Enhancements | 1 hour | Phase 4 |
| **6** | Testing & Debugging | 2-4 hours | All phases |

**Total Estimated Time**: 11-17 hours

---

## Phase 8: Testing Checklist

- [ ] CC1101 module initializes successfully
- [ ] Frequency can be changed via menu
- [ ] Signal capture works on test remote
- [ ] Signal replay triggers target device
- [ ] Frequency scanner detects active signals
- [ ] Jammer disrupts target frequency
- [ ] Menu navigation works correctly
- [ ] No memory leaks during extended operation
- [ ] Works alongside WiFi/BT attacks
- [ ] Compiles with `HAS_CC1101` undefined (graceful disable)

---

## Optional Enhancements (Future)

### Advanced Features
1. **Rolling Code Detection** - Detect and warn about rolling code systems
2. **Protocol Library** - Pre-built profiles for common devices (garage doors, doorbells)
3. **Scheduled Transmission** - Time-based signal replay
4. **Signal Recording to SPIFFS** - Persistent signal storage
5. **Dual CC1101 Support** - Simultaneous RX/TX like Evil Crow RF

### Protocol Support
- 📻 Generic OOK (most common)
- 🚪 Garage doors (Linear, Chamberlain, etc.)
- 🔔 Wireless doorbells
- 🚗 Car key fobs (read-only, educational)
- ⚡ Tesla charge port (315 MHz, specific encoding)
- 🌡️ Weather stations
- 🏠 Home automation (433 MHz switches)

---

## Legal Disclaimer

> ⚠️ **WARNING**: RF transmission without proper licensing may be illegal in your jurisdiction. This tool is intended for:
> - Security research on your own devices
> - Educational purposes
> - Authorized penetration testing
> 
> Always ensure you have permission before testing on any device or frequency.

---

## References

- [SmartRC-CC1101 Library](https://github.com/LSatan/SmartRC-CC1101-Driver-Lib)
- [CC1101 Datasheet](https://www.ti.com/lit/ds/symlink/cc1101.pdf)
- [ESP32 Marauder Original](https://github.com/justcallmekoko/ESP32Marauder)
- [Flipper Zero Sub-GHz Protocol](https://github.com/flipperdevices/flipperzero-firmware/tree/dev/lib/subghz)

---

*Plan created: 2026-01-16*
*Target Version: ESP32 Marauder TUI v2.0*
