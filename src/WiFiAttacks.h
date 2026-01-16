#pragma once

#include <Arduino.h>
#include "Config.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <LinkedList.h>

// ============================================
// WiFi Attack Module
// Ported from ESP32 Marauder WiFiScan
// ============================================

// 802.11 Frame Types
#define WIFI_FRAME_TYPE_MGMT    0x00
#define WIFI_FRAME_TYPE_CTRL    0x04
#define WIFI_FRAME_TYPE_DATA    0x08

// 802.11 Management Frame Subtypes
#define WIFI_MGMT_ASSOC_REQ     0x00
#define WIFI_MGMT_ASSOC_RESP    0x10
#define WIFI_MGMT_REASSOC_REQ   0x20
#define WIFI_MGMT_REASSOC_RESP  0x30
#define WIFI_MGMT_PROBE_REQ     0x40
#define WIFI_MGMT_PROBE_RESP    0x50
#define WIFI_MGMT_BEACON        0x80
#define WIFI_MGMT_DISASSOC      0xA0
#define WIFI_MGMT_AUTH          0xB0
#define WIFI_MGMT_DEAUTH        0xC0

// Access Point structure
struct AccessPoint {
    String essid;
    uint8_t bssid[6];
    uint8_t channel;
    int8_t rssi;
    bool selected;
};

// Station structure  
struct Station {
    uint8_t mac[6];
    uint8_t bssid[6];  // Associated AP
    int8_t rssi;
    bool selected;
};

// SSID for beacon spam
struct SSID {
    String name;
    bool selected;
};

// Current scan/attack mode
enum class WiFiMode : uint8_t {
    IDLE,
    SCAN_AP,
    SCAN_STATION,
    SNIFF_BEACON,
    SNIFF_PROBE,
    SNIFF_DEAUTH,
    SNIFF_PMKID,
    SNIFF_PWN,
    SNIFF_RAW,
    ATTACK_DEAUTH,
    ATTACK_BEACON_RANDOM,
    ATTACK_BEACON_LIST,
    ATTACK_RICKROLL,
    ATTACK_FUNNY
};

class WiFiAttacks {
public:
    void begin();
    void update();
    void stop();
    
    // Scanning
    void startScanAP();
    void startScanStation();
    void startSniffBeacon();
    void startSniffProbe();
    void startSniffDeauth();
    void startSniffPMKID();
    void startSniffPwn();
    void startSniffRaw();
    
    // Attacks
    void startDeauth();
    void startBeaconRandom();
    void startBeaconList();
    void startRickRoll();
    void startFunnyBeacon();
    
    // Channel
    void setChannel(uint8_t channel);
    uint8_t getChannel() const { return _channel; }
    
    // Target management
    LinkedList<AccessPoint>* getAPs() { return &_accessPoints; }
    LinkedList<Station>* getStations() { return &_stations; }
    LinkedList<SSID>* getSSIDs() { return &_ssids; }
    
    void selectAP(uint8_t index, bool selected);
    void addSSID(const char* ssid);
    void clearAll();
    
    bool isActive() const { return _mode != WiFiMode::IDLE; }
    
    // Public for callback access
    WiFiMode _mode = WiFiMode::IDLE;
    uint32_t _packetCount = 0;
    
    // Promiscuous callback handler (called from static callback)
    void handlePacket(void* buf, wifi_promiscuous_pkt_type_t type);
    
private:
    uint8_t _channel = DEFAULT_CHANNEL;
    uint32_t _lastUpdate = 0;
    
    // Channel hopping
    bool _channelHop = false;
    uint8_t _hopChannel = 1;
    uint32_t _lastHopTime = 0;
    static const uint32_t CHANNEL_HOP_INTERVAL = 500;  // ms per channel
    
    LinkedList<AccessPoint> _accessPoints;
    LinkedList<Station> _stations;
    LinkedList<SSID> _ssids;
    
    // Internal methods
    void sendDeauthToAll();
    void sendDeauthFrame(const uint8_t* ap, const uint8_t* client, uint8_t channel);
    void sendRandomBeacon();
    void sendListBeacon();
    void sendBeaconFrame(const char* ssid, uint8_t channel);
    String macToString(const uint8_t* mac);
    
    // Promiscuous mode helpers
    void startPromiscuous(bool channelHop);
    void stopPromiscuous();
    void handleChannelHop();
    
    // Frame parsing
    void parseBeaconFrame(const uint8_t* payload, int len, int rssi);
    void parseProbeRequest(const uint8_t* payload, int len, int rssi);
    void parseDeauthFrame(const uint8_t* payload, int len, int rssi);
    void parseEAPOL(const uint8_t* payload, int len, int rssi);
    void parsePwnagotchi(const uint8_t* payload, int len, int rssi);
    bool addStation(const uint8_t* mac, const uint8_t* bssid, int8_t rssi);
};

extern WiFiAttacks wifiAttacks;

