/*
 * ============================================================
 *  AI-BASED SMART GARBAGE SEGREGATION BIN
 *  Arduino Uno R3 — Final Code v2 (Proximity Sensor Enabled)
 * ============================================================
 *  - Proximity sensor detects metal -> routed straight to
 *    RECYCLABLE bin, bypassing the camera/AI step entirely.
 *  - IR sensor triggers the camera capture + classification
 *    flow for everything else.
 *  - Moisture sensor is the fallback if the ESP32-CAM/Flask
 *    pipeline times out or returns low confidence.
 * ============================================================
 */

#include <CheapStepper.h>
#include <Servo.h>
#include <SoftwareSerial.h>

// ─── Servo ────────────────────────────────────────────────
Servo servo1;
#define SERVO_PIN   7
int servoAng = 95;
#define SERVO_OPEN  10

// ─── Sensor Pins ──────────────────────────────────────────
#define ir      5
#define proxi   6
#define buzzer  12
int potPin = A0;

// ─── Stepper ──────────────────────────────────────────────
CheapStepper stepper(11, 10, 9, 8);

// ─── SoftwareSerial for ESP32-CAM ─────────────────────────
SoftwareSerial espSerial(2, 3);

// ─── 5-Bin positions ──────────────────────────────────────
const int BIN_ANGLES[5]  = {0, 60, 120, 180, 240};
const char* BIN_NAMES[5] = {"WET", "DRY", "PLASTIC", "BIOMEDICAL", "RECYCLABLE"};
int currentAngle = 0;

// ─── Constants ────────────────────────────────────────────
#define ESP_TIMEOUT_MS     10000
#define MOISTURE_THRESHOLD 20
int soil = 0;
int fsoil;

// ═══════════════════════════════════════════════════════════
void setup() {
  Serial.begin(9600);
  espSerial.begin(9600);

  pinMode(proxi,  INPUT_PULLUP);
  pinMode(ir,     INPUT);
  pinMode(buzzer, OUTPUT);

  servo1.attach(SERVO_PIN);
  stepper.setRpm(12);

  servo1.write(SERVO_OPEN);
  delay(2000);
  servo1.write(servoAng);
  delay(1000);

  currentAngle = 0;

  Serial.println("=== Smart Waste Sorter v2 — Proximity ENABLED ===");
  Serial.println("=== Drop waste into chute to begin ===");
  beep(2);
}

// ═══════════════════════════════════════════════════════════
void loop() {
  fsoil = 0;

  // ── Proximity sensor: metal -> RECYCLABLE, skip AI ────
  int prox = digitalRead(proxi);
  if (prox == 0) {
    Serial.println("[PROXIMITY] Metal detected → RECYCLABLE");
    tone(buzzer, 1000, 1000);
    sortWaste(4);
    return;
  }

  // ── IR presence check ─────────────────────────────────
  if (digitalRead(ir) == 0) {
    Serial.println("[IR] Object detected in chute");
    tone(buzzer, 1000, 500);
    delay(500);

    // ── Send CAPTURE to ESP32-CAM ────────────────────
    espSerial.println("CAPTURE");
    Serial.println("[ESP32] Capture command sent — waiting...");

    int category = waitForCategory(ESP_TIMEOUT_MS);

    if (category == -1) {
      Serial.println("[WARN] ESP32 timeout — using sensor fallback");
      category = sensorFallback();
    }

    Serial.print("[RESULT] → ");
    Serial.println(BIN_NAMES[category]);

    sortWaste(category);
  }
}

// ═══════════════════════════════════════════════════════════
int waitForCategory(unsigned long timeoutMs) {
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    if (espSerial.available()) {
      char c = espSerial.read();
      if (c == '9') {
        Serial.println("[WARN] Low AI confidence — sensor fallback");
        return sensorFallback();
      }
      if (c >= '0' && c <= '4') {
        return (int)(c - '0');
      }
    }
    delay(10);
  }
  return -1;
}

// ═══════════════════════════════════════════════════════════
int sensorFallback() {
  fsoil = 0;
  for (int i = 0; i < 3; i++) {
    soil = analogRead(potPin);
    soil = constrain(soil, 485, 1023);
    fsoil = (map(soil, 485, 1023, 100, 0)) + fsoil;
    delay(75);
  }
  fsoil = fsoil / 3;
  Serial.print("[FALLBACK] Moisture: ");
  Serial.print(fsoil);
  Serial.println("%");

  if (fsoil > MOISTURE_THRESHOLD) {
    Serial.println("[FALLBACK] → WET");
    return 0;
  }
  Serial.println("[FALLBACK] → DRY");
  return 1;
}

// ═══════════════════════════════════════════════════════════
void sortWaste(int category) {
  int targetAngle = BIN_ANGLES[category];
  int delta = targetAngle - currentAngle;

  Serial.print("[MOTOR] Moving to Bin ");
  Serial.print(category);
  Serial.print(" (");
  Serial.print(BIN_NAMES[category]);
  Serial.print(") delta=");
  Serial.println(delta);

  if (delta > 0) {
    stepper.moveDegreesCW(delta);
  } else if (delta < 0) {
    stepper.moveDegreesCCW(-delta);
  }
  currentAngle = targetAngle;
  delay(500);

  servo1.write(SERVO_OPEN);
  delay(2000);
  servo1.write(servoAng);
  delay(1000);

  beep(2);

  if (currentAngle > 0) {
    stepper.moveDegreesCCW(currentAngle);
    currentAngle = 0;
  }
  delay(500);

  stepper.off();
  Serial.println("[DONE] Ready for next item\n");
}

// ═══════════════════════════════════════════════════════════
void beep(int times) {
  for (int i = 0; i < times; i++) {
    tone(buzzer, 1000, 500);
    delay(600);
  }
}
