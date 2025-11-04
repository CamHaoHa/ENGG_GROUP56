STATE 0: IDLE (Bridge Closed, Traffic Flowing)
├─ Traffic lights: GREEN
├─ Boat lights: RED
├─ Boom gates: UP (traffic passes)
├─ Monitor ultrasonic sensors for boat
└─ If boat detected → STATE 1

STATE 1: BOAT DETECTED
├─ Traffic lights: YELLOW (warning to slow down , prepare to stop)
├─ Boat lights: RED
├─ Boom gates: UP (still allow traffic to pass, think about the vehicles which are still on the bridge, vehicles on bridge can exit)
├─ Wait 3s for traffic to slow
└─ Auto transition → STATE 2

STATE 2: CLEARING TRAFFIC
├─ Traffic lights: RED
├─ Boat lights: RED
├─ Boom gates: UP (ensure the bridge is clear)
├─ Speaker: ACTIVE (warning beeps to ask all traffic out of the bridge)
├─ Wait 5s for traffic to clear then boom gates down
└─ Auto transition → STATE 2B

STATE 2B: TRAFFIC CLEAR (CONFIRMATION) : buffer period after clearing traffic and before opening bridge
├─ Traffic lights: RED
├─ Boat lights: RED
├─ Boom gates: DOWN (no more vehicles or traffic on the bridge)
├─ Speaker : STOP
├─ wait 3s
└─ Auto transition -> State 3

STATE 3: OPENING BRIDGE
├─ Traffic lights: RED
├─ Boat lights: RED (boats should not move at this stage, the bridge is not fully open)
├─ Motor: OPEN direction
├─ Monitor top limit switch
└─ When top switch triggered → STATE 4

STATE 4: BRIDGE FULLY OPEN, YELLOW LIGHTS WARNING BOAT GET READY TO MOVE
├─ Traffic lights: RED
├─ Boat lights: YELLOW (for 3s then turn GREEN)
├─ Motor: STOPPED
└─ Auto transition (after 3s) -> STATE 5

STATE 5: BRIDGE OPEN (Waiting) ( two exit conditions, no boat detected for 3s, or 10 second time out)
├─ Traffic lights: RED
├─ Boat lights: GREEN
├─ Motor: STOPPED
├─ Monitor ultrasonic sensors
└─ When no boat detected for 3s → STATE 5. OR after 10s if there still a boat, we still need to STATE 6.

STATE 6: STOPPING BOATS
├─ Traffic lights: RED
├─ Boat lights: YELLOW
├─ Motor: STOPPED
└─ Auto transition (after 3s) -> STATE 7

STATE 7: CLOSING BRIDGE
├─ Traffic lights: RED
├─ Boat lights: RED
├─ Motor: CLOSE direction
├─ Monitor bottom limit switch
└─ When bottom switch triggered → STATE 8


STATE 8: BRIDGE FULLY CLOSED (Preparing Traffic)
├─ Traffic lights: RED→YELLOW
├─ Boat lights: RED
├─ Boom gates: UP
├─ Wait 2s
└─ Auto transition → STATE 0

MANUAL OVERRIDE MODE:
├─ Disables automatic state transitions
├─ Allows direct open/close commands
├─ Safety checks still active
└─ Can return to auto mode at any time


====

