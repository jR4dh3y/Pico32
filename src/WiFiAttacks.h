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
    
private:
    uint8_t _channel = DEFAULT_CHANNEL;
    uint32_t _lastUpdate = 0;
    
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
};

extern WiFiAttacks wifiAttacks;
