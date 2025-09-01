# ENGG_GROUP56

1. System Overview

3.1 Automation

The system automates bridge operation based on ship detection:





Detection: Sensor triggers ship arrival, signaling boats to wait and traffic to stop.



Bridge Opening: After a delay, opens the bridge with regulated speed, signaling boat passage.



Boat Passage: Allows boats through when open.



Bridge Closing: After boat clearance, closes the bridge with regulated speed, resuming traffic.



Safety Checks: Ensures bridge is clear before opening, using sensors or manual confirmation.

3.2 Remote UI

Hosted on an external server for scalability:





Display: Real-time bridge state with animations, ship status with visual emphasis, indicator states.



Controls: Manual open/close bridge, clear detection, toggle indicators.



Access: Browser-based, secured with authentication, responsive for desktop/mobile.

3.3 System Architecture





ESP32 Client: Handles sensor inputs, actuation, indicators; sends/receives JSON.



External Server: Hosts UI, API for data processing and commands.



Communication: HTTP/JSON for updates and overrides.

4. Software Engineering Considerations

4.1 Data Flow





ESP32 sends JSON on state changes (e.g., ship detection, bridge state).



Server updates UI with received data and animations.



UI sends manual commands to server, which relays to ESP32.



ESP32 polls for commands, executes overrides if safe.

4.2 Components





ESP32 Firmware: Sensor handling, state machine for automation, indicator control, JSON communication.



Server Application: REST API, UI with HTML/CSS/JS, authentication, state management.



Libraries: ESP32: WiFi, HTTPClient, ArduinoJson; Server: Express, body-parser, basic-auth.

4.3 Implementation Plan





Requirements Refinement: Finalize sensors, indicators, UI features.



ESP32 Development: Firmware for hardware and JSON.



Server Development: API and UI on Node.js/Express.



Integration: Test communication, automation, overrides.



Optimization: Reduce latency (e.g., WebSockets), power usage.

4.4 Constraints and Risks





ESP32 memory limited; client code <200 KB.



Network instability: Retries, offline retention.



Latency: HTTP ~100-500ms; WebSockets for real-time.



Security: Authentication, API key.



Power: 12V 5A supply for hardware; optimize ESP32 for efficiency.



Scalability: Server for multiple clients.