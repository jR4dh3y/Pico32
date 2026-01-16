# Targeted Station Deauth Attack - Implementation Plan

## Feature Request
Scan for nearby client devices (phones, laptops) and specifically target them with deauth attacks, rather than broadcasting deauth to all clients of an AP.

---

## 1. Does This Already Exist?

### Existing Functionality:
| Component | Status | Location |
|-----------|--------|----------|
| Station scanning | ✅ EXISTS | `WiFiAttacks::startScanStation()` in `WiFiAttacks.cpp:519` |
| Station storage | ✅ EXISTS | `LinkedList<Station> _stations` in `WiFiAttacks.h:128` |
| Station struct with `selected` field | ✅ EXISTS | `WiFiAttacks.h:41-46` |
| Deauth frame sending | ✅ EXISTS | `sendDeauthFrame(ap, client, channel)` in `WiFiAttacks.cpp:658` |
| AP selection UI | ✅ EXISTS | `enterAPSelectionMode()`, `handleAPSelectionInput()` in `SerialTUI.cpp` |
| `selectStation()` function | ❌ MISSING | Need to add (parallel to `selectAP()`) |
| Station selection UI | ❌ MISSING | Need to add (parallel to AP selection UI) |
| Targeted station deauth attack | ❌ MISSING | Need to add new attack mode |
| Menu entry for station deauth | ❌ MISSING | Need to add to `MenuDefs.h` |

---

## 2. Can I Extend Something Existing?

### YES - Several components can be extended:

| Existing Code | Extension Needed | Effort |
|---------------|------------------|--------|
| `selectAP(index, selected)` | Add parallel `selectStation(index, selected)` | Low - copy pattern |
| `enterAPSelectionMode()` | Add parallel `enterStationSelectionMode()` | Low - copy pattern |
| `handleAPSelectionInput(char c)` | Add parallel `handleStationSelectionInput(char c)` | Low - copy pattern |
| `renderAPSelection()` | Add parallel `renderStationSelection()` | Low - copy pattern |
| `sendDeauthToAll()` | Add parallel `sendDeauthToStations()` | Low - already has `sendDeauthFrame()` |
| `InputMode` enum | Add `SELECT_STATION` | Trivial |
| `MenuAction` enum | Add `TARGETS_SELECT_STA`, `WIFI_ATTACK_DEAUTH_STA` | Trivial |

---

## 3. Where Should This Live?

| New Code | Location | Reasoning |
|----------|----------|-----------|
| `selectStation()` | `WiFiAttacks.cpp/h` | Parallel to `selectAP()`, same class |
| `sendDeauthToStations()` | `WiFiAttacks.cpp` (private) | Attack logic lives in WiFiAttacks |
| `startDeauthStation()` | `WiFiAttacks.cpp/h` (public) | New attack entry point |
| Station selection UI | `SerialTUI.cpp/h` | All UI lives in SerialTUI |
| Menu entries | `MenuDefs.h` | All menu definitions there |
| Action handler | `main.cpp` | Existing pattern in `handleAction()` |

**Verdict**: Keep additions in existing files following established patterns. No new files needed.

---

## 4. Am I Duplicating Anything?

### Potential Duplication Risk:
The station selection UI will be ~90% similar to AP selection UI. 

### Mitigation Options:
1. **Option A**: Accept duplication (small amount, clear purpose)
2. **Option B**: Refactor to generic selection UI with callbacks (over-engineering for this use case)
3. **Option C**: Template/parameterize the selection functions

**Recommendation**: **Option A** - Accept minor duplication. The code is simple (~50 lines each), and the differences (Station vs AccessPoint struct, different display format) make abstraction awkward. Clear, readable code > DRY for small amounts.

### Shared Code (NO duplication here):
- `sendDeauthFrame()` - already exists and is reusable
- Frame construction logic - stays in one place
- Promiscuous mode handling - stays in one place

---

## 5. Is Each Function Doing Too Much?

### Review of New Functions:

| Function | Single Responsibility | Pass? |
|----------|----------------------|-------|
| `selectStation(index, selected)` | Toggle a station's selected state | ✅ |
| `startDeauthStation()` | Initialize station-targeted deauth attack mode | ✅ |
| `sendDeauthToStations()` | Send deauth frame to each selected station | ✅ |
| `enterStationSelectionMode()` | Switch TUI to station selection input mode | ✅ |
| `handleStationSelectionInput()` | Process keypresses in station selection mode | ✅ |
| `renderStationSelection()` | Display station list with selection UI | ✅ |

All functions can be described in one sentence without "and".

---

## Implementation Checklist

### Phase 1: WiFiAttacks Backend (WiFiAttacks.h, WiFiAttacks.cpp)

- [ ] **1.1** Add `ATTACK_DEAUTH_STATION` to `WiFiMode` enum
- [ ] **1.2** Add `selectStation(uint8_t index, bool selected)` method (public)
- [ ] **1.3** Add `startDeauthStation()` method (public)
- [ ] **1.4** Add `sendDeauthToStations()` method (private)
- [ ] **1.5** Update `update()` to handle `ATTACK_DEAUTH_STATION` case

