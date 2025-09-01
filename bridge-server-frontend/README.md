Bridge Control System
IoT system for controlling a bridge and LEDs via an ESP32, with a Node.js web server for remote monitoring and control.
Directory Structure
bridge-control-system/
├── esp32/
│   └── esp32_client.ino        # ESP32 Arduino sketch
├── server/
│   ├── server.js              # Node.js/Express server
│   └── public/
│       └── control.js         # Frontend JavaScript
├── package.json               # Node.js dependencies
└── README.md                  # This file

Setup

ESP32:

Open esp32/esp32_client.ino in Arduino IDE or PlatformIO.
Update ssid, password, and serverUrl with your WiFi credentials and server address.
Upload to your ESP32 board.


Server:

Navigate to server/ and run npm install to install dependencies.
Run npm start to start the server on http://localhost:3000.
Ensure the server is accessible to the ESP32 (e.g., use a public IP or local network).


Access:

Open http://<server-ip>:3000 in a browser.
Log in with username admin and password admin.
Monitor bridge/LED status and send control commands.



Notes

Replace your-server-ip in esp32_client.ino with the actual server IP/domain.
For production, secure the server with HTTPS, proper authentication, and a database.
Ensure the ESP32 is connected to a WiFi network with internet access.
