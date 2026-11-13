// SABSTCN - CHECK POST 3 | ESP32 final firmware (sensors + GPS + SMS)
// Any sensor that is not connected shows "--" on the OLED and in the monitor.
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TinyGPSPlus.h>

// ----- EDIT THESE -----
#define PHONE       "+91XXXXXXXXXX"  // number that receives the SMS
#define STORED_LAT  26.2170          // fixed post / college position (Google Maps)
#define STORED_LNG  78.1828
#define PIR_WARM_MS 60000            // use 10000 for quick tests, 60000 for the demo
#define WIFI_SSID "vivo 1920"
#define WIFI_PASS "satwik12345"
#define PREFIX    "sabstcn-482/cp3"

// ----- Pins -----
#define PIR_PIN 14
#define TRIG    27
\


#define ECHO    13
#define MQ2_PIN 32
#define BUZZER  25
#define GPS_RX  16   // GPS TX -> D16
#define GPS_TX  17   // GPS RX -> D17
#define SIM_RX  26   // SIM800L TX -> D26
#define SIM_TX  33   // SIM800L RX -> D33

// ----- Thresholds -----
#define PIR_HOLD_MS 12000   // MOTION stays steady for 12 s (stops flicker)
#define NEAR_CM     10      // L2: object 5-10 cm
#define CLOSE_CM    5       // L3: object under 5 cm
#define SMOKE_DELTA 250     // rise above clean-air baseline = smoke
#define LEARN_MS    30000   // MQ-2 learns clean air for 30 s
#define SMS_GAP_MS  60000   // one SMS per minute at most

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
TinyGPSPlus gps;
HardwareSerial gpsSerial(1), sim(2);
bool oledOK, pirSeen, mqOn, simOK, simNet;
float base;
unsigned long mqT, pirHigh, smsT, oledT, simT;
const char* lastSms = "ready";

bool initOLED() {
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) return false;
  oled.setTextSize(1); oled.setTextColor(WHITE);
  return true;
}

float readDist() {                       // -1 = no valid echo (missing or out of range)
  float sum = 0; int ok = 0;
  for (int i = 0; i < 3; i++) {
    digitalWrite(TRIG, LOW);  delayMicroseconds(2);
    digitalWrite(TRIG, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG, LOW);
    long t = pulseIn(ECHO, HIGH, 25000);
    float d = t * 0.034 / 2;
    if (t > 0 && d >= 2 && d <= 400) { sum += d; ok++; }
    delay(15);
  }
  return ok >= 2 ? sum / ok : -1;
}

int readSmoke() {
  long s = 0;
  for (int i = 0; i < 16; i++) { s += analogRead(MQ2_PIN); delay(2); }
  return s / 16;
}

void feedGPS(unsigned long ms) {         // keeps the GPS parser fed while waiting
  unsigned long st = millis();
  while (millis() - st < ms) while (gpsSerial.available()) gps.encode(gpsSerial.read());
}

String readUntil(const char* tok, unsigned long ms) {
  String r; unsigned long st = millis();
  while (millis() - st < ms) {
    while (sim.available()) r += (char)sim.read();
    while (gpsSerial.available()) gps.encode(gpsSerial.read());
    if (r.indexOf(tok) >= 0 || r.indexOf("ERROR") >= 0) break;
  }
  return r;
}

String at(const char* cmd, const char* tok, unsigned long ms) {
  while (sim.available()) sim.read();
  sim.println(cmd);
  return readUntil(tok, ms);
}

void simCheck() {                        // module present? registered on the network?
  simOK = at("AT", "OK", 800).indexOf("OK") >= 0;
  String r = simOK ? at("AT+CREG?", "OK", 1000) : String("");
  simNet = simOK && (r.indexOf(",1") >= 0 || r.indexOf(",5") >= 0);
}

bool sendSMS(const String& msg) {
  if (!simOK || !simNet) return false;
  at("AT+CMGF=1", "OK", 1000);
  sim.print("AT+CMGS=\""); sim.print(PHONE); sim.println("\"");
  if (readUntil(">", 3000).indexOf(">") < 0) return false;
  sim.print(msg); sim.write(26);
  return readUntil("+CMGS", 12000).indexOf("+CMGS") >= 0;
}

const char* smsTxt() { return !simOK ? "--" : !simNet ? "no net" : lastSms; }

