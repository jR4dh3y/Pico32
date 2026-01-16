#include "SerialTUI.h"
#include "WiFiAttacks.h"

SerialTUI tui;

void SerialTUI::begin() {
    Serial.begin(SERIAL_BAUD);
    while (!Serial) delay(10);
    
    // Initialize input buffer
    memset(_inputBuffer, 0, sizeof(_inputBuffer));
    _inputPos = 0;
    _resultCount = 0;
    _lastResultTime = 0;
    
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
        _resultCount = 0;
        Serial.print(ANSI::CURSOR_HIDE);
    }
}

void SerialTUI::printResult(const char* result) {
    // Output throttling - limit to one result per RESULT_THROTTLE_MS
    unsigned long now = millis();
    _resultCount++;
    
    if (now - _lastResultTime < RESULT_THROTTLE_MS) {
        // Throttled - just update counter display periodically
        if (_resultCount % 10 == 0) {
            Serial.print(ANSI::CLEAR_LINE);
            Serial.print("\r");
            Serial.print(ANSI::FG_YELLOW);
            Serial.print("[~] Results: ");
            Serial.print(_resultCount);
            Serial.print(ANSI::RESET);
        }
        return;
    }
    
    _lastResultTime = now;
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
// Interactive Modes
// ============================================

void SerialTUI::enterAPSelectionMode() {
    _inputMode = InputMode::SELECT_AP;
    _needsRedraw = true;
}

void SerialTUI::enterTextInputMode(const char* prompt) {
    _inputMode = InputMode::INPUT_TEXT;
    _inputPrompt = prompt;
    memset(_inputBuffer, 0, sizeof(_inputBuffer));
    _inputPos = 0;
    _needsRedraw = true;
}

void SerialTUI::exitInputMode() {
    _inputMode = InputMode::MENU;
    _inputPrompt = nullptr;
    _needsRedraw = true;
}

// ============================================
// Rendering
// ============================================

void SerialTUI::render() {
    Serial.print(ANSI::CLEAR_SCREEN);
    Serial.print(ANSI::CURSOR_HOME);
    
    renderHeader();
    
    switch (_inputMode) {
        case InputMode::SELECT_AP:
            renderAPSelection();
            break;
        case InputMode::INPUT_TEXT:
            renderTextInput();
            break;
        default:
            renderMenu();
            break;
    }
    
    renderFooter();
}

void SerialTUI::renderHeader() {
    // Top border
    Serial.print(ANSI::FG_GRAY);
    Serial.println("========================================");
    Serial.print(ANSI::RESET);
    
    // PICO   32 ASCII Art
    // Width: 13(PICO) + 3(gap) + 7(32) = 23 chars
    // Center: (40-23)/2 = 8 spaces padding
    Serial.print(ANSI::BOLD);
    
    // Line 1
    Serial.print("        ");
    Serial.print(ANSI::FG_MAGENTA);
    Serial.print("### ");
    Serial.print(ANSI::FG_YELLOW);
    Serial.print("# ");
    Serial.print(ANSI::FG_GREEN);
    Serial.print("### ");
    Serial.print(ANSI::FG_CYAN);
    Serial.print("###   "); // Extra spaces after O
    Serial.print(ANSI::FG_RED);
    Serial.println("### ###");
    
    // Line 2
    Serial.print("        ");
    Serial.print(ANSI::FG_MAGENTA);
    Serial.print("# # ");
    Serial.print(ANSI::FG_YELLOW);
    Serial.print("# ");
    Serial.print(ANSI::FG_GREEN);
    Serial.print("#   ");
    Serial.print(ANSI::FG_CYAN);
    Serial.print("# #   ");
    Serial.print(ANSI::FG_RED);
    Serial.println("  #   #");
    
    // Line 3
    Serial.print("        ");
    Serial.print(ANSI::FG_MAGENTA);
    Serial.print("### ");
    Serial.print(ANSI::FG_YELLOW);
    Serial.print("# ");
    Serial.print(ANSI::FG_GREEN);
    Serial.print("#   ");
    Serial.print(ANSI::FG_CYAN);
    Serial.print("# #   ");
    Serial.print(ANSI::FG_RED);
    Serial.println("### ###");
    
    // Line 4
    Serial.print("        ");
    Serial.print(ANSI::FG_MAGENTA);
    Serial.print("#   ");
    Serial.print(ANSI::FG_YELLOW);
    Serial.print("# ");
    Serial.print(ANSI::FG_GREEN);
    Serial.print("#   ");
    Serial.print(ANSI::FG_CYAN);
    Serial.print("# #   ");
    Serial.print(ANSI::FG_RED);
    Serial.println("  # #  ");
    
    // Line 5
    Serial.print("        ");
    Serial.print(ANSI::FG_MAGENTA);
    Serial.print("#   ");
    Serial.print(ANSI::FG_YELLOW);
    Serial.print("# ");
    Serial.print(ANSI::FG_GREEN);
    Serial.print("### ");
    Serial.print(ANSI::FG_CYAN);
    Serial.print("###   ");
    Serial.print(ANSI::FG_RED);
    Serial.println("### ###");
    
    Serial.print(ANSI::RESET);
    
    // Version subtitle - centered
    Serial.print(ANSI::FG_WHITE);
    Serial.print("         v");
    Serial.print(VERSION);
    Serial.print(ANSI::FG_GRAY);
    Serial.println(" | WiFi/BT Toolkit");
    Serial.print(ANSI::RESET);
    
    // Bottom border
    Serial.print(ANSI::FG_GRAY);
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

void SerialTUI::renderAPSelection() {
    Serial.print(ANSI::FG_YELLOW);
    Serial.print(ANSI::BOLD);
    Serial.println("=== SELECT ACCESS POINTS ===");
    Serial.print(ANSI::RESET);
    Serial.println();
    
    auto* aps = wifiAttacks.getAPs();
    
    if (aps->size() == 0) {
        Serial.print(ANSI::FG_GRAY);
        Serial.println("No APs found. Scan first!");
        Serial.print(ANSI::RESET);
    } else {
        // Count selected
        int selectedCount = 0;
        for (int i = 0; i < aps->size(); i++) {
            if (aps->get(i).selected) selectedCount++;
        }
        
        Serial.print(ANSI::FG_CYAN);
        Serial.print("Selected: ");
        Serial.print(selectedCount);
        Serial.print("/");
        Serial.println(aps->size());
        Serial.print(ANSI::RESET);
        Serial.println();
        
        // Show up to 10 APs (limited by single digit keys)
        int maxShow = min(10, aps->size());
        for (int i = 0; i < maxShow; i++) {
            AccessPoint ap = aps->get(i);
            
            // Selection marker
            if (ap.selected) {
                Serial.print(ANSI::FG_GREEN);
                Serial.print("[*] ");
            } else {
                Serial.print(ANSI::FG_GRAY);
                Serial.print("[ ] ");
            }
            
            // Key number
            Serial.print(ANSI::FG_YELLOW);
            Serial.print(i);
            Serial.print(": ");
            Serial.print(ANSI::RESET);
            
            // SSID with color based on selection
            if (ap.selected) {
                Serial.print(ANSI::FG_GREEN);
            }
            Serial.print(ap.essid.c_str());
            Serial.print(ANSI::FG_GRAY);
            Serial.print(" [Ch:");
            Serial.print(ap.channel);
            Serial.print("] ");
            Serial.print(ap.rssi);
            Serial.println("dBm");
            Serial.print(ANSI::RESET);
        }
        
        if (aps->size() > 10) {
            Serial.print(ANSI::FG_GRAY);
            Serial.print("... and ");
            Serial.print(aps->size() - 10);
            Serial.println(" more (not selectable)");
            Serial.print(ANSI::RESET);
        }
    }
    Serial.println();
}

void SerialTUI::renderTextInput() {
    Serial.print(ANSI::FG_YELLOW);
    Serial.print(ANSI::BOLD);
    Serial.println("=== ENTER SSID ===");
    Serial.print(ANSI::RESET);
    Serial.println();
    
    if (_inputPrompt) {
        Serial.println(_inputPrompt);
        Serial.println();
    }
    
    // Show input field
    Serial.print(ANSI::FG_CYAN);
    Serial.print("> ");
    Serial.print(ANSI::RESET);
    Serial.print(ANSI::FG_WHITE);
    Serial.print(_inputBuffer);
    Serial.print(ANSI::CURSOR_SHOW);  // Show cursor in input mode
    Serial.print("_");  // Visual cursor
    Serial.print(ANSI::RESET);
    Serial.println();
    Serial.println();
    
    Serial.print(ANSI::FG_GRAY);
    Serial.print("(");
    Serial.print(_inputPos);
    Serial.println("/32 chars)");
    Serial.print(ANSI::RESET);
    Serial.println();
}

void SerialTUI::renderFooter() {
    Serial.print(ANSI::FG_GRAY);
    Serial.println("----------------------------------------");
    
    switch (_inputMode) {
        case InputMode::SELECT_AP:
            Serial.println(" [0-9] Toggle AP    [A] Select All");
            Serial.println(" [N] Select None    [Enter/Q] Done");
            break;
        case InputMode::INPUT_TEXT:
            Serial.println(" Type SSID name (max 32 chars)");
            Serial.println(" [Enter] Confirm   [Esc] Cancel");
            break;
        default:
            Serial.println(" [W/S] or [Arrows] Navigate");
            Serial.println(" [Enter] Select    [Q/Esc] Back");
            break;
    }
    
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
        
        // Handle based on input mode
        if (_inputMode == InputMode::SELECT_AP) {
            handleAPSelectionInput(c);
            return;
        } else if (_inputMode == InputMode::INPUT_TEXT) {
            handleTextInput(c);
            return;
        }
        
        // Normal menu mode - escape sequence handling for arrow keys
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

void SerialTUI::handleAPSelectionInput(char c) {
    auto* aps = wifiAttacks.getAPs();
    
    // Handle number keys 0-9
    if (c >= '0' && c <= '9') {
        int idx = c - '0';
        if (idx < aps->size()) {
            AccessPoint ap = aps->get(idx);
            wifiAttacks.selectAP(idx, !ap.selected);  // Toggle
            _needsRedraw = true;
        }
        return;
    }
    
    switch (c) {
        case 'a':
        case 'A':
            // Select all
            for (int i = 0; i < aps->size(); i++) {
                wifiAttacks.selectAP(i, true);
            }
            _needsRedraw = true;
            break;
            
        case 'n':
        case 'N':
            // Deselect all
            for (int i = 0; i < aps->size(); i++) {
                wifiAttacks.selectAP(i, false);
            }
            _needsRedraw = true;
            break;
            
        case '\r':
        case '\n':
        case 'q':
        case 'Q':
        case 27:  // ESC
            // Done - exit selection mode
            exitInputMode();
            break;
    }
}

void SerialTUI::handleTextInput(char c) {
    // Handle escape for cancel
    if (c == 27) {
        memset(_inputBuffer, 0, sizeof(_inputBuffer));
        _inputPos = 0;
        exitInputMode();
        return;
    }
    
    // Handle enter for confirm
    if (c == '\r' || c == '\n') {
        if (_inputPos > 0) {
            // Signal that input is ready - set pending action
            _pendingAction = MenuAction::TARGETS_ADD_SSID;
        }
        exitInputMode();
        return;
    }
    
    // Handle backspace
    if (c == 8 || c == 127) {
        if (_inputPos > 0) {
            _inputPos--;
            _inputBuffer[_inputPos] = '\0';
            _needsRedraw = true;
        }
        return;
    }
    
    // Handle printable characters
    if (c >= 32 && c <= 126 && _inputPos < 32) {
        _inputBuffer[_inputPos++] = c;
        _inputBuffer[_inputPos] = '\0';
        _needsRedraw = true;
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
    if (_inputMode != InputMode::MENU) {
        exitInputMode();
        return;
    }
    
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
