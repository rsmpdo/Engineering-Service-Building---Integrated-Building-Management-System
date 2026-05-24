1. Lighting Control System
Control: Managed directly by the ESP32 Dev module.
Hardware: LED strips and individual LEDs connected through relays.
Interface: Fully controllable using the web dashboard.

2. Fire Alarm System (Arduino Mega)
Sensors: DHT11 sensors used to detect fire conditions through temperature and humidity changes.
Actuators: Blinking red LEDs, buzzers, and a water vapor generator (motor).
Logic:
Timers: Motor activates vapor for 30 seconds and the alarm rings for 45 seconds.
Override: Manual web-based trigger for LEDs and buzzers.
Feedback: Includes a feedback LED indicator.

3. Car Park Gate System (Arduino Uno)
Access Control: Two RFID readers manage entry and exit.
Mechanism: Servo motor controls a 90-degree gate rotation.
Logic:
Auto-Close: Gate closes automatically after 3 seconds.
Safety: Tag reading is disabled while the gate is open and resumes once closed.
Override: Manual gate operation available via the website.

4. CCTV Surveillance System
Module: ESP32-CAM provides a live video feed.
Storage: Captured images are stored on an SD card.
Integration: Live feed embedded into the main control website.

5. Elevator Control System (Arduino Uno)
Motor: Controlled using a NEMA17 stepper motor.
Memory: Arduino EEPROM stores the current cabin position, retaining data after power loss.
Movement: Precise step counts enable accurate floor stops.
Communication: Target floors are sent via Serial communication with the current floor displayed.

Architecture & Stack
Master Node: ESP32 Dev Module hosts the website using WiFiServer and communicates with Arduino Mega and Unos via UART Serial.
Web Interface: Developed using HTML, CSS, and JavaScript.
