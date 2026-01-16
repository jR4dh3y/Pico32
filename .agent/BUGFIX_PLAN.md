---
title: ESP32 Marauder TUI - Bug Fix & UX Improvement Plan
created: 2026-01-16
status: completed
---

# ESP32 Marauder TUI - Bug Fix & UX Improvement Plan

## Overview

This document outlines the issues identified in the ESP32 Marauder TUI project and the plan to fix them systematically.

---

## Issues Identified

### 1. 🔴 CRITICAL: Bluetooth MAC Address Error
**Error:** `E (408434) system_api: Base MAC must be a unicast MAC`

**Root Cause Analysis:**
- In `BTAttacks.cpp` line 449-458, `randomizeMac()` generates a random MAC:
  ```cpp
  mac[0] |= 0x02;  // Local bit
  esp_base_mac_addr_set(mac);
  ```
- Problem: `esp_base_mac_addr_set()` is being called **after** NimBLE is initialized
- This function sets the **base ESP32 MAC**, not the BLE advertising address
- The MAC must be unicast (bit 0 of first byte must be 0), but random bytes can violate this
- Also, you cannot change base MAC once BLE is running

**What Already Exists:**
- NimBLE has built-in methods to set advertising address: `NimBLEDevice::setOwnAddrType()` and advertising-level address randomization

**Fix Approach:**
- Remove `esp_base_mac_addr_set()` call entirely
- Use NimBLE's native advertising address handling with `BLE_OWN_ADDR_RANDOM`
- Set random address properly via `NimBLEAdvertising::setAdvertisementData()` with random MAC

---

### 2. 🔴 CRITICAL: Cannot Properly Select APs/SSIDs
**Problem:** User cannot interactively select which APs to target for deauth

**Root Cause Analysis:**
- In `main.cpp` lines 217-236, `TARGETS_SELECT` action only toggles the **first non-selected** AP automatically
- There's no interactive selection mechanism - user has no control
- No way to select by index or toggle specific items
- The UX flow is: Scan APs → ??? → Can't select → Attack fails

**What Already Exists:**
- `selectAP(index, selected)` function in WiFiAttacks.cpp line 383-388
- List APs action shows indexed list with `[0] SSID...`
- Menu infrastructure supports keyboard input

**Fix Approach:**
- Need an **interactive AP selection screen** that:
  1. Shows numbered list of APs
  2. Lets user press 0-9 to toggle selection
  3. Shows which APs are selected with `*` marker
  4. Press Enter/Q to confirm and go back

---

### 3. 🔴 CRITICAL: Cannot Add Custom SSIDs
**Problem:** `TARGETS_ADD_SSID` just adds hardcoded "TestSSID"

**Root Cause Analysis:**
- In `main.cpp` lines 238-242:
  ```cpp
  case MenuAction::TARGETS_ADD_SSID:
      wifiAttacks.addSSID("TestSSID");  // Hardcoded!
  ```
- No serial input reading implemented
- User cannot type custom SSID names

**What Already Exists:**
- `addSSID(const char* ssid)` function works correctly
- Serial reading infrastructure exists in SerialTUI

**Fix Approach:**
- Add input mode to SerialTUI for text entry
- When user selects "Add SSID", enter input mode
- Read characters until Enter, then call addSSID() with the entered text

---

### 4. 🟠 HIGH: Text Overflow During Attacks
**Problem:** Output goes infinitely without any throttling or screen management

**Root Cause Analysis:**
- During attacks, `printResult()` and `printStatus()` are called continuously
- No output throttling mechanism
- No screen scrolling management
- BT attacks update every 2 seconds in BTAttacks.cpp line 138, but scans print every device found
- WiFi sniffing functions don't have proper output limiting

**What Already Exists:**
- `_lastUpdate` timestamp exists in both attack modules
- `printStatus()` prints to serial without any rate limiting

**Fix Approach:**
- Add output throttling - limit printResult() calls per second
- Add a "results count" display that updates instead of printing every line
- Consider a "last N results" buffer
- For scans: show summary + device count, not every single device

---

### 5. 🟠 HIGH: Deauth Attack Doesn't Work Without Selected APs
**Problem:** User can't select APs properly, so deauth does nothing

**Root Cause Analysis:**
- In `WiFiAttacks::sendDeauthToAll()` line 278-288:
  ```cpp
  if (ap.selected) {  // Nothing selected = nothing sent
      sendDeauthFrame(...)
  }
  ```