void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT_PULLDOWN);      // missing PIR reads 0, never noise
  pinMode(ECHO, INPUT_PULLDOWN);
  pinMode(TRIG, OUTPUT);
  pinMode(BUZZER, OUTPUT); digitalWrite(BUZZER, LOW);
  gpsSerial.setRxBufferSize(1024);
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  sim.begin(9600, SERIAL_8N1, SIM_RX, SIM_TX);
  oledOK = initOLED();
  if (oledOK) { oled.clearDisplay(); oled.setCursor(0, 20); oled.println("  SABSTCN  POST 3"); oled.println("  starting..."); oled.display(); }
  delay(3000);                           // SIM800L needs ~3 s to boot
  simCheck();
  Serial.println("CHECK POST 3 started");
}

void loop() {
  if (!oledOK && millis() - oledT > 3000) { oledT = millis(); oledOK = initOLED(); }

  // PIR: cannot be auto-detected, so it shows "--" until it has been seen working
  bool warm = millis() < PIR_WARM_MS;
  if (digitalRead(PIR_PIN)) { pirSeen = true; if (!warm) pirHigh = millis(); }
  bool pir = pirHigh && millis() - pirHigh < PIR_HOLD_MS;

  // Ultrasonic
  float d = readDist();
  bool usOK = d > 0;

  // MQ-2 (learns clean-air baseline first; a reading under 60 means "not connected")
  int s = readSmoke();
  bool learning = false, hit = false;
  if (s <= 60) mqOn = false;
  else {
    if (!mqOn) { mqOn = true; mqT = millis(); base = s; }
    if (millis() - mqT < LEARN_MS) { base = s; learning = true; }
    else { hit = (s - base) >= SMOKE_DELTA; if (!hit) base += (s - base) * 0.005; }
  }

  // Threat level
  bool nearObj = usOK && d <= NEAR_CM;
  int t = (pir || hit) ? 1 : 0;
  if (nearObj)              t = 2;
  if (usOK && d < CLOSE_CM) t = 3;
  if (nearObj && hit)       t = 4;
  digitalWrite(BUZZER, t >= 3 || (t == 2 && (millis() / 400) % 2));

  // Position: live GPS if it has a fix, otherwise the stored post position
  bool gpsWired = gps.charsProcessed() > 10;
  bool live = gps.location.isValid() && gps.location.age() < 10000;
  double la = live ? gps.location.lat() : STORED_LAT;
  double lo = live ? gps.location.lng() : STORED_LNG;

  if (t < 3 && millis() - simT > (simNet ? 60000UL : 20000UL)) { simT = millis(); simCheck(); }
  bool doSms = t >= 3 && (!smsT || millis() - smsT > SMS_GAP_MS);
  if (doSms) lastSms = "sending";

  // OLED
  if (oledOK) {
    oled.invertDisplay(t >= 3);
    oled.clearDisplay(); oled.setCursor(0, 0);
    oled.printf("CHECK POST 3    L%d\n", t);
    if (!pirSeen)  oled.println("PIR  : --");
    else if (warm) oled.printf("PIR  : warm %lus\n", (PIR_WARM_MS - millis()) / 1000);
    else           oled.printf("PIR  : %s\n", pir ? "MOTION" : "clear");
    if (!usOK)       oled.println("DIST : --");
    else if (d > 50) oled.println("DIST : clear");
    else             oled.printf("DIST : %.1f cm\n", d);
    if (!mqOn) oled.println("SMOKE: --");
    else       oled.printf("SMOKE: %d %s\n", s, learning ? "learn" : hit ? "DANGER" : "ok");
    oled.printf("GPS  : %s\n", !gpsWired ? "--" : live ? "LIVE" : "no fix");
    oled.printf("SMS  : %s\n", smsTxt());
    if (t >= 3) oled.printf("%.4f,%.4f", la, lo);
    oled.display();
  }

  // Serial log
  Serial.print("PIR:");   Serial.print(!pirSeen ? "--" : warm ? "warm" : pir ? "1" : "0");
  Serial.print(" D:");    if (usOK) Serial.print(d, 1); else Serial.print("--");
  Serial.print(" S:");    if (mqOn) Serial.print(s); else Serial.print("--");
  Serial.print(" GPS:");  Serial.print(!gpsWired ? "--" : live ? "LIVE" : "nofix");
  Serial.print(" SMS:");  Serial.print(smsTxt());
  Serial.print(" T:L");   Serial.println(t);

  // SMS alert (L3 / L4)
  if (doSms) {
    smsT = millis();
    String m = String("CHECKPOST-3 ALERT L") + String(t) + (live ? " [LIVE] " : " [STORED] ")
             + "https://maps.google.com/?q=" + String(la, 6) + "," + String(lo, 6);
    lastSms = sendSMS(m) ? "SENT" : "FAIL";
    Serial.print("ALERT "); Serial.println(lastSms);
  }
  feedGPS(150);
}