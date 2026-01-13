#pragma once

#include <Arduino.h>

// ============================================
// ANSI Escape Codes for TUI Rendering
// ============================================

namespace ANSI {
    // Cursor control
    constexpr const char* CLEAR_SCREEN = "\033[2J";
    constexpr const char* CURSOR_HOME = "\033[H";
    constexpr const char* CURSOR_HIDE = "\033[?25l";
    constexpr const char* CURSOR_SHOW = "\033[?25h";
    constexpr const char* CLEAR_LINE = "\033[2K";
    
    // Colors (tasteful palette)
    constexpr const char* RESET = "\033[0m";
    constexpr const char* BOLD = "\033[1m";
    constexpr const char* DIM = "\033[2m";
    
    // Foreground
    constexpr const char* FG_RED = "\033[31m";
    constexpr const char* FG_GREEN = "\033[32m";
    constexpr const char* FG_YELLOW = "\033[33m";
    constexpr const char* FG_CYAN = "\033[36m";
    constexpr const char* FG_WHITE = "\033[37m";
    constexpr const char* FG_GRAY = "\033[90m";
    
    // Special
    constexpr const char* HIGHLIGHT = "\033[7m";  // Inverse
}

// ============================================
// Menu Item Types
// ============================================

enum class MenuAction : uint8_t {
    NONE,
    SUBMENU,
    WIFI_SCAN_AP,
    WIFI_SCAN_STA,
    WIFI_SNIFF_BEACON,
    WIFI_SNIFF_PROBE,
    WIFI_SNIFF_DEAUTH,
    WIFI_SNIFF_PMKID,
    WIFI_SNIFF_PWN,
    WIFI_SNIFF_RAW,
    WIFI_ATTACK_DEAUTH,
    WIFI_ATTACK_BEACON_RANDOM,
    WIFI_ATTACK_BEACON_LIST,
    WIFI_ATTACK_RICKROLL,
    WIFI_ATTACK_FUNNY,
    WIFI_SET_CHANNEL,
    BT_SCAN_ALL,
    BT_SCAN_AIRTAG,
    BT_SCAN_FLIPPER,
    BT_SCAN_SKIMMER,
    BT_SPAM_APPLE,
    BT_SPAM_WINDOWS,
    BT_SPAM_SAMSUNG,
    BT_SPAM_GOOGLE,
    BT_SPAM_ALL,
    TARGETS_LIST_AP,
    TARGETS_LIST_STA,
    TARGETS_LIST_SSID,
    TARGETS_SELECT,
    TARGETS_ADD_SSID,
    TARGETS_CLEAR,
    SETTINGS_CHANNEL,
    REBOOT,
    BACK
};

// ============================================
// Menu Item Structure
// ============================================

struct MenuItem {
    const char* label;
    MenuAction action;
    const MenuItem* submenu;
    uint8_t submenuSize;
};

// ============================================
// Menu Definitions
// ============================================

// WiFi Sniff submenu
const MenuItem wifiSniffMenu[] = {
    {"Beacon Frames", MenuAction::WIFI_SNIFF_BEACON, nullptr, 0},
    {"Probe Requests", MenuAction::WIFI_SNIFF_PROBE, nullptr, 0},
    {"Deauth Packets", MenuAction::WIFI_SNIFF_DEAUTH, nullptr, 0},
    {"PMKID/EAPOL", MenuAction::WIFI_SNIFF_PMKID, nullptr, 0},
    {"Pwnagotchi", MenuAction::WIFI_SNIFF_PWN, nullptr, 0},
    {"Raw Packets", MenuAction::WIFI_SNIFF_RAW, nullptr, 0},
    {"< Back", MenuAction::BACK, nullptr, 0}
};

// WiFi Attack submenu
const MenuItem wifiAttackMenu[] = {
    {"Deauth Selected", MenuAction::WIFI_ATTACK_DEAUTH, nullptr, 0},
    {"Beacon Random", MenuAction::WIFI_ATTACK_BEACON_RANDOM, nullptr, 0},
    {"Beacon List", MenuAction::WIFI_ATTACK_BEACON_LIST, nullptr, 0},
    {"Rick Roll", MenuAction::WIFI_ATTACK_RICKROLL, nullptr, 0},
    {"Funny SSIDs", MenuAction::WIFI_ATTACK_FUNNY, nullptr, 0},
    {"< Back", MenuAction::BACK, nullptr, 0}
};

// WiFi main submenu
const MenuItem wifiMenu[] = {
    {"Scan APs", MenuAction::WIFI_SCAN_AP, nullptr, 0},
    {"Scan Stations", MenuAction::WIFI_SCAN_STA, nullptr, 0},
    {"Sniff >", MenuAction::SUBMENU, wifiSniffMenu, 7},
    {"Attack >", MenuAction::SUBMENU, wifiAttackMenu, 6},
    {"Set Channel", MenuAction::WIFI_SET_CHANNEL, nullptr, 0},
    {"< Back", MenuAction::BACK, nullptr, 0}
};

// Bluetooth Spam submenu
const MenuItem btSpamMenu[] = {
    {"Apple (Sour)", MenuAction::BT_SPAM_APPLE, nullptr, 0},
    {"Windows", MenuAction::BT_SPAM_WINDOWS, nullptr, 0},
    {"Samsung", MenuAction::BT_SPAM_SAMSUNG, nullptr, 0},
    {"Google", MenuAction::BT_SPAM_GOOGLE, nullptr, 0},
    {"All Spam", MenuAction::BT_SPAM_ALL, nullptr, 0},
    {"< Back", MenuAction::BACK, nullptr, 0}
};

// Bluetooth main submenu
const MenuItem btMenu[] = {
    {"Scan All", MenuAction::BT_SCAN_ALL, nullptr, 0},
    {"Scan Airtags", MenuAction::BT_SCAN_AIRTAG, nullptr, 0},
    {"Scan Flippers", MenuAction::BT_SCAN_FLIPPER, nullptr, 0},
    {"Card Skimmers", MenuAction::BT_SCAN_SKIMMER, nullptr, 0},
    {"Spam >", MenuAction::SUBMENU, btSpamMenu, 6},
    {"< Back", MenuAction::BACK, nullptr, 0}
};

// Targets submenu
const MenuItem targetsMenu[] = {
    {"List APs", MenuAction::TARGETS_LIST_AP, nullptr, 0},
    {"List Stations", MenuAction::TARGETS_LIST_STA, nullptr, 0},
    {"List SSIDs", MenuAction::TARGETS_LIST_SSID, nullptr, 0},
    {"Select/Deselect", MenuAction::TARGETS_SELECT, nullptr, 0},
    {"Add SSID", MenuAction::TARGETS_ADD_SSID, nullptr, 0},
    {"Clear All", MenuAction::TARGETS_CLEAR, nullptr, 0},
    {"< Back", MenuAction::BACK, nullptr, 0}
};

// Settings submenu
const MenuItem settingsMenu[] = {
    {"Channel", MenuAction::SETTINGS_CHANNEL, nullptr, 0},
    {"< Back", MenuAction::BACK, nullptr, 0}
};

// Main menu
const MenuItem mainMenu[] = {
    {"WiFi", MenuAction::SUBMENU, wifiMenu, 6},
    {"Bluetooth", MenuAction::SUBMENU, btMenu, 6},
    {"Targets", MenuAction::SUBMENU, targetsMenu, 7},
    {"Settings", MenuAction::SUBMENU, settingsMenu, 2},
    {"Reboot", MenuAction::REBOOT, nullptr, 0}
};

constexpr uint8_t MAIN_MENU_SIZE = 5;

