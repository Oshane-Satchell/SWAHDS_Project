#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Define pins for soil moisture sensors
#define MOISTURE_SENSOR_1_PIN 13
#define MOISTURE_SENSOR_2_PIN 4
#define MOISTURE_SENSOR_3_PIN 15

// Define pins for solenoid valves
#define RAIN_WATER_TANK_VALVE_PIN 32
#define NWC_TANK_INPUT_VALVE_PIN 33
#define NWC_TANK_OUTPUT_VALVE_PIN 25
#define CENTRAL_PUMP_VALVE_PIN 23
#define ZONE_ONE_VALVE_PIN 26
#define ZONE_TWO_VALVE_PIN 19
#define ZONE_THREE_VALVE_PIN 14

// Define moisture threshold values
#define MOISTURE_THRESHOLD_HIGH 2500
#define MOISTURE_THRESHOLD1_HIGH 4050
#define MOISTURE_THRESHOLD_LOW 1500

// Define reed switch pin
const int FS_RWT = 2; // Pin connected to reed switch
const int FS_NWC = 36; // Pin connected to NWC float switch

// Initialize the LCD
LiquidCrystal_I2C lcd(0x27, 16, 2); // Set the LCD address to 0x27 for a 16 chars and 2 line display

void setup() {
  // Initialize serial communication
  Serial.begin(9600);

  // Initialize the LCD
  lcd.begin(16, 2);
  lcd.backlight();
  lcd.clear();

  // Set pin modes for moisture sensors
  pinMode(MOISTURE_SENSOR_1_PIN, INPUT);
  pinMode(MOISTURE_SENSOR_2_PIN, INPUT);
  pinMode(MOISTURE_SENSOR_3_PIN, INPUT);

  // Set pin modes for solenoid valves
  pinMode(RAIN_WATER_TANK_VALVE_PIN, OUTPUT);
  pinMode(NWC_TANK_INPUT_VALVE_PIN, OUTPUT);
  pinMode(NWC_TANK_OUTPUT_VALVE_PIN, OUTPUT);
  pinMode(CENTRAL_PUMP_VALVE_PIN, OUTPUT);
  pinMode(ZONE_ONE_VALVE_PIN, OUTPUT);
  pinMode(ZONE_TWO_VALVE_PIN, OUTPUT);
  pinMode(ZONE_THREE_VALVE_PIN, OUTPUT);

  // Set pin mode for reed switch with internal pull-down resistor
  pinMode(FS_RWT, INPUT_PULLDOWN);
  pinMode(FS_NWC, INPUT_PULLDOWN);
}

void loop() {
  // Read moisture sensor values
  int moisture1 = analogRead(MOISTURE_SENSOR_1_PIN);
  int moisture2 = analogRead(MOISTURE_SENSOR_2_PIN);
  int moisture3 = analogRead(MOISTURE_SENSOR_3_PIN);

  // Read state of reed switch
  int proximity = digitalRead(FS_RWT);
  int level = digitalRead(FS_NWC);

  // Check tank water level
  if (proximity == LOW) {
    Serial.println("TANK EMPTY");
    digitalWrite(RAIN_WATER_TANK_VALVE_PIN, LOW); // Deactivate rain water tank valve
    digitalWrite(NWC_TANK_OUTPUT_VALVE_PIN, HIGH); // Activate NWC tank output valve
    lcd.setCursor(0, 1);
    lcd.print("Rain Tank: LOW ");
  } else {
    Serial.println("TANK FULL");
    digitalWrite(RAIN_WATER_TANK_VALVE_PIN, HIGH); // Activate rain water tank valve
    digitalWrite(NWC_TANK_OUTPUT_VALVE_PIN, LOW); // Deactivate NWC tank output valve
    lcd.setCursor(0, 1);
    lcd.print("Rain Tank: HIGH");
  }

  // Check float switch state for NWC tank input valve control
  if (level == LOW) {
    digitalWrite(NWC_TANK_INPUT_VALVE_PIN, HIGH); // Open NWC tank input valve
  } else {
    digitalWrite(NWC_TANK_INPUT_VALVE_PIN, LOW); // Close NWC tank input valve
  }

  // Check moisture levels and control valves accordingly
  lcd.setCursor(0, 0);
  if (moisture1 > MOISTURE_THRESHOLD_HIGH) {
    digitalWrite(ZONE_ONE_VALVE_PIN, HIGH);
    lcd.print("Moisture 1: Dry ");
  } else {
    digitalWrite(ZONE_ONE_VALVE_PIN, LOW);
    lcd.print("Moisture 1: Moist");
  }

  if (moisture2 > MOISTURE_THRESHOLD_HIGH) {
    digitalWrite(ZONE_TWO_VALVE_PIN, HIGH);
    lcd.setCursor(0, 1);
    lcd.print("Moisture 2: Dry ");
  } else {
    digitalWrite(ZONE_TWO_VALVE_PIN, LOW);
    lcd.setCursor(0, 1);
    lcd.print("Moisture 2: Moist");
  }

  if (moisture3 > MOISTURE_THRESHOLD1_HIGH) {
    digitalWrite(ZONE_THREE_VALVE_PIN, HIGH);
    lcd.setCursor(0, 1);
    lcd.print("Moisture 3: Dry ");
  } else {
    digitalWrite(ZONE_THREE_VALVE_PIN, LOW);
    lcd.setCursor(0, 1);
    lcd.print("Moisture 3: Moist");
  }

  // Activate central pump if any zone needs watering
  if (moisture1 > MOISTURE_THRESHOLD_HIGH || moisture2 > MOISTURE_THRESHOLD_HIGH || moisture3 > MOISTURE_THRESHOLD1_HIGH) {
    digitalWrite(CENTRAL_PUMP_VALVE_PIN, HIGH);
  } else {
    digitalWrite(CENTRAL_PUMP_VALVE_PIN, LOW);
    digitalWrite(NWC_TANK_OUTPUT_VALVE_PIN, LOW);
  }

  // Wait for a short duration before reading moisture again
  delay(1000);
}
