/**
 * ESP32 Marauder TUI - Main Entry Point
 * 
 * Stripped-down menu-based TUI for WiFi/Bluetooth attacks
 * Target: ESP32 Dev Module (no PSRAM, no display, no SD)
 */

#include <Arduino.h>
#include "Config.h"
#include "SerialTUI.h"
#include "WiFiAttacks.h"
#include "BTAttacks.h"


/**
 * Handle menu action from TUI
 */
void handleAction(MenuAction action) {
    switch (action) {
        // WiFi Scans
        case MenuAction::WIFI_SCAN_AP:
            tui.printStatus("Starting AP scan...");
            tui.setScanning(true);
            wifiAttacks.startScanAP();
            break;
            
        case MenuAction::WIFI_SCAN_STA:
            tui.printStatus("Starting station scan...");
            tui.setScanning(true);
            wifiAttacks.startScanStation();
            break;
            
        // WiFi Sniff
        case MenuAction::WIFI_SNIFF_BEACON:
            tui.printStatus("Sniffing beacon frames...");
            tui.setScanning(true);
            wifiAttacks.startSniffBeacon();
            break;
            
        case MenuAction::WIFI_SNIFF_PROBE:
            tui.printStatus("Sniffing probe requests...");
            tui.setScanning(true);
            wifiAttacks.startSniffProbe();
            break;
            
        case MenuAction::WIFI_SNIFF_DEAUTH:
            tui.printStatus("Sniffing deauth packets...");
            tui.setScanning(true);
            wifiAttacks.startSniffDeauth();
            break;
            
        case MenuAction::WIFI_SNIFF_PMKID:
            tui.printStatus("Sniffing PMKID/EAPOL...");
            tui.setScanning(true);
            wifiAttacks.startSniffPMKID();
            break;
            
        case MenuAction::WIFI_SNIFF_PWN:
            tui.printStatus("Scanning for Pwnagotchi...");
            tui.setScanning(true);
            wifiAttacks.startSniffPwn();
            break;
            
        case MenuAction::WIFI_SNIFF_RAW:
            tui.printStatus("Raw packet capture...");
            tui.setScanning(true);
            wifiAttacks.startSniffRaw();
            break;
            
        // WiFi Attacks
        case MenuAction::WIFI_ATTACK_DEAUTH:
            {
                auto* aps = wifiAttacks.getAPs();
                if (aps->size() == 0) {
                    tui.printError("No targets! Scan APs first.");
                    break;
                }
                // Check if any APs are selected
                int selectedCount = 0;
                for (int i = 0; i < aps->size(); i++) {
                    if (aps->get(i).selected) selectedCount++;
                }
                if (selectedCount == 0) {
                    tui.printError("No APs selected! Use Targets > Select/Deselect first.");
                    break;
                }
                char buf[48];
                snprintf(buf, sizeof(buf), "Deauth attack on %d APs...", selectedCount);
                tui.printStatus(buf);
                tui.setScanning(true);
                wifiAttacks.startDeauth();
            }
            break;
            
        case MenuAction::WIFI_ATTACK_BEACON_RANDOM:
            tui.printStatus("Beacon spam (random)...");
            tui.setScanning(true);
            wifiAttacks.startBeaconRandom();
            break;
            
        case MenuAction::WIFI_ATTACK_BEACON_LIST:
            if (wifiAttacks.getSSIDs()->size() == 0) {
                tui.printError("No SSIDs! Add some first.");
                break;
            }
            tui.printStatus("Beacon spam (list)...");
            tui.setScanning(true);
            wifiAttacks.startBeaconList();
            break;
            
        case MenuAction::WIFI_ATTACK_RICKROLL:
            tui.printStatus("Rick Roll activated!");
            tui.setScanning(true);
            wifiAttacks.startRickRoll();
            break;
            
        case MenuAction::WIFI_ATTACK_FUNNY:
            tui.printStatus("Funny beacons go brrr...");
            tui.setScanning(true);
            wifiAttacks.startFunnyBeacon();
            break;
            
        case MenuAction::WIFI_SET_CHANNEL:
            // Cycle channel 1-14
            {
                uint8_t ch = (wifiAttacks.getChannel() % 14) + 1;
                wifiAttacks.setChannel(ch);
                char buf[32];
                snprintf(buf, sizeof(buf), "Channel set to %d", ch);
                tui.printStatus(buf);
            }
            break;
            
        // Bluetooth Scans
        case MenuAction::BT_SCAN_ALL:
            tui.printStatus("BT scan all devices...");
            tui.setScanning(true);
            btAttacks.startScanAll();
            break;
            
        case MenuAction::BT_SCAN_AIRTAG:
            tui.printStatus("Scanning for AirTags...");
            tui.setScanning(true);
            btAttacks.startScanAirtag();
            break;
            
        case MenuAction::BT_SCAN_FLIPPER:
            tui.printStatus("Scanning for Flippers...");
            tui.setScanning(true);
            btAttacks.startScanFlipper();
            break;
            
        case MenuAction::BT_SCAN_SKIMMER:
            tui.printStatus("Detecting card skimmers...");
            tui.setScanning(true);
            btAttacks.startScanSkimmer();
            break;
            
        // Bluetooth Spam
        case MenuAction::BT_SPAM_APPLE:
            tui.printStatus("Sour Apple spam...");
            tui.setScanning(true);
            btAttacks.startSpamApple();
            break;
            
        case MenuAction::BT_SPAM_WINDOWS:
            tui.printStatus("SwiftPair spam...");
            tui.setScanning(true);
            btAttacks.startSpamWindows();
            break;
            
        case MenuAction::BT_SPAM_SAMSUNG:
            tui.printStatus("Samsung spam...");
            tui.setScanning(true);
            btAttacks.startSpamSamsung();
            break;
            
        case MenuAction::BT_SPAM_GOOGLE:
            tui.printStatus("Google FastPair spam...");
            tui.setScanning(true);
            btAttacks.startSpamGoogle();
            break;
            
        case MenuAction::BT_SPAM_ALL:
            tui.printStatus("All BT spam enabled...");
            tui.setScanning(true);
            btAttacks.startSpamAll();
            break;
            
        // Target Management
        case MenuAction::TARGETS_LIST_AP:
            {
                auto* aps = wifiAttacks.getAPs();
                char buf[64];
                snprintf(buf, sizeof(buf), "APs: %d", aps->size());
                tui.printStatus(buf);
                for (int i = 0; i < aps->size() && i < 10; i++) {
                    AccessPoint ap = aps->get(i);
                    snprintf(buf, sizeof(buf), "%s%s [%d] %ddBm", 
                             ap.selected ? "*" : " ",
                             ap.essid.c_str(),
                             ap.channel,
                             ap.rssi);
                    tui.printResult(buf);
                }
            }
            break;
            
        case MenuAction::TARGETS_LIST_STA:
            {
                auto* stas = wifiAttacks.getStations();
                char buf[64];
                snprintf(buf, sizeof(buf), "Stations: %d", stas->size());
                tui.printStatus(buf);
            }
            break;
            
        case MenuAction::TARGETS_LIST_SSID:
            {
                auto* ssids = wifiAttacks.getSSIDs();
                char buf[64];
                snprintf(buf, sizeof(buf), "SSIDs: %d", ssids->size());
                tui.printStatus(buf);
                for (int i = 0; i < ssids->size() && i < 10; i++) {
                    SSID ssid = ssids->get(i);
                    tui.printResult(ssid.name.c_str());
                }
            }
            break;
            
        case MenuAction::TARGETS_SELECT:
            {
                auto* aps = wifiAttacks.getAPs();
                if (aps->size() == 0) {
                    tui.printError("No APs scanned! Run WiFi > Scan APs first.");
                    break;
                }
                // Enter interactive AP selection mode
                tui.enterAPSelectionMode();
            }
            break;
            
        case MenuAction::TARGETS_ADD_SSID:
            {
                // Check if coming from text input mode with a buffer
                const char* inputBuf = tui.getInputBuffer();
                if (inputBuf && inputBuf[0] != '\0') {
                    // Got input from text mode - add the SSID
                    wifiAttacks.addSSID(inputBuf);
                    char buf[48];
                    snprintf(buf, sizeof(buf), "Added SSID: %s", inputBuf);
                    tui.printStatus(buf);
                } else {
                    // Enter text input mode to get SSID
                    tui.enterTextInputMode("Enter SSID name (max 32 chars):");
                }
            }
            break;
            
        case MenuAction::TARGETS_CLEAR:
            wifiAttacks.clearAll();
            tui.printStatus("All targets cleared");
            break;
            
        case MenuAction::SETTINGS_CHANNEL:
            {
                uint8_t ch = (wifiAttacks.getChannel() % 14) + 1;
                wifiAttacks.setChannel(ch);
                char buf[32];
                snprintf(buf, sizeof(buf), "Channel: %d", ch);
                tui.printStatus(buf);
            }
            break;
            
        case MenuAction::REBOOT:
            tui.printStatus("Rebooting...");
            delay(500);
            ESP.restart();
            break;
            
        case MenuAction::BACK:
            // Signal to stop current operation
            wifiAttacks.stop();
            btAttacks.stop();
            tui.setScanning(false);
            break;
            
        default:
            break;
    }
    
    tui.clearPendingAction();
}

