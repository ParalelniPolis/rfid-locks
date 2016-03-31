# Paralelni Polis RFID Access System
This repository contains all the resources needed to build your own WiFi connected RFID access system based on ESP8266. For simplicity, we use the NodeMCU breakout board and program it through the Arduino IDE. The basic idea is, that once you scan your 13.56 MHz tag, the ESP8266 asks the server whether you are allowed to enter and it switches a relay to open the door.

## Components
- NodeMCU 1.0 v2 board (ESP8266)
- MFRC-522 RFID shield
- Relay Module
- Power source

## Pin Layout
### RC522:
- SDA  to  GPIO02 - D4
- SCK  to  GPIO14 - D5
- MOSI to  GPIO13 - D7
- MISO to  GPIO12 - D6
- IRQ      N/A
- GND  to  GND
- RST  to  GPIO00 - D3
- 3.3V to  3.3V

### Relay:
 - +  to  3.3V
 - -  to  GND
 - Signal to  D2

## Source Code Configuration
The source code in **locks.ino** needs to be adapted for your specific use case in the following parts:
- You have to replace WIFI_SSID with your WiFi SSID and PASSWORD with your WiFi password.
- If you use locks at more than one place, you should name each lock with CUSTOM_LOCK_IDENTIFIER
- You need to provide the host name and API URL, where you run the server part
- Update the SHA1 fingerprint from your server's certificate
- Provide random strings for SALT1 and SALT2 as an additional layer of UID security during transport and storage on the server
- The `check_auth` function needs to be updated with the exact formatting of API requests required by your server and the appropriate success response
 
## Coming Soon
We are working on a 3D model of a box which will have exact mounting points for all the components. The results will be shared here and you will be able to 3D print it on your own.
