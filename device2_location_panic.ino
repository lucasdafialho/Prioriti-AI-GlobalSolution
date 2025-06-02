// Global Solution 2025 - IoT Device 2: Location Tracker + Panic Button
// Platform: ESP32 (Simulated in Wokwi)
// Includes MQTT Publishing Logic

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <TinyGPS++.h> // Requires installing TinyGPS++ library in Wokwi

// --- Pin Definitions ---
#define PANIC_BUTTON_PIN 15 // Choose an appropriate GPIO pin
#define GPS_RX_PIN 16       // ESP32 RX connected to GPS TX
#define GPS_TX_PIN 17       // ESP32 TX connected to GPS RX

// --- GPS Objects ---
TinyGPSPlus gps;
HardwareSerial gpsSerial(1); // Use UART1 for GPS

// --- WiFi Credentials --- 
const char* ssid = "Wokwi-GUEST"; // Use Wokwi's default or your local SSID
const char* password = ""; // Use empty for Wokwi's default or your local password

// --- MQTT Configuration ---
const char* mqtt_server = "broker.hivemq.com"; // Using public broker
const int mqtt_port = 1883;
const char* mqtt_topic = "fiap/gs/locationtracker01/data";
const char* mqtt_client_id = "LocationTracker01_ESP32Client"; // Unique client ID

// --- MQTT Client ---
WiFiClient espClient;
PubSubClient client(espClient);

// --- Timing and State Control ---
unsigned long lastLocationSendTime = 0;
const long locationSendInterval = 60000; // Send location every 60 seconds
volatile bool panicButtonPressed = false;
unsigned long lastDebounceTime = 0;
const long debounceDelay = 50;

// --- Function Declarations ---
void setup_wifi();
void reconnect_mqtt();
void publish_mqtt_data(JsonDocument& doc);
void IRAM_ATTR handlePanicButtonInterrupt();

void setup() {
  Serial.begin(115200);
  Serial.println("\nDevice 2: Location Tracker + Panic Button Booting...");

  // Initialize GPS Serial
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("GPS Serial Initialized.");

  // Initialize Panic Button Pin with Interrupt
  pinMode(PANIC_BUTTON_PIN, INPUT_PULLUP); // Use internal pull-up resistor
  attachInterrupt(digitalPinToInterrupt(PANIC_BUTTON_PIN), handlePanicButtonInterrupt, FALLING); // Trigger on button press (LOW signal)
  Serial.println("Panic Button Initialized.");

  // Connect to WiFi
  setup_wifi();

  // Configure MQTT
  client.setServer(mqtt_server, mqtt_port);

  Serial.println("Setup complete. Starting loop...");
}

void loop() {
  // Process GPS data
  while (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read())) {
      // GPS sentence successfully encoded
    }
  }

  // Ensure MQTT connection is active
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();

  unsigned long currentTime = millis();

  // Handle Panic Button Press (debounced)
  if (panicButtonPressed) {
    Serial.println("Panic button pressed! Sending alert...");
    StaticJsonDocument<256> panicDoc;
    panicDoc["deviceId"] = mqtt_client_id;
    panicDoc["timestamp"] = currentTime;
    panicDoc["alertType"] = "PanicButton";
    if (gps.location.isValid()) {
        panicDoc["latitude"] = gps.location.lat();
        panicDoc["longitude"] = gps.location.lng();
    } else {
        panicDoc["latitude"] = nullptr; // Indicate no valid location
        panicDoc["longitude"] = nullptr;
    }
    publish_mqtt_data(panicDoc); // Publish immediately
    panicButtonPressed = false; // Reset flag after sending
    delay(1000); // Prevent rapid re-triggering
  }

  // Check if it's time to send regular location data
  if (currentTime - lastLocationSendTime >= locationSendInterval) {
    lastLocationSendTime = currentTime;

    if (gps.location.isValid()) {
      StaticJsonDocument<256> locationDoc;
      locationDoc["deviceId"] = mqtt_client_id;
      locationDoc["timestamp"] = currentTime;
      locationDoc["alertType"] = "LocationUpdate";
      locationDoc["latitude"] = gps.location.lat();
      locationDoc["longitude"] = gps.location.lng();
      locationDoc["altitude_m"] = gps.altitude.isValid() ? gps.altitude.meters() : (double)NAN;
      locationDoc["speed_kmph"] = gps.speed.isValid() ? gps.speed.kmph() : (double)NAN;
      locationDoc["satellites"] = gps.satellites.isValid() ? gps.satellites.value() : (int)NAN;
      
      publish_mqtt_data(locationDoc);
    } else {
      Serial.println("No valid GPS location data to send.");
    }
  }

  delay(100);
}

// Interrupt Service Routine for Panic Button
void IRAM_ATTR handlePanicButtonInterrupt() {
  // Basic debounce
  if ((millis() - lastDebounceTime) > debounceDelay) {
    panicButtonPressed = true;
    lastDebounceTime = millis();
  }
}

// --- WiFi and MQTT Helper Functions (Same as Device 1) ---

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
    if (++retries > 20) { 
        Serial.println("\nWiFi connection failed.");
        return;
    }
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqtt_client_id)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void publish_mqtt_data(JsonDocument& doc) {
  if (!client.connected()) {
    Serial.println("MQTT client not connected. Cannot publish.");
    return;
  }

  char buffer[256]; // Adjust size if needed
  size_t n = serializeJson(doc, buffer);

  Serial.print("Publishing message to topic: ");
  Serial.println(mqtt_topic);
  // Serial.println(buffer);

  if (client.publish(mqtt_topic, buffer, n)) {
    Serial.println("Message published successfully");
  } else {
    Serial.println("Failed to publish message");
  }
}