void setup() {
    // Initialize TUI
    tui.begin();
    
    // Welcome message
    Serial.println();
    tui.printStatus("Initializing...");
    
    // Initialize WiFi
    wifiAttacks.begin();
    tui.printStatus("WiFi ready");
    
    // Initialize Bluetooth
    btAttacks.begin();
    tui.printStatus("Bluetooth ready");
    
    // Show free heap
    char buf[64];
    snprintf(buf, sizeof(buf), "Free heap: %d bytes", ESP.getFreeHeap());
    tui.printStatus(buf);
    
    delay(1500);
}

void loop() {
    // Update TUI
    tui.update();
    
    // Check for pending action
    MenuAction action = tui.getPendingAction();
    if (action != MenuAction::NONE) {
        handleAction(action);
    }
    
    // Update attacks if running
    if (wifiAttacks.isActive()) {
        wifiAttacks.update();
        
        // Check for stop signal
        if (tui.getPendingAction() == MenuAction::BACK) {
            wifiAttacks.stop();
            tui.setScanning(false);
            tui.clearPendingAction();
        }
    }
    
    if (btAttacks.isActive()) {
        btAttacks.update();
        
        // Check for stop signal
        if (tui.getPendingAction() == MenuAction::BACK) {
            btAttacks.stop();
            tui.setScanning(false);
            tui.clearPendingAction();
        }
    }
    
    delay(10);
}
