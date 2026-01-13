#pragma once

#include <Arduino.h>
#include "Config.h"
#include "MenuDefs.h"

// ============================================
// Serial TUI Engine
// Handles menu rendering and keyboard input
// ============================================

class SerialTUI {
public:
    void begin();
    void update();
    
    // State
    bool isActive() const { return _active; }
    bool isScanning() const { return _scanning; }
    void setScanning(bool scanning);
    
    // Output during scans
    void printResult(const char* result);
    void printStatus(const char* status);
    void printError(const char* error);
    
    // Get current action to execute
    MenuAction getPendingAction();
    void clearPendingAction();
    
private:
    // Menu state
    const MenuItem* _currentMenu = mainMenu;
    uint8_t _currentMenuSize = MAIN_MENU_SIZE;
    uint8_t _selectedIndex = 0;
    
    // Menu stack for navigation (max 5 levels deep)
    struct MenuState {
        const MenuItem* menu;
        uint8_t size;
        uint8_t selected;
    };
    MenuState _menuStack[5];
    uint8_t _menuStackDepth = 0;
    
    // State flags
    bool _active = true;
    bool _scanning = false;
    bool _needsRedraw = true;
    MenuAction _pendingAction = MenuAction::NONE;
    
    // Input handling
    uint8_t _escapeState = 0;
    unsigned long _lastEscapeTime = 0;
    
    // Methods
    void render();
    void renderHeader();
    void renderMenu();
    void renderFooter();
    void handleInput();
    void processKey(char key);
    void processArrowKey(char direction);
    void selectItem();
    void goBack();
    void pushMenu(const MenuItem* menu, uint8_t size);
    void popMenu();
};

// Global instance
extern SerialTUI tui;
