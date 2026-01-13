#include "SerialTUI.h"

SerialTUI tui;

void SerialTUI::begin() {
    Serial.begin(SERIAL_BAUD);
    while (!Serial) delay(10);
    
    // Clear screen and show menu
    Serial.print(ANSI::CLEAR_SCREEN);
    Serial.print(ANSI::CURSOR_HOME);
    Serial.print(ANSI::CURSOR_HIDE);
    
    _needsRedraw = true;
}

void SerialTUI::update() {
    handleInput();
    
    if (_needsRedraw && !_scanning) {
        render();
        _needsRedraw = false;
    }
}

void SerialTUI::setScanning(bool scanning) {
    _scanning = scanning;
    if (!scanning) {
        _needsRedraw = true;
        Serial.print(ANSI::CURSOR_HIDE);
    }
}

void SerialTUI::printResult(const char* result) {
    Serial.print(ANSI::FG_GREEN);
    Serial.print("[+] ");
    Serial.print(ANSI::RESET);
    Serial.println(result);
}

void SerialTUI::printStatus(const char* status) {
    Serial.print(ANSI::FG_CYAN);
    Serial.print("[*] ");
    Serial.print(ANSI::RESET);
    Serial.println(status);
}

void SerialTUI::printError(const char* error) {
    Serial.print(ANSI::FG_RED);
    Serial.print("[!] ");
    Serial.print(ANSI::RESET);
    Serial.println(error);
}

MenuAction SerialTUI::getPendingAction() {
    return _pendingAction;
}

void SerialTUI::clearPendingAction() {
    _pendingAction = MenuAction::NONE;
}

// ============================================
// Rendering
// ============================================

void SerialTUI::render() {
    Serial.print(ANSI::CLEAR_SCREEN);
    Serial.print(ANSI::CURSOR_HOME);
    
    renderHeader();
    renderMenu();
    renderFooter();
}

void SerialTUI::renderHeader() {
    Serial.print(ANSI::FG_CYAN);
    Serial.print(ANSI::BOLD);
    Serial.println("========================================");
    Serial.println("       ESP32 Marauder TUI v" VERSION);
    Serial.println("       WiFi/BT Attack Platform");
    Serial.println("========================================");
    Serial.print(ANSI::RESET);
    Serial.println();
}

void SerialTUI::renderMenu() {
    for (uint8_t i = 0; i < _currentMenuSize; i++) {
        if (i == _selectedIndex) {
            // Highlighted item
            Serial.print(ANSI::HIGHLIGHT);
            Serial.print(ANSI::FG_CYAN);
            Serial.print(" > ");
        } else {
            Serial.print("   ");
        }
        
        // Color based on item type
        if (_currentMenu[i].action == MenuAction::BACK) {
            Serial.print(ANSI::FG_GRAY);
        } else if (_currentMenu[i].submenu != nullptr) {
            Serial.print(ANSI::FG_WHITE);
        } else if (_currentMenu[i].action >= MenuAction::WIFI_ATTACK_DEAUTH && 
                   _currentMenu[i].action <= MenuAction::WIFI_ATTACK_FUNNY) {
            Serial.print(ANSI::FG_RED);
        } else if (_currentMenu[i].action >= MenuAction::BT_SPAM_APPLE && 
                   _currentMenu[i].action <= MenuAction::BT_SPAM_ALL) {
            Serial.print(ANSI::FG_RED);
        }
        
        Serial.print(_currentMenu[i].label);
        Serial.print(ANSI::RESET);
        Serial.println();
    }
    Serial.println();
}

void SerialTUI::renderFooter() {
    Serial.print(ANSI::FG_GRAY);
    Serial.println("----------------------------------------");
    Serial.println(" [W/S] or [Arrows] Navigate");
    Serial.println(" [Enter] Select    [Q/Esc] Back");
    Serial.print(ANSI::RESET);
}

// ============================================
// Input Handling
// ============================================

void SerialTUI::handleInput() {
    while (Serial.available()) {
        char c = Serial.read();
        
        // During scanning, any key stops
        if (_scanning) {
            _pendingAction = MenuAction::BACK;  // Signal to stop
            return;
        }
        
        // Escape sequence handling for arrow keys
        // ESC [ A = Up, ESC [ B = Down, ESC [ C = Right, ESC [ D = Left
        if (_escapeState == 0 && c == 27) {  // ESC
            _escapeState = 1;
            _lastEscapeTime = millis();
        } else if (_escapeState == 1) {
            if (c == '[') {
                _escapeState = 2;
            } else {
                // Just ESC pressed
                goBack();
                _escapeState = 0;
            }
        } else if (_escapeState == 2) {
            processArrowKey(c);
            _escapeState = 0;
        } else {
            processKey(c);
        }
    }
    
    // Timeout escape sequence
    if (_escapeState > 0 && millis() - _lastEscapeTime > 50) {
        if (_escapeState == 1) goBack();  // Just ESC
        _escapeState = 0;
    }
}

void SerialTUI::processKey(char key) {
    switch (key) {
        case 'w':
        case 'W':
            if (_selectedIndex > 0) {
                _selectedIndex--;
                _needsRedraw = true;
            }
            break;
            
        case 's':
        case 'S':
            if (_selectedIndex < _currentMenuSize - 1) {
                _selectedIndex++;
                _needsRedraw = true;
            }
            break;
            
        case '\r':
        case '\n':
            selectItem();
            break;
            
        case 'q':
        case 'Q':
            goBack();
            break;
    }
}

void SerialTUI::processArrowKey(char direction) {
    switch (direction) {
        case 'A':  // Up
            if (_selectedIndex > 0) {
                _selectedIndex--;
                _needsRedraw = true;
            }
            break;
            
        case 'B':  // Down
            if (_selectedIndex < _currentMenuSize - 1) {
                _selectedIndex++;
                _needsRedraw = true;
            }
            break;
            
        case 'C':  // Right - select
            selectItem();
            break;
            
        case 'D':  // Left - back
            goBack();
            break;
    }
}

void SerialTUI::selectItem() {
    const MenuItem& item = _currentMenu[_selectedIndex];
    
    if (item.action == MenuAction::SUBMENU && item.submenu != nullptr) {
        pushMenu(item.submenu, item.submenuSize);
    } else if (item.action == MenuAction::BACK) {
        goBack();
    } else {
        _pendingAction = item.action;
    }
    
    _needsRedraw = true;
}

void SerialTUI::goBack() {
    if (_menuStackDepth > 0) {
        popMenu();
        _needsRedraw = true;
    }
}

void SerialTUI::pushMenu(const MenuItem* menu, uint8_t size) {
    if (_menuStackDepth < 5) {
        _menuStack[_menuStackDepth] = {_currentMenu, _currentMenuSize, _selectedIndex};
        _menuStackDepth++;
        _currentMenu = menu;
        _currentMenuSize = size;
        _selectedIndex = 0;
    }
}

void SerialTUI::popMenu() {
    if (_menuStackDepth > 0) {
        _menuStackDepth--;
        _currentMenu = _menuStack[_menuStackDepth].menu;
        _currentMenuSize = _menuStack[_menuStackDepth].size;
        _selectedIndex = _menuStack[_menuStackDepth].selected;
    }
}
