// ESP8266 PWM Motor Control Scheduler with Web Interface
// Developed for Arduino IDE

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Ticker.h>
#include <EEPROM.h>
#include <DS1302.h>

// Access Point credentials
const char* ap_ssid = "PWM_Controller";
const char* ap_password = "12345678";

const char* ssid = "FiberHGW_ZY281D";
const char* password = "Y7fWDWPUEUHe";

ESP8266WebServer server(80);

// Scheduler settings
float dayOn = 1.0, dayOff = 15.0, nightOn = 1.0, nightOff = 60.0;
int dayStartHour = 6, dayStartMin = 0, nightStartHour = 20, nightStartMin = 0;
int pwmDuty = 50; // 0 - 100

// Internal state
unsigned long lastSwitchTime = 0;
bool pumpState = false;
bool isDay = true;

Ticker pwmTicker;
Ticker ledTicker;

// Time from DS1302
int currentYear = 2024, currentMonth = 1, currentDay = 1;
int currentHour = 0, currentMin = 0;
unsigned long lastMinuteUpdate = 0;

// DS1302 wiring
const int rtcCLK = D4;  // CLK
const int rtcDAT = D3;  // IO
const int rtcRST = D2;  // RST
DS1302 rtc(rtcRST, rtcDAT, rtcCLK);

const int pwmPin = D5; // GPIO14
const int relayPin = D7;

void saveSettings() {
  EEPROM.begin(64);
  EEPROM.put(0, dayOn);
  EEPROM.put(4, dayOff);
  EEPROM.put(8, nightOn);
  EEPROM.put(12, nightOff);
  EEPROM.put(16, dayStartHour);
  EEPROM.put(20, dayStartMin);
  EEPROM.put(24, nightStartHour);
  EEPROM.put(28, nightStartMin);
  EEPROM.put(32, pwmDuty);
  EEPROM.commit();
  EEPROM.end();
}

void loadSettings() {
  EEPROM.begin(64);
  EEPROM.get(0, dayOn);
  EEPROM.get(4, dayOff);
  EEPROM.get(8, nightOn);
  EEPROM.get(12, nightOff);
  EEPROM.get(16, dayStartHour);
  EEPROM.get(20, dayStartMin);
  EEPROM.get(24, nightStartHour);
  EEPROM.get(28, nightStartMin);
  EEPROM.get(32, pwmDuty);
  EEPROM.end();
}

void handleRoot() {
  unsigned long now = millis();
  bool isCurrentDay = withinDayPeriod();
  float onTime = isCurrentDay ? dayOn : nightOn;
  float offTime = isCurrentDay ? dayOff : nightOff;
  unsigned long interval = (pumpState ? onTime : offTime) * 60000;
  unsigned long timeLeft = (lastSwitchTime + interval > now) ? (lastSwitchTime + interval - now) : 0;
  unsigned int minutesLeft = timeLeft / 60000;
  unsigned int secondsLeft = (timeLeft % 60000) / 1000;
  String html = "<html><body><h1>PWM Scheduler</h1>"
                "<form method='GET' action='/set'>"
                "Day On (min): <input name='dayon' type='text' value='" + String(dayOn) + "'><br>"
                "Day Off (min): <input name='dayoff' type='text' value='" + String(dayOff) + "'><br>"
                "Night On (min): <input name='nighton' type='text' value='" + String(nightOn) + "'><br>"
                "Night Off (min): <input name='nightoff' type='text' value='" + String(nightOff) + "'><br>"
                "Day Start (hh:mm): <input name='dayh' type='text' value='" + String(dayStartHour) + "'>:<input name='daym' type='text' value='" + String(dayStartMin) + "'><br>"
                "Night Start (hh:mm): <input name='nighth' type='text' value='" + String(nightStartHour) + "'>:<input name='nightm' type='text' value='" + String(nightStartMin) + "'><br>"
                "PWM Duty (0-100): <input name='duty' type='text' value='" + String(pwmDuty) + "'><br>"
                "Date (yyyy-mm-dd): <input name='year' type='text' value='" + String(currentYear) + "'>-<input name='month' type='text' value='" + String(currentMonth) + "'>-<input name='day' type='text' value='" + String(currentDay) + "'><br>"
                "Current Time (hh:mm): <input name='hour' type='text' value='" + String(currentHour) + "'>:<input name='min' type='text' value='" + String(currentMin) + "'><br>"
                "<input type='submit'></form></body></html>";
    html += "<br><b>" + String(pumpState ? "Time left to OFF: " : "Time left to ON: ") + String(minutesLeft) + " min " + String(secondsLeft) + " sec</b><br>";
    html += "<br><b>Current Date and Time: " + String(currentYear) + "-" + String(currentMonth) + "-" + String(currentDay) + " " + String(currentHour) + ":" + String(currentMin) + "</b><br>";
  server.send(200, "text/html", html);
}

