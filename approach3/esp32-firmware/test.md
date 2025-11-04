# Software Testing Documentation

## TPM.SW01 - Ultrasonic Boat Detection

**Name of TPM:** Ultrasonic Sensor Boat Detection System

**Purpose of TPM:** Ensure that the ultrasonic sensors reliably detect approaching boats and trigger the bridge opening sequence.

**Source Requirement:** REQ.01 (System shall detect approaching boats)

**Risk Level:** High

**What should be measured:**

-   Distance readings from both ultrasonic sensors (D15/D2 and D4/D16)
-   Detection threshold accuracy (50cm)
-   False positive rate
-   Response time from detection to STATE1 transition
-   Continuous detection requirement (500ms minimum)

**How should it be measured:**

-   Place objects at known distances (30cm, 50cm, 70cm) and verify detection
-   Monitor Serial output for distance readings
-   Use `detectBoat()` function return values
-   Measure time between first detection and state transition
-   Test with various object sizes and materials

**How often should it be measured:**

-   During initial setup and calibration
-   After any hardware modifications
-   Weekly during development phase
-   Before each demonstration

**Measures of success:**

-   Sensors detect objects within 50cm range with 95% accuracy
-   No detection beyond 50cm threshold
-   State transition occurs within 600ms of continuous detection
-   Both sensors function independently
-   Serial output shows correct distance readings

**Measures of failure:**

-   False positives (detection without object present)
-   Missed detections (object present but not detected)
-   Incorrect distance readings (>10% error)
-   Single sensor failure causing system failure
-   Detection delay >1000ms

**Possible causes of failure:**

-   Incorrect wiring or pin configuration
-   Ultrasonic sensor malfunction
-   Environmental interference (noise, temperature)
-   Software timing issues with `pulseIn()` function
-   Power supply instability

---

## TPM.SW02 - Limit Switch Integration

**Name of TPM:** Bridge Position Limit Switch System

**Purpose of TPM:** Ensure limit switches accurately detect bridge fully open and fully closed positions, preventing motor over-travel.

**Source Requirement:** REQ.06 (Bridge position control and safety)

**Risk Level:** High

**What should be measured:**

-   Limit switch activation state (pressed/released)
-   Motor stop response time when limit reached
-   Consistency of limit switch readings
-   Safety interlock functionality in motor control functions

**How should it be measured:**

-   Manually press each limit switch and verify Serial output
-   Test `isBridgeFullyOpen()` and `isBridgeFullyClosed()` functions
-   Monitor motor behavior in STATE3 and STATE7
-   Verify motor stops within 100ms of limit activation
-   Test both manual and automatic modes

**How often should it be measured:**

-   Every code upload during development
-   Before each operational test
-   After any mechanical adjustments
-   Daily during integration testing

**Measures of success:**

-   Limit switches read LOW when pressed, HIGH when released
-   Motor stops immediately (<100ms) when limit reached
-   `motorOpen()` refuses to run when top limit active
-   `motorClose()` refuses to run when bottom limit active
-   API state endpoint correctly reports limit switch status

**Measures of failure:**

-   Limit switch readings inverted or incorrect
-   Motor continues running after limit reached
-   Motor allows movement in wrong direction at limits
-   Inconsistent readings (bouncing)
-   System crashes when limit switch triggered

**Possible causes of failure:**

-   Incorrect INPUT_PULLUP configuration
-   Wiring issues (reversed polarity)
-   Mechanical misalignment
-   Software logic errors in limit checking
-   Switch bounce not debounced

---

## TPM.SW03 - State Machine Sequencing

**Name of TPM:** Bridge Control State Machine

**Purpose of TPM:** Verify that the bridge control system progresses through all states in correct sequence with proper timing.

**Source Requirement:** REQ.02, REQ.03, REQ.04 (Sequential operation of bridge, lights, and gates)

**Risk Level:** High

**What should be measured:**

-   State transition timing (STATE0→STATE1→...→STATE8→STATE0)
-   State duration compliance with design specifications
-   Light patterns for each state
-   Boom gate positions for each state
-   Motor behavior in each state

**How should it be measured:**

-   Monitor Serial output for state transitions
-   Use stopwatch to verify timing delays
-   Visual inspection of traffic lights and boom gates
-   API `/api/state` endpoint polling
-   Log complete cycle from boat detection to return to idle

**How often should it be measured:**

