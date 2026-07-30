#include <Arduino.h>
/*
 * SABSTCN — Smart Autonomous Border Surveillance
 * & Tactical Communication Network
 *
 * Phase 2 — Task 3: First Program
 * MITS Gwalior | ECE | 2024-25
 *
 * Author: Satwik Dubey
 * Board:  ESP32 DevKit V1
 */

#define LED_PIN    2        // Onboard LED (GPIO2)
#define BAUD_RATE  115200   // Serial monitor speed

void setup() {
  Serial.begin(BAUD_RATE);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);

  Serial.println("=====================================");
  Serial.println("   SABSTCN SYSTEM INITIALIZING...   ");
  Serial.println("   MITS Gwalior | ECE | 2024-25     ");
  Serial.println("=====================================");
  Serial.println("[INFO] LED initialized on GPIO2");
  Serial.println("[INFO] System ready.");

  Serial.println("=====================================");
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  Serial.println("[STATUS] LED ON  — System Alive");
  delay(500);

  digitalWrite(LED_PIN, LOW);
  Serial.println("[STATUS] LED OFF — Monitoring...");
  delay(500);
}