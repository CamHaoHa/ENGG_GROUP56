# Integration Test Cases for Bridge Control System

## 1. Hardware Component Tests

### 1.1 Ultrasonic Sensors

-   [ ] **Test Entry Sensor (Sensor 1)** - Detect object at 35-50cm range
-   [ ] **Test Exit Sensor (Sensor 2)** - Detect object at 35-50cm range
-   [ ] **Small Boat Filtering** - Place object <35cm, verify it's IGNORED
-   [ ] **Large Boat Detection** - Place object ≥35cm, verify it's DETECTED
-   [ ] **Continuous Detection** - Hold object for 500ms, verify state transition
-   [ ] **Detection Loss** - Remove object before 500ms, verify timer reset
-   [ ] **Sensor Error Handling** - Block sensor, verify error message in serial

### 1.2 Limit Switches

-   [ ] **Top Limit Switch** - Manually press, verify `isBridgeFullyOpen()` returns true
-   [ ] **Bottom Limit Switch** - Manually press, verify `isBridgeFullyClosed()` returns true
-   [ ] **Motor Safety** - Verify motor stops when limit switches triggered

### 1.3 Motor Control

-   [ ] **Motor Open** - Send open command, verify motor direction HIGH
-   [ ] **Motor Close** - Send close command, verify motor direction LOW
-   [ ] **Motor Stop** - Verify motor stops when commanded
-   [ ] **Limit Switch Safety** - Verify motor won't open at top limit
-   [ ] **Limit Switch Safety** - Verify motor won't close at bottom limit

### 1.4 Servo Gates (Boom Gates)

-   [ ] **Raise Gates** - Verify servos move to 0° (RAISED)
-   [ ] **Lower Gates** - Verify servos move to 90° (LOWERED)
-   [ ] **Hold Position** - Verify servos detach after positioning

### 1.5 LED Traffic Lights

-   [ ] **Traffic Lights** - Cycle through RED, YELLOW, GREEN individually
-   [ ] **Boat Lights** - Cycle through RED, YELLOW, GREEN individually
-   [ ] **Status LED** - Verify LED on pin 26 responds correctly

---

## 2. WiFi & Network Tests

### 2.1 WiFi Access Point

-   [ ] **AP Creation** - Verify AP "group56bridge" is visible
-   [ ] **Connection** - Connect to AP with password "bridgehereweare"
-   [ ] **IP Address** - Verify ESP32 IP is 192.168.4.1
-   [ ] **Client Connection** - Connect multiple devices simultaneously

### 2.2 HTTP Server

-   [ ] **Server Availability** - Verify HTTP server responds on port 80
-   [ ] **Root Endpoint** - Test GET "/" redirects to React app
-   [ ] **CORS Headers** - Verify all responses include CORS headers

---

## 3. Authentication & API Tests

### 3.1 Login/Logout

-   [ ] **Successful Login** - POST `/api/login` with correct credentials
    ```json
    {
        "username": "admin",
        "password": "admin"
    }
    ```
-   [ ] **Failed Login** - POST `/api/login` with wrong credentials (expect 401)
-   [ ] **Token Generation** - Verify unique token returned
-   [ ] **Logout** - GET `/api/logout` invalidates token
-   [ ] **Duplicate Login** - Login twice, verify first token invalidated

### 3.2 State Endpoint

-   [ ] **Unauthorized Access** - GET `/api/state` without token (expect 401)
-   [ ] **Authorized Access** - GET `/api/state` with valid token (expect 200)
-   [ ] **State Response** - Verify JSON contains all required fields:
    -   `currentState`, `bridgeState`, `manualOverride`
    -   `boatDetected`, `boatSensor1`, `boatSensor2`
    -   `redLedA`, `yellowLedA`, `greenLedA`
    -   `redLedB`, `yellowLedB`, `greenLedB`
    -   `limitSwitchTop`, `limitSwitchBottom`

### 3.3 Command Endpoint

-   [ ] **Unauthorized Command** - POST `/api/command` without token (expect 401)
-   [ ] **Invalid JSON** - POST malformed JSON (expect 400)
-   [ ] **Unknown Action** - POST unknown action (expect 400)
    ```json
    {
        "action": "invalidAction"
    }
    ```

---

## 4. Automatic Mode State Machine Tests