-   Every functional test cycle
-   After any state machine logic changes
-   During integration testing
-   Before system demonstration

**Measures of success:**
| State | Duration | Traffic Lights | Boat Lights | Boom Gate | Motor |
|-------|----------|----------------|-------------|-----------|-------|
| STATE0 | Indefinite | GREEN | RED | Raised | Stopped |
| STATE1 | 3s | YELLOW | RED | Raised | Stopped |
| STATE2 | 5s | RED | RED | Raised | Stopped |
| STATE2B | 3s | RED | RED | Lowered | Stopped |
| STATE3 | Until limit/10s | RED | RED | Lowered | Opening |
| STATE4 | 3s | RED | YELLOW | Lowered | Stopped |
| STATE5 | 10s | RED | GREEN | Lowered | Stopped |
| STATE6 | 3s | RED | YELLOW | Lowered | Stopped |
| STATE7 | Until limit/10s | RED | RED | Lowered | Closing |
| STATE8 | 2s | YELLOW | RED | Raised | Stopped |

**Measures of failure:**

-   State skipped or repeated
-   Timing significantly off (>500ms error)
-   Incorrect light patterns
-   Boom gate in wrong position
-   Motor running in wrong state
-   System stuck in a state

**Possible causes of failure:**

-   Logic errors in state machine switch statement
-   Incorrect delay constants
-   `millis()` overflow handling issues
-   Missing `stateActionsLogged` reset
-   Race conditions in state transitions

---

## TPM.SW04 - WiFi API Communication

**Name of TPM:** WiFi Access Point and REST API

**Purpose of TPM:** Ensure reliable WiFi connectivity and API communication between ESP32 and control interface.

**Risk Level:** Medium

**What should be measured:**

-   WiFi AP establishment and stability
-   API endpoint response times
-   Authentication token generation and validation
-   CORS header configuration
-   JSON payload parsing accuracy

**How should it be measured:**

-   Connect device to "group56bridge" WiFi network
-   Use curl commands or Postman to test all endpoints
-   Monitor Serial output for HTTP requests
-   Measure API response times
-   Test concurrent connections

**How often should it be measured:**

-   After WiFi configuration changes
-   During network integration testing
-   Before frontend integration
-   Weekly during development

**Measures of success:**

-   WiFi AP accessible at 192.168.4.1
-   Successful login returns valid token
-   All endpoints respond within 500ms
-   CORS headers allow frontend access
-   State endpoint returns correct JSON structure
-   Command endpoint accepts and executes actions
-   Unauthorized requests properly rejected

**Measures of failure:**

-   Cannot connect to WiFi AP
-   API endpoints return 500 errors
-   Authentication bypass possible
-   CORS errors in browser
-   Malformed JSON responses
-   Commands not executed
-   Token validation fails

**Possible causes of failure:**

-   Incorrect WiFi credentials
-   IP address conflicts
-   ArduinoJson memory allocation issues
-   Missing CORS headers
-   Token generation/storage bugs
-   Request parsing errors

---

## TPM.SW05 - Manual Override System

**Name of TPM:** Manual Control Override Mode

**Purpose of TPM:** Verify manual override allows direct control of all bridge components while preventing automatic operation.

**Source Requirement:** REQ.07 (Manual control capability)

**Risk Level:** Medium

**What should be measured:**

-   Override enable/disable functionality
-   Manual motor control (open/close)
-   Individual light control (traffic and boat)
-   Boom gate control in override mode
-   State machine suspension during override
-   Proper return to automatic mode

**How should it be measured:**

-   Enable override via API command
-   Test all manual control actions
-   Verify state machine stops updating
-   Confirm ultrasonic detection disabled
-   Test disable override and state recovery

**How often should it be measured:**

-   During each integration test
-   After override logic changes
-   Before demonstrations requiring manual control
-   Monthly during operation

**Measures of success:**

-   `enableOverride` command sets `manualOverrideActive = true`
-   State machine stops progressing
-   Ultrasonic detection disabled
-   All manual commands execute (open, close, light changes)
-   Limit switches still enforced
-   `disableOverride` returns to correct state
-   LED indicates override status

**Measures of failure:**

-   Override doesn't prevent state transitions
-   Manual commands ignored
-   Automatic detection occurs during override
-   Cannot exit override mode
-   System crashes during override
-   Limit switches bypassed
-   Incorrect state after override disabled