- The check at main.cpp line 72-75 shows error if no APs, but doesn't check if any are **selected**

**What Already Exists:**
- Selection logic works if we could actually select APs

**Fix Approach:**
- Part of Issue #2 - once AP selection works, this will work
- Also: add check for "no APs **selected**" with appropriate message

---

### 6. 🟡 MEDIUM: Sniffing Functions Are Stubs
**Problem:** WiFi sniffing modes don't actually capture packets

**Root Cause Analysis:**
- Looking at WiFiAttacks.cpp lines 179-213, all sniff functions just:
  ```cpp
  _mode = WiFiMode::SNIFF_XXX;
  _packetCount = 0;
  tui.printStatus("...");
  // No actual promiscuous mode setup!
  ```
- `esp_wifi_set_promiscuous(true)` is never called
- Promiscuous callback is never registered

**What Already Exists:**
- `_callbackInstance` pointer exists but unused
- `esp_wifi_set_promiscuous()` imported in headers

**Fix Approach:**
- Implement actual promiscuous mode setup
- Register packet callback function
- Filter packets based on frame type for each sniff mode
- Note: This is a larger feature - may defer to separate task

---

### 7. 🟡 MEDIUM: Beacon Attacks Use Wrong Channel
**Problem:** All beacon attacks broadcast on `_channel` but targets may be on different channels

**Root Cause Analysis:**
- `sendListBeacon()` always uses `_channel` variable
- For deauth to work, we need to match the target AP's channel
- Beacon spam doesn't necessarily need this

**What Already Exists:**
- Each AP has its channel stored in `AccessPoint.channel`

**Fix Approach:**
- For deauth: iterate and switch to each AP's channel before sending
- Already partially implemented in `sendDeauthFrame()` line 301

---

## Implementation Tasks

### Phase 1: Critical Fixes (Do First)
- [x] **Task 1.1:** Fix Bluetooth MAC error
  - Remove `esp_base_mac_addr_set()` 
  - Use NimBLE's random address type
  - Test BT spam attacks work without errors

- [x] **Task 1.2:** Implement interactive AP selection
  - Add `InputMode` enum to SerialTUI (SELECT_AP, INPUT_TEXT, etc.)
  - Add `renderAPSelection()` method
  - Handle number key presses for toggling
  - Add visual selected state

- [x] **Task 1.3:** Implement SSID text input
  - Add `_inputBuffer` to SerialTUI
  - Add `renderInputPrompt()` method  
  - Handle backspace, enter, escape
  - Call addSSID on completion

### Phase 2: UX Improvements
- [x] **Task 2.1:** Add output throttling
  - Add rate limiter to printResult()
  - Add result counter instead of spam
  - Consider circular buffer for last N results

- [x] **Task 2.2:** Improve attack feedback
  - Add selected AP count check before deauth
  - Show which APs are being attacked
  - Show packet count periodically (already exists, needs verification)

### Phase 3: Feature Completion (Optional/Later)
- [x] **Task 3.1:** Implement promiscuous mode sniffing
  - Added static promiscuous callback with frame type detection
  - Implemented frame parsers for: Beacon, Probe Request, Deauth, EAPOL, Pwnagotchi
  - Added channel hopping (500ms per channel) for comprehensive coverage
  - Station scan now discovers clients by capturing data frames
  - Raw sniff mode shows frame type distribution

---

## File Changes Summary

| File | Changes Needed |
|------|----------------|
| `BTAttacks.cpp` | Remove `esp_base_mac_addr_set()`, use NimBLE random addressing |
| `SerialTUI.h` | Add InputMode enum, input buffer, new methods |
| `SerialTUI.cpp` | Implement AP selection screen, text input mode |
| `main.cpp` | Update TARGETS_SELECT and ADD_SSID handlers |
| `MenuDefs.h` | May need new MenuActions for input modes |

---

## Verification Steps

1. Build compiles without errors
2. No BT MAC errors in serial output
3. Can scan APs and see numbered list
4. Can toggle AP selection with number keys
5. Can type custom SSID names
6. Deauth attack runs on selected APs
7. Output doesn't scroll infinitely during scans

---

## Notes

- All changes should be minimal and focused
- Reuse existing patterns (LinkedList, ANSI codes, etc.)
- Don't add new dependencies
- Keep memory footprint low (no PSRAM available)