### 4.1 Full Cycle (STATE0 → STATE8)

-   [ ] **STATE0 (IDLE)** - Verify traffic GREEN, boat RED, gates RAISED
-   [ ] **STATE0 → STATE1** - Detect large boat at entry for 500ms
-   [ ] **STATE1 (BOAT DETECTED)** - Verify traffic YELLOW, wait 3s
-   [ ] **STATE1 → STATE2** - Verify transition after 3s
-   [ ] **STATE2 (CLEARING TRAFFIC)** - Verify traffic RED, gates still RAISED, wait 5s
-   [ ] **STATE2 → STATE2B** - Verify transition after 5s
-   [ ] **STATE2B (TRAFFIC CLEAR)** - Verify gates LOWERED, wait 3s
-   [ ] **STATE2B → STATE3** - Verify transition after 3s
-   [ ] **STATE3 (OPENING)** - Verify motor opening, monitor limit switch
-   [ ] **STATE3 → STATE4** - Verify transition on limit switch OR 10s timeout
-   [ ] **STATE4 (OPEN YELLOW)** - Verify boat YELLOW, wait 3s
-   [ ] **STATE4 → STATE5** - Verify boat GREEN after 3s
-   [ ] **STATE5 (WAITING)** - Verify boat GREEN, monitor exit sensor
-   [ ] **STATE5 → STATE6** - Detect boat at exit for 500ms
-   [ ] **STATE6 (STOPPING BOATS)** - Verify boat YELLOW, wait 3s
-   [ ] **STATE6 → STATE7** - Verify transition after 3s
-   [ ] **STATE7 (CLOSING)** - Verify motor closing, monitor limit switch
-   [ ] **STATE7 → STATE8** - Verify transition on limit switch OR 10s timeout
-   [ ] **STATE8 (CLOSED YELLOW)** - Verify traffic YELLOW, gates RAISED, wait 2s
-   [ ] **STATE8 → STATE0** - Return to IDLE, traffic GREEN

### 4.2 Edge Cases

-   [ ] **STATE5 Emergency Timeout** - Wait 30s in STATE5, verify auto-close
-   [ ] **STATE5 Reverse Detection** - Detect boat at entry while in STATE5
-   [ ] **Small Boat Rejection** - Place object <35cm, verify NO state transition
-   [ ] **Sensor Intermittent** - Intermittent detection <500ms, verify no transition

---

## 5. Manual Override Mode Tests

### 5.1 Enable/Disable Override

-   [ ] **Enable Override** - POST `{"action":"enableOverride"}`, verify mode active
-   [ ] **Disable Override** - POST `{"action":"disableOverride"}`, return to auto mode
-   [ ] **Override from STATE0** - Enable override in IDLE state
-   [ ] **Override from STATE5** - Enable override while bridge open

### 5.2 Manual Light Control

-   [ ] **Traffic Red** - POST `{"action":"trafficRed"}` in override mode
-   [ ] **Traffic Yellow** - POST `{"action":"trafficYellow"}` in override mode
-   [ ] **Traffic Green** - POST `{"action":"trafficGreen"}` in override mode
-   [ ] **Boat Red** - POST `{"action":"boatRed"}` in override mode
-   [ ] **Boat Yellow** - POST `{"action":"boatYellow"}` in override mode
-   [ ] **Boat Green** - POST `{"action":"boatGreen"}` in override mode
-   [ ] **Light Command Without Override** - Verify rejected (expect 400)

### 5.3 Manual Bridge Control

-   [ ] **Manual Open** - POST `{"action":"open"}` in override mode
-   [ ] **Manual Close** - POST `{"action":"close"}` in override mode
-   [ ] **Open Timeout** - Verify motor stops after 10s if limit not reached
-   [ ] **Close Timeout** - Verify motor stops after 10s if limit not reached
-   [ ] **Limit Switch Override** - Verify motor stops at limits

### 5.4 Clear Command

-   [ ] **Clear from Override** - POST `{"action":"clear"}`, verify reset to STATE0
-   [ ] **Clear During Operation** - Clear while motor running, verify safe stop

---

## 6. Trigger Commands (Auto Mode)

### 6.1 Manual Trigger Open

-   [ ] **Open from STATE0** - POST `{"action":"open"}` when idle
-   [ ] **Open Rejection** - Try opening in non-STATE0, verify ignored