### Phase 2: Menu Definitions (MenuDefs.h)

- [ ] **2.1** Add `TARGETS_SELECT_STA` to `MenuAction` enum
- [ ] **2.2** Add `WIFI_ATTACK_DEAUTH_STA` to `MenuAction` enum
- [ ] **2.3** Add "Select Stations" item to `targetsMenu[]`
- [ ] **2.4** Add "Deauth Stations" item to `wifiAttackMenu[]`
- [ ] **2.5** Update array sizes

### Phase 3: SerialTUI UI (SerialTUI.h, SerialTUI.cpp)

- [ ] **3.1** Add `SELECT_STATION` to `InputMode` enum
- [ ] **3.2** Add `enterStationSelectionMode()` method (public)
- [ ] **3.3** Add `renderStationSelection()` method (private)
- [ ] **3.4** Add `handleStationSelectionInput()` method (private)
- [ ] **3.5** Update `render()` switch to handle `SELECT_STATION`
- [ ] **3.6** Update `handleInput()` to route to station handler
- [ ] **3.7** Update `renderFooter()` for station selection controls

### Phase 4: Main Integration (main.cpp)

- [ ] **4.1** Add `TARGETS_SELECT_STA` case in `handleAction()`
- [ ] **4.2** Add `WIFI_ATTACK_DEAUTH_STA` case in `handleAction()`

### Phase 5: Testing

- [ ] **5.1** Compile and flash
- [ ] **5.2** Test: Scan Stations finds devices
- [ ] **5.3** Test: Targets > Select Stations shows list
- [ ] **5.4** Test: Can toggle station selection with number keys
- [ ] **5.5** Test: Deauth Stations attack runs without errors
- [ ] **5.6** Test: Attack stops with any key press

---

## Deauth Frame Details for Station Targeting

When targeting a specific station (client), the deauth frame needs:

```
┌─────────────────────────────────────────────────────────────┐
│ Frame Control: 0xC0 0x00 (Deauthentication)                │
│ Duration: 0x00 0x00                                         │
│ Destination: Station MAC (the client we're targeting)      │
│ Source: AP BSSID (spoof as the AP the station is on)       │
│ BSSID: AP BSSID                                             │
│ Sequence: 0x00 0x00                                         │
│ Reason Code: 0x07 0x00                                      │
└─────────────────────────────────────────────────────────────┘
```

Key difference from broadcast deauth:
- **Destination**: Specific station MAC instead of `FF:FF:FF:FF:FF:FF`
- **Source/BSSID**: Use the station's associated AP (stored in `Station.bssid`)

The existing `sendDeauthFrame(ap, client, channel)` already supports this - we just swap the arguments:
- Current usage: `sendDeauthFrame(ap.bssid, broadcast, channel)`
- New usage: `sendDeauthFrame(station.bssid, station.mac, channel)`

We also need to determine the channel. Options:
1. Use the station's associated AP's channel (if we scanned APs too)
2. Channel hop while sending (current approach for station scan)
3. Use current channel setting

**Recommendation**: Channel hop during attack, or use stored AP channel if available.

---

## Estimated LOC Changes

| File | Lines Added | Lines Modified |
|------|-------------|----------------|
| WiFiAttacks.h | ~8 | 0 |
| WiFiAttacks.cpp | ~50 | ~5 |
| MenuDefs.h | ~6 | ~4 |
| SerialTUI.h | ~3 | 0 |
| SerialTUI.cpp | ~80 | ~10 |
| main.cpp | ~25 | 0 |
| **Total** | **~172** | **~19** |

---

## Risk Assessment

| Risk | Mitigation |
|------|------------|
| Channel mismatch (station on different channel) | Channel hop during attack or send on all channels |
| Station not re-scanned after AP changes | Document that station scan should be redone after AP scan |
| Memory overflow with too many stations | Already limited to `MAX_STATIONS` in Config.h |
| UI confusing (AP vs Station selection) | Clear labels and different menu paths |

---

## Code Patterns to Follow

### Pattern: Selection Function (from `selectAP`)
```cpp
void WiFiAttacks::selectStation(uint8_t index, bool selected) {
    if (index < _stations.size()) {
        Station s = _stations.get(index);
        s.selected = selected;
        _stations.set(index, s);
    }
}
```

### Pattern: Attack Initialization (from `startDeauth`)
```cpp
void WiFiAttacks::startDeauthStation() {
    _mode = WiFiMode::ATTACK_DEAUTH_STATION;
    _packetCount = 0;
    _lastUpdate = millis();
    esp_wifi_set_promiscuous(true);  // Required for raw TX
    tui.printStatus("Station deauth started (press any key to stop)...");
}
```

### Pattern: Frame Sending Loop (from `sendDeauthToAll`)
```cpp
void WiFiAttacks::sendDeauthToStations() {
    for (int i = 0; i < _stations.size(); i++) {
        Station s = _stations.get(i);
        if (s.selected) {
            // Deauth station from its associated AP
            // TODO: Determine channel (hop or use stored)
            sendDeauthFrame(s.bssid, s.mac, _channel);
            _packetCount++;
        }
    }
}
```
