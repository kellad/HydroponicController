
# ESP8266 PWM Motor Control Scheduler with Web Interface

This project implements a time-based motor control system using the ESP8266. It toggles a motor on/off with PWM duty cycle based on user-defined schedules for daytime and nighttime periods. The device also hosts a web interface for configuration.

## Features

- **PWM motor control** with adjustable duty cycle
- **Day/Night scheduling**: separate ON/OFF durations for each
- **Web interface** for setting all parameters
- **DS1302 RTC** support for timekeeping
- **EEPROM storage** of settings
- **LED indicator** (blinking interval reflects motor state)
- **Relay control** to switch motor power
- **Static IP configuration** for reliable Wi-Fi access

## Hardware Requirements

- **ESP8266 module** (e.g., NodeMCU or Wemos D1 mini)
- **DS1302 RTC module**
- **Relay module** for switching motor
- **Motor (PWM controllable)**
- Optional: LED (uses built-in LED by default)

### Pin Connections

| Module       | ESP8266 Pin |
|--------------|-------------|
| DS1302 CLK   | D4 (GPIO2)  |
| DS1302 DAT   | D3 (GPIO0)  |
| DS1302 RST   | D2 (GPIO4)  |
| Relay IN     | D7 (GPIO13) |
| PWM Output   | D5 (GPIO14) |

## Software Features

### Scheduler Logic
The day is split into two periods:

- **Day Period:** From `dayStartHour:dayStartMin` to `nightStartHour:nightStartMin`
- **Night Period:** All other times

Each period has its own:
- `On Time` (minutes motor stays on)
- `Off Time` (minutes motor stays off)

PWM duty cycle (0–100%) is used when the motor is on.

## Web Interface

Access the web interface via browser by visiting the static IP (default: `http://192.168.1.8`).

### Adjustable Parameters:

- Day/Night ON/OFF durations (in minutes)
- Day start time and Night start time
- PWM duty cycle (0–100%)
- Current date and time (for RTC)

Changes are saved to EEPROM and applied immediately.

## Wi-Fi Configuration

Modify the following lines in the code for your Wi-Fi network:

```cpp
const char* ssid = "YourSSID";
const char* password = "YourPassword";
```

Static IP configuration (can be changed in `setup()`):

```cpp
IPAddress local_IP(192, 168, 1, 8);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
```

## EEPROM

Settings stored in EEPROM include:

- Day/Night ON/OFF durations
- Start times
- PWM duty

## How to Upload

1. Install [Arduino IDE](https://www.arduino.cc/en/software)
2. Install **ESP8266 Board Support** via Boards Manager
3. Install required libraries:
   - `ESP8266WiFi`
   - `ESP8266WebServer`
   - `Ticker`
   - `EEPROM`
   - `DS1302` (may require manual installation)
4. Select the correct ESP8266 board
5. Upload the sketch

## Notes

- RTC must be manually set once from the web interface
- System time advances in software using `millis()` but resets from RTC on reboot
- You can re-enable Access Point mode instead of Wi-Fi client by uncommenting:
  ```cpp
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  ```

## License

MIT License. Free for personal and commercial use with attribution.