### 6.2 Manual Trigger Close

-   [ ] **Close from STATE5** - POST `{"action":"close"}` when bridge open
-   [ ] **Close Rejection** - Try closing when not in STATE5, verify ignored

---

## 7. Safety & Error Handling

### 7.1 Motor Safety

-   [ ] **No Open at Top** - Attempt open when top limit pressed
-   [ ] **No Close at Bottom** - Attempt close when bottom limit pressed
-   [ ] **Stop on Limit** - Verify automatic stop on limit switch trigger

### 7.2 Timeout Handling

-   [ ] **STATE3 Timeout** - No top limit for 10s, verify warning + continue
-   [ ] **STATE7 Timeout** - No bottom limit for 10s, verify warning + continue
-   [ ] **STATE5 Emergency** - 30s timeout in STATE5, verify auto-close

### 7.3 Serial Monitoring

-   [ ] **Boot Messages** - Verify detailed startup info in serial monitor
-   [ ] **State Transitions** - Verify logged state changes
-   [ ] **Sensor Readings** - Verify ultrasonic distances logged
-   [ ] **Error Messages** - Verify warnings for sensor errors/timeouts

---

## 8. Stress & Reliability Tests

### 8.1 Continuous Operation

-   [ ] **10 Full Cycles** - Run 10 complete open/close cycles, verify stability
-   [ ] **Rapid Detection** - Rapidly move object in/out of sensor range
-   [ ] **Long Idle** - Leave in STATE0 for extended period

### 8.2 Network Stress

-   [ ] **Rapid API Calls** - Send 100 rapid `/api/state` requests
-   [ ] **Concurrent Clients** - Multiple devices sending commands simultaneously
-   [ ] **Token Invalidation** - Login from 2nd device, verify 1st token invalid

### 8.3 Power & Reset

-   [ ] **Cold Boot** - Power cycle ESP32, verify clean startup
-   [ ] **Mid-Operation Reset** - Reset during STATE3, verify safe recovery
-   [ ] **WiFi Reconnection** - Disconnect/reconnect WiFi client

---

## 9. Integration with React Dashboard

### 9.1 Real-time Updates

-   [ ] **State Polling** - Verify dashboard updates every interval
-   [ ] **Sensor Status** - Verify boat detection shows in UI
-   [ ] **LED Status** - Verify LED states match physical LEDs

### 9.2 User Actions

-   [ ] **Login Flow** - Login via dashboard, verify token stored
-   [ ] **Override Toggle** - Enable/disable override via UI
-   [ ] **Manual Controls** - Test all buttons in override mode
-   [ ] **Auto Commands** - Test open/close buttons in auto mode

---

## 10. Documentation Verification

-   [ ] **Serial Output Matches Code** - Verify all `Serial.println()` statements accurate
-   [ ] **Pin Assignments** - Verify physical wiring matches code constants
-   [ ] **Timing Values** - Verify delay constants match requirements
-   [ ] **State Diagram** - Verify state machine matches documentation

---

## Test Execution Checklist

### Equipment Needed:

-   ESP32 with bridge_control.ino uploaded
-   Ultrasonic sensors HC-SR04 (x2)
-   Limit switches (x2)
-   DC motor with driver
-   Servo motors (x2)
-   LED traffic lights (6 LEDs)
-   Power supply
-   Computer with Serial Monitor
-   WiFi-capable device for API testing

### Testing Order:

1. Hardware component tests (1.1-1.5)
2. WiFi & network setup (2.1-2.2)
3. Authentication (3.1-3.3)
4. Automatic state machine (4.1-4.2)
5. Manual override (5.1-5.4)
6. Safety tests (7.1-7.3)
7. Integration tests (9.1-9.2)

---

## Test Results Template

| Test ID | Test Name    | Status            | Notes | Date | Tester |
| ------- | ------------ | ----------------- | ----- | ---- | ------ |
| 1.1.1   | Entry Sensor | ⬜ Pass / ❌ Fail |       |      |        |
| 1.1.2   | Exit Sensor  | ⬜ Pass / ❌ Fail |       |      |        |
| ...     | ...          | ...               |       |      |        |

---

## Critical Issues Log