void handleSet() {
  if (server.hasArg("dayon")) dayOn = server.arg("dayon").toFloat();
  if (server.hasArg("dayoff")) dayOff = server.arg("dayoff").toFloat();
  if (server.hasArg("nighton")) nightOn = server.arg("nighton").toFloat();
  if (server.hasArg("nightoff")) nightOff = server.arg("nightoff").toFloat();
  if (server.hasArg("dayh")) dayStartHour = server.arg("dayh").toInt();
  if (server.hasArg("daym")) dayStartMin = server.arg("daym").toInt();
  if (server.hasArg("nighth")) nightStartHour = server.arg("nighth").toInt();
  if (server.hasArg("nightm")) nightStartMin = server.arg("nightm").toInt();
  if (server.hasArg("duty")) pwmDuty = server.arg("duty").toInt();
  if (server.hasArg("hour")) currentHour = server.arg("hour").toInt();
  if (server.hasArg("min")) currentMin = server.arg("min").toInt();
  if (server.hasArg("year")) currentYear = server.arg("year").toInt();
  if (server.hasArg("month")) currentMonth = server.arg("month").toInt();
  if (server.hasArg("day")) currentDay = server.arg("day").toInt();
  rtc.writeProtect(false);
  rtc.halt(false);
  Time newTime(currentYear, currentMonth, currentDay, currentHour, currentMin, 0, Time::kSunday);
  rtc.time(newTime);
  saveSettings();
  server.sendHeader("Location", "/");
  server.send(303);
}

bool withinDayPeriod() {
  int now = currentHour * 60 + currentMin;
  int dayStart = dayStartHour * 60 + dayStartMin;
  int nightStart = nightStartHour * 60 + nightStartMin;
  return (now >= dayStart && now < nightStart);
}

void updatePump() {
  unsigned long now = millis();
  bool isCurrentDay = withinDayPeriod();
  float onTime = isCurrentDay ? dayOn : nightOn;
  float offTime = isCurrentDay ? dayOff : nightOff;

  unsigned long interval = (pumpState ? onTime : offTime) * 60000; // convert min to ms
  if (now - lastSwitchTime >= interval) {
    pumpState = !pumpState;
    lastSwitchTime = now;
    analogWrite(pwmPin, pumpState ? map(pwmDuty, 0, 100, 0, 1023) : 0);
    
    if(pumpState) { 
      digitalWrite(relayPin,HIGH); 
    }
    else {
      digitalWrite(relayPin,LOW); 
    } 

    ledTicker.detach();
    ledTicker.attach_ms(pumpState ? 200 : 800, blinkLed);
  }
}

void updateClock() {
  if (millis() - lastMinuteUpdate >= 60000) {
    lastMinuteUpdate += 60000;
    currentMin++;
    if (currentMin >= 60) {
      currentMin = 0;
      currentHour++;
      if (currentHour >= 24) currentHour = 0;
    }
  }
}

const int ledPin = LED_BUILTIN;

// Removed duplicate Ticker declaration

void blinkLed() {
  static bool ledState = false;
  ledState = !ledState;
  digitalWrite(ledPin, ledState ? LOW : HIGH);
}

void setup() {
  Time t = rtc.time();
  currentYear = t.yr;
  currentMonth = t.mon;
  currentDay = t.date;
  currentHour = t.hr;
  currentMin = t.min;
  pinMode(pwmPin, OUTPUT);
  pinMode(relayPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  digitalWrite(relayPin, LOW);
  analogWriteFreq(20000);
  analogWrite(pwmPin, 0);

  loadSettings();

  //WiFi.mode(WIFI_AP);
  //WiFi.softAP(ap_ssid, ap_password);

  
  IPAddress local_IP(192, 168, 1, 8);     // 
  IPAddress gateway(192, 168, 1, 1);        // The router’s IP
  IPAddress subnet(255, 255, 255, 0);       // Typical subnet

  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }

  WiFi.begin(ssid, password);

  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.begin();

  ledTicker.attach_ms(pumpState ? 200 : 800, blinkLed);
}

void loop() {
  server.handleClient();
  updateClock();
  updatePump();
}
