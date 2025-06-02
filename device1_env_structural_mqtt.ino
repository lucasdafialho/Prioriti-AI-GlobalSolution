// Global Solution 2025 - IoT Device 1: Environmental & Structural Node
// Platform: ESP32 (Simulated in Wokwi)
// Includes MQTT Publishing Logic

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "DHT.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

// --- Pin Definitions ---
#define DHTPIN 4       // Pin for DHT22 data
#define DHTTYPE DHT22    // DHT sensor type
#define TRIGPIN 12     // HC-SR04 Trigger Pin
#define ECHOPIN 13     // HC-SR04 Echo Pin

// --- Sensor Objects ---
DHT dht(DHTPIN, DHTTYPE);
Adafruit_MPU6050 mpu;

// --- WiFi Credentials --- 
const char* ssid = "Wokwi-GUEST"; // Use Wokwi's default or your local SSID
const char* password = ""; // Use empty for Wokwi's default or your local password

// --- MQTT Configuration ---
const char* mqtt_server = "broker.hivemq.com"; // Using public broker
const int mqtt_port = 1883;
const char* mqtt_topic = "fiap/gs/envnode01/data";
const char* mqtt_client_id = "EnvNode01_ESP32Client"; // Unique client ID

// --- MQTT Client ---
WiFiClient espClient;
PubSubClient client(espClient);

// --- Timing Control ---
unsigned long lastSendTime = 0;
const long sendInterval = 30000; // Send data every 30 seconds for faster demo

// --- Function Declarations ---
void setup_wifi();
void reconnect_mqtt();
void publish_mqtt_data(JsonDocument& doc);

void setup() {
  Serial.begin(115200);
  Serial.println("\nDevice 1: Environmental & Structural Node Booting...");

  // Initialize DHT sensor
  dht.begin();
  Serial.println("DHT22 Initialized.");

  // Initialize HC-SR04 pins
  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);
  Serial.println("HC-SR04 Pins Initialized.");

  // Initialize MPU6050
  Wire.begin();
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) { delay(10); }
  }
  Serial.println("MPU6050 Found!");
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // Connect to WiFi
  setup_wifi();

  // Configure MQTT
  client.setServer(mqtt_server, mqtt_port);

  Serial.println("Setup complete. Starting loop...");
}

void loop() {
  // Ensure MQTT connection is active
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop(); // Allow the MQTT client to process incoming messages and maintain connection

  unsigned long currentTime = millis();

  // Check if it's time to send data
  if (currentTime - lastSendTime >= sendInterval) {
    lastSendTime = currentTime;

    // --- Read Sensors ---
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    long duration, distance_cm;
    sensors_event_t a, g, temp;

    // Read HC-SR04
    digitalWrite(TRIGPIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIGPIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIGPIN, LOW);
    duration = pulseIn(ECHOPIN, HIGH, 25000); // Added timeout
    if (duration == 0) { // Timeout or error
        distance_cm = -1; // Indicate error
        Serial.println("HC-SR04 Timeout/Error");
    } else {
        distance_cm = duration * 0.034 / 2;
    }

    // Read MPU6050
    mpu.getEvent(&a, &g, &temp);

    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Failed to read from DHT sensor!");
      return; 
    }

    // --- Prepare JSON Payload ---
    StaticJsonDocument<512> jsonDoc;
    jsonDoc["deviceId"] = mqtt_client_id;
    jsonDoc["timestamp"] = currentTime; 
    
    JsonObject data = jsonDoc.createNestedObject("data");
    data["temperature_c"] = round(temperature * 10.0) / 10.0; // Round to 1 decimal
    data["humidity_percent"] = round(humidity * 10.0) / 10.0;
    data["distance_cm"] = distance_cm;
    
    JsonObject accel = data.createNestedObject("accelerometer");
    accel["x"] = round(a.acceleration.x * 100.0) / 100.0; // Round to 2 decimals
    accel["y"] = round(a.acceleration.y * 100.0) / 100.0;
    accel["z"] = round(a.acceleration.z * 100.0) / 100.0;

    JsonObject gyro = data.createNestedObject("gyroscope");
    gyro["x"] = round(g.gyro.x * 100.0) / 100.0;
    gyro["y"] = round(g.gyro.y * 100.0) / 100.0;
    gyro["z"] = round(g.gyro.z * 100.0) / 100.0;

    // --- Publish Data via MQTT ---
    publish_mqtt_data(jsonDoc);
  }

  delay(100); 
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  int retries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (++retries > 20) { // Timeout after 10 seconds
        Serial.println("\nWiFi connection failed. Please check credentials or network.");
        // Optional: Enter deep sleep or retry later
        return; // Or ESP.restart();
    }
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect_mqtt() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqtt_client_id)) { // Pass client ID here
      Serial.println("connected");
      // Once connected, publish an announcement...
      // client.publish("fiap/gs/envnode01/status", "connected");
      // ... and resubscribe? (Not needed for this publisher-only device)
      // client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void publish_mqtt_data(JsonDocument& doc) {
  if (!client.connected()) {
    Serial.println("MQTT client not connected. Cannot publish.");
    return;
  }

  char buffer[512];
  size_t n = serializeJson(doc, buffer);

  Serial.print("Publishing message to topic: ");
  Serial.println(mqtt_topic);
  // Serial.println(buffer); // Optional: print JSON before sending

  if (client.publish(mqtt_topic, buffer, n)) {
    Serial.println("Message published successfully");
  } else {
    Serial.println("Failed to publish message");
  }
}