| Issue # | Description | Severity        | Status        | Resolution |
| ------- | ----------- | --------------- | ------------- | ---------- |
|         |             | High/Medium/Low | Open/Resolved |            |

---

## API Testing Examples

### cURL Commands for Testing:

**Login:**

```bash
curl -X POST http://192.168.4.1/api/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"admin"}'
```

**Get State:**

```bash
curl -X GET http://192.168.4.1/api/state \
  -H "x-auth-token: YOUR_TOKEN_HERE"
```

<!-- ...existing code... -->

### 1.1 Ultrasonic Sensors

#### Test 1.1.1: Entry Sensor (Sensor 1) - Detect object at 35-50cm range

**Procedure:**
1. Power on ESP32 and open Serial Monitor (115200 baud)
2. Place object at various distances from Entry Sensor (Pins D15/D2)
3. Observe serial output and state changes

**Expected Output:**

*When object at 40cm (within 35-50cm range):*
```
🚤 LARGE BOAT DETECTED at Entry (Sensor 1)
   Distance: 40cm (threshold: ≥35cm)
   Detection time: 523ms
```

*When object at 30cm (below threshold):*
```
(No detection message - small boat ignored)
```

*When object at 60cm (outside range):*
```
(No detection message - too far)
```

**Pass Criteria:**
- [ ] Objects at 35-50cm consistently detected
- [ ] Detection message shows correct distance
- [ ] Objects <35cm ignored (small boat filtering)
- [ ] Objects >50cm ignored
- [ ] `boatDetectedSensor1` becomes `true` in `/api/state` response

---

#### Test 1.1.2: Exit Sensor (Sensor 2) - Detect object at 35-50cm range

**Procedure:**
1. Keep Serial Monitor open
2. Place object at various distances from Exit Sensor (Pins D4/D16)
3. Observe serial output

**Expected Output:**

*When object at 42cm (within 35-50cm range):*
```
🚤 LARGE BOAT DETECTED at Exit (Sensor 2)
   Distance: 42cm (threshold: ≥35cm)
   Detection time: 518ms
```

*When in STATE5 and boat exits:*
```
🚤 LARGE BOAT DETECTED at Exit (Sensor 2)
   Distance: 38cm (threshold: ≥35cm)
   Detection time: 502ms
🚦 Transitioning from STATE5 to STATE6 (STOPPING BOATS)
```

**Pass Criteria:**
- [ ] Objects at 35-50cm consistently detected
- [ ] Exit detection triggers STATE5 → STATE6 transition
- [ ] `boatDetectedSensor2` becomes `true` in `/api/state` response
- [ ] Detection message shows correct distance

---

#### Test 1.1.3: Small Boat Filtering

**Procedure:**
1. Place small object (e.g., hand) very close to sensor (<35cm)
2. Hold for >500ms
3. Verify NO state transition occurs

**Expected Output:**

*When object at 25cm:*
```
(No "LARGE BOAT DETECTED" message)
(System remains in STATE0 - IDLE)
```

**Pass Criteria:**
- [ ] No detection message printed
- [ ] `boatCurrentlyDetected` remains `false`
- [ ] System stays in STATE0 (IDLE)
- [ ] No state transition after 500ms

---

#### Test 1.1.4: Large Boat Detection

**Procedure:**
1. Place object at 38cm from Entry Sensor
2. Hold steady for >500ms
3. Observe state transition

**Expected Output:**

*At detection start:*
```
🚤 LARGE BOAT DETECTED at Entry (Sensor 1)
   Distance: 38cm (threshold: ≥35cm)
   Detection time: 0ms
```

*After 500ms continuous detection:*
```
🚤 LARGE BOAT DETECTED at Entry (Sensor 1)
   Distance: 38cm (threshold: ≥35cm)
   Detection time: 523ms
🚦 Transitioning from STATE0 to STATE1 (BOAT DETECTED)
```

**Pass Criteria:**
- [ ] Detection message appears immediately
- [ ] State transition occurs after 500ms
- [ ] Detection time increments in serial output
- [ ] Traffic light changes to YELLOW

---

#### Test 1.1.5: Continuous Detection

**Procedure:**
1. Place object at 40cm from Entry Sensor
2. Hold for exactly 500ms
3. Verify state transition timing

**Expected Output:**

