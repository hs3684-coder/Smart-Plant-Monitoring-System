// Viral Science — Blynk IoT Smart Plant Monitoring (ESP8266)
// Uses credentials provided by the user in the prior message.

// -------------------- Pins & Blynk --------------------
/*
Relay D3
Btn   D7
Soil  A0
PIR   D5
SDA   D2
SCL   D1
Temp  D4 (DHT11)
*/

// ---------- ESP8266 D-pin compatibility (only if not already defined) ----------
#ifndef D0
  #define D0 16
  #define D1 5
  #define D2 4
  #define D3 0
  #define D4 2
  #define D5 14
  #define D6 12
  #define D7 13
  #define D8 15
#endif

// -------------------- Blynk Template & Token (from your message) --------------------
#define BLYNK_TEMPLATE_ID   "TMPL3Y16qR6NW"
#define BLYNK_TEMPLATE_NAME "Smart Plant"
#define BLYNK_AUTH_TOKEN    "trn5r55oZRU6LXiAnXmVH-9zMmRz6zYP"

// ---------- Credentials (from your message) ----------
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Airtel_H305";
char pass[] = "passH305";

// -------------------- Includes --------------------
#include <LiquidCrystal_I2C.h>
#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>

// -------------------- LCD --------------------
LiquidCrystal_I2C lcd(0x3F, 16, 2); // change to 0x27 if your LCD uses that address

// -------------------- Sensors --------------------
#define DHT_PIN  D4
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

#define SOIL_PIN A0
#define PIR_PIN  D5

// -------------------- Relay & Button --------------------
#define RELAY_PIN_1   D3
#define PUSH_BUTTON_1 D7
#define VPIN_BUTTON_1 V12

// -------------------- Virtual Pins --------------------
#define VPIN_TEMP   V0
#define VPIN_HUM    V1
#define VPIN_SOIL   V3
#define VPIN_PIRLED V5
#define VPIN_PIR_EN V6   // toggle for PIR

// -------------------- Globals --------------------
BlynkTimer timer;
WidgetLED pirLed(VPIN_PIRLED);

volatile int relay1State = LOW;
int pushButton1State = HIGH;
int lastButtonRead   = HIGH;
unsigned long lastDebounce = 0;
const unsigned long debounceMs = 35;

int PIR_ToggleValue = 0;  // default OFF

// Cache last-sent values to avoid spamming writes
float lastT = NAN, lastH = NAN;
int   lastSoil = -1;
bool  lastMotion = false;

// -------------------- Helpers --------------------
int readSoilPercent() {
  // Average a few samples for stability
  const int N = 8;
  long sum = 0;
  for (int i=0; i<N; i++) {
    sum += analogRead(SOIL_PIN); // ESP8266 ADC: 0..1023
    delay(2);
  }
  int raw = sum / N;
  raw = constrain(raw, 0, 1023);
  // Map "wet" (low voltage) -> high %
  int pct = map(raw, 0, 1023, 100, 0);
  return constrain(pct, 0, 100);
}

void safeLCDPrint(int col, int row, const String& s) {
  lcd.setCursor(col, row);
  lcd.print(s);
  // pad remainder to clear old chars
  int pad = max(0, 16 - col - (int)s.length());
  while (pad-- > 0) lcd.print(' ');
}

// -------------------- Timed tasks --------------------
void taskReadDHT() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    Serial.println("DHT read failed");
    return;
  }
  // Only send if changed notably
  if (isnan(lastT) || fabs(t - lastT) >= 0.3) {
    Blynk.virtualWrite(VPIN_TEMP, t);
    lastT = t;
  }
  if (isnan(lastH) || fabs(h - lastH) >= 1.0) {
    Blynk.virtualWrite(VPIN_HUM, h);
    lastH = h;
  }
  safeLCDPrint(0, 0, "T:" + String(t,1));
  safeLCDPrint(8, 0, "H:" + String((int)h));
}

void taskReadSoil() {
  int soil = readSoilPercent();
  if (soil != lastSoil) {
    Blynk.virtualWrite(VPIN_SOIL, soil);
    lastSoil = soil;
  }
  safeLCDPrint(0, 1, "S:" + String(soil));
}

void taskCheckMotion() {
  if (PIR_ToggleValue != 1) {
    // disabled
    if (lastMotion) {
      pirLed.off();
      lastMotion = false;
    }
    safeLCDPrint(5, 1, "M:OFF");
    return;
  }

  bool motion = digitalRead(PIR_PIN) == HIGH;
  if (motion != lastMotion) {
    if (motion) {
      // Use your existing event name from Blynk Console:
      Blynk.logEvent("pirmotion", "WARNING! Motion Detected!");
      pirLed.on();
      safeLCDPrint(5, 1, "M:ON ");
    } else {
      pirLed.off();
      safeLCDPrint(5, 1, "M:OFF");
    }
    lastMotion = motion;
  }
}

void taskUpdateWateringLabel() {
  safeLCDPrint(11, 1, relay1State == HIGH ? "W:ON " : "W:OFF");
}

// -------------------- Button & Relay --------------------
void handleButton() {
  int reading = digitalRead(PUSH_BUTTON_1);
  if (reading != lastButtonRead) {
    lastDebounce = millis();
    lastButtonRead = reading;
  }
  if ((millis() - lastDebounce) > debounceMs) {
    if (reading == LOW && pushButton1State == HIGH) {
      // falling edge
      relay1State = !relay1State;
      digitalWrite(RELAY_PIN_1, relay1State);
      Blynk.virtualWrite(VPIN_BUTTON_1, relay1State);
      taskUpdateWateringLabel();
    }
    pushButton1State = reading;
  }
}

// -------------------- Blynk Glue --------------------
BLYNK_CONNECTED() {
  Blynk.syncVirtual(VPIN_BUTTON_1, VPIN_PIR_EN);
}

BLYNK_WRITE(VPIN_BUTTON_1) {
  relay1State = param.asInt();
  digitalWrite(RELAY_PIN_1, relay1State);
  taskUpdateWateringLabel();
}

BLYNK_WRITE(VPIN_PIR_EN) {
  PIR_ToggleValue = param.asInt();
}

// -------------------- Setup & Loop --------------------
void setup() {
  Serial.begin(115200);

  // LCD init (your library supports begin)
  lcd.begin(16, 2);
  lcd.backlight();

  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY_PIN_1, OUTPUT);
  digitalWrite(RELAY_PIN_1, LOW);

  pinMode(PUSH_BUTTON_1, INPUT_PULLUP);

  Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);
  dht.begin();

  safeLCDPrint(0, 0, "  Initializing  ");
  for (int a = 5; a <= 10; a++) { lcd.setCursor(a, 1); lcd.print("."); delay(200); }
  safeLCDPrint(11, 1, "W:OFF");

  // Timers: slower = stable & within Blynk limits
  timer.setInterval(2000L, taskReadSoil);       // every 2s
  timer.setInterval(3000L, taskReadDHT);        // every 3s
  timer.setInterval(200L,  taskCheckMotion);    // every 200ms
  timer.setInterval(50L,   handleButton);       // poll button
}

void loop() {
  Blynk.run();
  timer.run();
}