**Possible causes of failure:**

-   Missing `manualOverrideActive` checks in loop
-   Race condition between manual and automatic control
-   Incorrect state recovery logic
-   API command not updating flag
-   Missing return statements in loop

---

## TPM.SW06 - Safety Interlocks

**Name of TPM:** Motor Safety and Limit Protection

**Purpose of TPM:** Ensure motor safety functions prevent damage and unsafe conditions.

**Source Requirement:** REQ.08 (System safety requirements)

**Risk Level:** High

**What should be measured:**

-   Motor stop at limit switches
-   Prevention of opening when already open
-   Prevention of closing when already closed
-   Emergency stop functionality
-   Motor timeout behavior

**How should it be measured:**

-   Attempt to open bridge when top limit active
-   Attempt to close bridge when bottom limit active
-   Test motor timeout in STATE3 and STATE7
-   Verify Serial output shows safety messages
-   Monitor motor power during interlocks

**How often should it be measured:**

-   Every power-on test
-   After motor control code changes
-   Daily during testing phase
-   Before any live demonstration

**Measures of success:**

-   `motorOpen()` returns immediately if `isBridgeFullyOpen()`
-   `motorClose()` returns immediately if `isBridgeFullyClosed()`
-   Motor stops within 100ms of limit switch activation
-   Timeout prevents indefinite motor operation
-   Serial output shows "Cannot open - already at top limit" etc.

**Measures of failure:**

-   Motor continues at limits
-   Safety checks bypassed
-   Motor runs indefinitely
-   Physical damage to mechanism
-   No error messages in Serial output

**Possible causes of failure:**

-   Missing limit checks in motor functions
-   Limit switch logic inverted
-   Override mode bypassing safety
-   Timeout values too large
-   Missing `motorStop()` calls

---

## Test Execution Checklist

### Pre-Test Setup

-   [ ] ESP32 powered and connected
-   [ ] Serial Monitor open at 115200 baud
-   [ ] All sensors and switches wired correctly
-   [ ] WiFi connection established
-   [ ] Mechanical system operational

### Test Sequence

1. **Power-On Diagnostics**

    - Verify Serial initialization messages
    - Check limit switch initial states
    - Confirm WiFi AP started

2. **Sensor Tests (TPM.SW01)**

    - Test both ultrasonic sensors
    - Verify detection threshold
    - Confirm continuous detection requirement

3. **Limit Switch Tests (TPM.SW02)**

    - Press and release each switch manually
    - Verify motor safety interlocks
    - Check API state reporting

4. **State Machine Test (TPM.SW03)**

    - Trigger automatic cycle
    - Time each state duration
    - Verify light and gate sequences

5. **API Tests (TPM.SW04)**

    - Login authentication
    - State polling
    - Command execution
    - Logout

6. **Manual Override Tests (TPM.SW05)**

    - Enable override
    - Test all manual controls
    - Disable and verify recovery

7. **Safety Tests (TPM.SW06)**
    - Test all safety interlocks
    - Verify emergency stops
    - Confirm timeout behavior

### Test Results Template

```
Test Date: __________
Tester: __________
Firmware Version: __________

TPM.SW01 - Ultrasonic Detection: PASS / FAIL
Notes: _________________________________

TPM.SW02 - Limit Switches: PASS / FAIL
Notes: _________________________________

TPM.SW03 - State Machine: PASS / FAIL
Notes: _________________________________

TPM.SW04 - WiFi API: PASS / FAIL
Notes: _________________________________

TPM.SW05 - Manual Override: PASS / FAIL
Notes: _________________________________

TPM.SW06 - Safety Interlocks: PASS / FAIL
Notes: _________________________________

Overall System Status: PASS / FAIL
```

---

## Known Issues and Limitations

1. **Ultrasonic Sensor Range**: Maximum reliable range ~400cm, minimum ~2cm
2. **WiFi Range**: Limited to ~10m from ESP32 depending on environment
3. **Motor Timing**: Actual open/close times may vary with mechanical friction
4. **State Machine**: No interrupt-based detection, relies on polling in loop()
5. **Serial Buffer**: Heavy serial output may cause minor timing delays

---

## Revision History

| Version | Date       | Changes               | Author |
| ------- | ---------- | --------------------- | ------ |
| 1.0     | 2025-10-29 | Initial documentation | -      |