*Initial detection:*
```
🚤 LARGE BOAT DETECTED at Entry (Sensor 1)
   Distance: 40cm (threshold: ≥35cm)
   Detection time: 0ms
```

*After 200ms:*
```
🚤 LARGE BOAT DETECTED at Entry (Sensor 1)
   Distance: 40cm (threshold: ≥35cm)
   Detection time: 203ms
```

*After 500ms:*
```
🚤 LARGE BOAT DETECTED at Entry (Sensor 1)
   Distance: 40cm (threshold: ≥35cm)
   Detection time: 501ms
🚦 Transitioning from STATE0 to STATE1 (BOAT DETECTED)
🚦 New State: STATE1 (BOAT DETECTED)
    Traffic A: YELLOW | Boat B: RED | Gates: RAISED
```

**Pass Criteria:**
- [ ] Detection time increments consistently
- [ ] State transition occurs at ~500ms mark
- [ ] State change logged with full details

---

#### Test 1.1.6: Detection Loss

**Procedure:**
1. Place object at 40cm from Entry Sensor
2. Hold for 300ms
3. Remove object
4. Verify timer resets and no state transition

**Expected Output:**

*During detection (0-300ms):*
```
🚤 LARGE BOAT DETECTED at Entry (Sensor 1)
   Distance: 40cm (threshold: ≥35cm)
   Detection time: 103ms
🚤 LARGE BOAT DETECTED at Entry (Sensor 1)
   Distance: 40cm (threshold: ≥35cm)
   Detection time: 305ms
```

*After removal:*
```
(No more detection messages)
(System remains in STATE0)
```

**Pass Criteria:**
- [ ] Detection messages stop when object removed
- [ ] No state transition occurs
- [ ] System remains in STATE0
- [ ] Next detection starts timer from 0ms again

---

#### Test 1.1.7: Sensor Error Handling

**Procedure:**
1. Block sensor with hand completely (cause timeout)
2. Or disconnect sensor wire temporarily
3. Observe error messages

**Expected Output:**

*When sensor times out:*
```
⚠️ Ultrasonic sensor timeout on Sensor 1 (Entry)
```

*When sensor returns invalid reading:*
```
⚠️ Invalid distance reading: 0cm (Sensor 1)
```

**Pass Criteria:**
- [ ] Error messages appear in serial output
- [ ] System continues operating (doesn't crash)
- [ ] Invalid readings ignored (don't trigger state change)
- [ ] System recovers when sensor unblocked

---

### Serial Monitor Output Format Reference

**Normal Operation Output:**
```
========================================
🔄 STATE CHECK: Currently in STATE0 (IDLE)
🔍 Ultrasonic Check (every 200ms)
   Sensor 1 (Entry): 180cm
   Sensor 2 (Exit): 195cm
========================================

========================================
🔄 STATE CHECK: Currently in STATE0 (IDLE)
🔍 Ultrasonic Check (every 200ms)
   Sensor 1 (Entry): 38cm
   Sensor 2 (Exit): 190cm
🚤 LARGE BOAT DETECTED at Entry (Sensor 1)
   Distance: 38cm (threshold: ≥35cm)
   Detection time: 0ms
========================================

(... continues every 200ms ...)

========================================
🔄 STATE CHECK: Currently in STATE0 (IDLE)
🔍 Ultrasonic Check (every 200ms)
   Sensor 1 (Entry): 38cm
   Sensor 2 (Exit): 189cm
🚤 LARGE BOAT DETECTED at Entry (Sensor 1)
   Distance: 38cm (threshold: ≥35cm)
   Detection time: 512ms
🚦 Transitioning from STATE0 to STATE1 (BOAT DETECTED)
🚦 New State: STATE1 (BOAT DETECTED)
    Traffic A: YELLOW | Boat B: RED | Gates: RAISED
========================================
```

**API State Response During Detection:**
```json
{
  "currentState": 1,
  "bridgeState": false,
  "manualOverride": false,
  "boatDetected": true,
  "boatSensor1": true,
  "boatSensor2": false,
  "redLedA": 0,
  "yellowLedA": 1,
  "greenLedA": 0,
  "redLedB": 1,
  "yellowLedB": 0,
  "greenLedB": 0,
  "limitSwitchTop": false,
  "limitSwitchBottom": true
}
```

<!-- ...existing code... -->