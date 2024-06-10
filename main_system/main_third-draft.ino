#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "DHT.h"
#include <LiquidCrystal_I2C.h>

const char *ssid = ""; // WiFi SSID
const char *password = ""; // WiFi password
const char *endpoint = ""; // API endpoint

// Define pins for DHT11 sensor
#define DHTTYPE DHT11
#define DHTPIN 12

// Define pins for LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);  

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
#define MOISTURE_THRESHOLD_LOW 1500

// Define reed switch pin
const int FS_RWT = 2; // Pin connected to reed switch
const int FS_NWC = 36; // Pin connected to NWC float switch

DHT dht(DHTPIN, DHTTYPE);

// Variables to track watering state
bool is_raining = false;
bool pump_active = false;
unsigned long rain_detected_time = 0;
unsigned long watering_start_time = 0;

void setup() {
  // Initialize serial communication
  Serial.begin(9600);

  // Initialize DHT sensor
  dht.begin();

  // Initialize LCD
  lcd.init();
  lcd.backlight();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

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
  // Check WiFi connection status
  if(WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("");

    HTTPClient http;

    // Establish a connection to the server
    String url = String(endpoint) + "/sensor-data";
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    DynamicJsonDocument doc(1024);
    String httpRequestData;

    // Read sensor values
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    int moisture1 = analogRead(MOISTURE_SENSOR_1_PIN);
    int moisture2 = analogRead(MOISTURE_SENSOR_2_PIN);
    int moisture3 = analogRead(MOISTURE_SENSOR_3_PIN);
    int rain_water_level = digitalRead(FS_RWT);
    int nwc_water_level = digitalRead(FS_NWC);

    // Serialize JSON object into a string to be sent to the API
    doc["temperature"] = temperature;
    doc["humidity"] = humidity;
    doc["rain_water_level"] = rain_water_level;
    doc["nwc_water_level"] = nwc_water_level;
    doc["zone1_soil_moisture"] = moisture1;
    doc["zone2_soil_moisture"] = moisture2;
    doc["zone3_soil_moisture"] = moisture3;
    serializeJson(doc, httpRequestData);

    // Send HTTP POST request
    int httpResponseCode = http.POST(httpRequestData);
    String http_response;

    // Check result of POST request. Negative response code means server wasn't reached
    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);

      Serial.print("HTTP Response from server: ");
      http_response = http.getString();
      Serial.println(http_response);

      // Parse server response
      DynamicJsonDocument responseDoc(1024);
      deserializeJson(responseDoc, http_response);
      bool start_pump = responseDoc["start_pump"];
      bool stop_pump = responseDoc["stop_pump"];

      // Handle start/stop pump command
      if (start_pump) {
        activatePump();
      }
      if (stop_pump) {
        deactivatePump();
      }
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }

    http.end();

    // Display moisture data on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Moisture 1: ");
    lcd.print(moisture1 > MOISTURE_THRESHOLD_HIGH ? "Moist" : "Dry");
    lcd.setCursor(0, 1);
    lcd.print("Moisture 2: ");
    lcd.print(moisture2 > MOISTURE_THRESHOLD_HIGH ? "Moist" : "Dry");
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Moisture 3: ");
    lcd.print(moisture3 > MOISTURE_THRESHOLD_HIGH ? "Moist" : "Dry");
    lcd.setCursor(0, 1);
    lcd.print("Rain Tank: ");
    lcd.print(rain_water_level == LOW ? "Low" : "High");
    delay(2000);

    // Check for rain detection and handle delay
    if (rain_water_level == HIGH) {
      is_raining = true;
      rain_detected_time = millis();
      deactivatePump();
    }

    if (is_raining && millis() - rain_detected_time >= 3600000) { // 1 hour delay
      is_raining = false;
    }

    // Handle automatic watering based on soil moisture and user schedule
    if (!is_raining) {
      if (moisture1 < MOISTURE_THRESHOLD_LOW || moisture2 < MOISTURE_THRESHOLD_LOW || moisture3 < MOISTURE_THRESHOLD_LOW) {
        activatePump();
      } else if (moisture1 > MOISTURE_THRESHOLD_HIGH && moisture2 > MOISTURE_THRESHOLD_HIGH && moisture3 > MOISTURE_THRESHOLD_HIGH) {
        deactivatePump();
      }
    }
  } else {
    Serial.println("WiFi Disconnected");
  }

  // Wait for a short duration before reading sensors again
  delay(10000);
}

void activatePump() {
  digitalWrite(CENTRAL_PUMP_VALVE_PIN, HIGH);
  digitalWrite(ZONE_ONE_VALVE_PIN, HIGH);
  digitalWrite(ZONE_TWO_VALVE_PIN, HIGH);
  digitalWrite(ZONE_THREE_VALVE_PIN, HIGH);
  pump_active = true;
}

void deactivatePump() {
  digitalWrite(CENTRAL_PUMP_VALVE_PIN, LOW);
  digitalWrite(ZONE_ONE_VALVE_PIN, LOW);
  digitalWrite(ZONE_TWO_VALVE_PIN, LOW);
  digitalWrite(ZONE_THREE_VALVE_PIN, LOW);
  pump_active = false;
}
