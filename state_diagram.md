# Bridge States

The bridge MUST be in one following distinct states, representing a specific condition of the bridge and its traffic control system.

## System Ready

-   Description: The bridge at the close default position, allowing pedestrian and traffic to cross 250 mm wide river. No ship is detected.
-   Bridge Position: Closed.
-   Traffic Signal: Green.
-   Boat Wait Signal: Red. (No Ship detected).
-   Remote UI: shows bridge closed, no ship detected, traffic allowed.
-   Sensor : keep monitor for ship detection
-   Local System: update remote UI via Wi-Fi, listen for override command.

## Ship Detected

-   Description: detect approaching ship(s), prepare the bridge for opening. The boat is signaled to wait.
-   Bridge Position: Closed.
-   Traffic Signal: Green (prepare to turn to red, traffic bar down)
-   Boat Wait Signal: Flashing Red
-   Remote UI: show "Ship detected, initiating Bridge Opening"
-   Sensor: ...
-   Local System: process sensor input, activate boat signal (flashing red), prepare to stop traffic

## Traffic Stopped (Opening bridge)

-   Description: stop pedestrian and traffic , ensure safety before the bridge opens.
-   Bridge Position: Closed.
-   Traffic Signal: RED.
-   Boat Wait Signal : Flashing Red.
-   Remote UI: shows "Traffic stopped, prepare to open bridge"
-   Sensor: ...
-   Local System: ensure it is safe to open the bridge, no traffic/pedestrian on the bridge.

## Bridge Opening

-   Description: the bridge is moving from closed to open position, 200mm clearance for the ship.
-   Bridge Position: partially open/close.
-   Traffic Signal: Red.
-   Boat Wait Signal : Flashing red
-   Remote UI: shows "Bridge Opening " (open to fully open position)
-   Sensor: ...
-   Local System: control motor to open bridge, monitor the process.

## Bridge Fully Open ( Ship Passage Allow) (Ship passing)

-   Description: the bridge is at fully open position, provide at least 200mm clearance and 200mm opening span for the ship to pass/
-   Bridge Position: fully open (200 mm clearance)
-   Traffic Signal: Red
-   Boat Wait Signal : Green
-   Remote UI : shows "bridge open, Ship passing"
-   Sensor: monitor ship position
-   Local System: listen to over ride command, error control.

## Ship Passed

-   Description: The sensor no longer detects the ship, indicating the ship has passed, and the bridge prepares to close.
-   Bridge position: fully open
-   Traffic Signal: Red
-   Boat Wait Signal: Red
-   Remote UI: shows "Ship passed, preparing to close bridge"
-   Sensor: ensure no coming ship.
-   Local System : confirm ship absence, init bridge closing.

## Bridge Closing:

-   Description: partially close/partially open ( moving from closed position to open position)
-   Bridge Position: partially close/ partially open
-   Traffic Signal: Red
-   Boat Wait Signal : Red.
-   Remote UI : shows "Bridge closing, closing to fully close"
-   Sensor:
-   Local System: control motor to close bridge, ensure bridge is fully close.

## Error State

-   Description: An error occur, such as motor problems.
-   Bridge Position: could be open/ close/ transitioning positions.
-   Traffic Signal: Red.
-   Boat Wait Signal: Red.
-   Remote UI: Error state, error details.
-   Sensor: monitor the bridge state
-   Local System: detect error, log error, halt system, notify admin/technician via local and user interface.

## Override State

-   Description: The remote user/interface sends a manual to overwrite automation sequences.
-   Bridge Position: ...
-   Traffic Signal: Red (Assume Red for safety)
-   Boat Wait Signal: Red (Assume Red for safety)
-   Remote UI: Show override/manual action and current system state.
-   Sensor: ...
-   Local System: process override command, update indicators.

## Assumption/ Question to concern

-   The system was built assume only one ship waiting to pass the bridge, how to scale it?
-   what happen there are two much ships on the queue : assume infinite ship waiting to pass, should bridge keep opening ? or should we allow a window of time for ship passing ( 1 minute)
-   What happen if ship not follow the rule or safety guide, not waiting with boat signal, ship that attack the bridge.
-   How can we ensure only we ship pass the bridge?
-   What happen if the ship not moving or pass when the bridge is fully open?
-   How long it takes for the bridge to fully open and close position?
-   How long it takes for one cycle?

## Next tasks

-   Data flow
-   Two specific diagrams for automation and override / manual
