#include "SerialTUI.h"

SerialTUI tui;

void SerialTUI::begin() {
    Serial.begin(SERIAL_BAUD);
    while (!Serial) delay(10);
    
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
    Serial.println("========================================");
    Serial.print(ANSI::RESET);
    Serial.println();
}

void SerialTUI::renderMenu() {
    // TODO: Implement menu rendering
}

void SerialTUI::renderFooter() {
    Serial.print(ANSI::FG_GRAY);
    Serial.println("----------------------------------------");
    Serial.println(" [W/S] Navigate  [Enter] Select  [Q] Back");
    Serial.print(ANSI::RESET);
}

void SerialTUI::handleInput() {
    // TODO: Implement input handling
}