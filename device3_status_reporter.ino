// Global Solution 2025 - IoT Device 3: Simplified Status Reporter
// Platform: ESP32 (Simulated in Wokwi)
// Includes MQTT Publishing Logic

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// --- Pin Definitions ---
#define BUTTON_PIN_OK 15      // Example: Status OK
#define BUTTON_PIN_ASSIST 2   // Example: Assistance Needed
#define BUTTON_PIN_DANGER 4   // Example: Immediate Danger
// Add more buttons as needed

// --- WiFi Credentials --- 
const char* ssid = "Wokwi-GUEST"; // Use Wokwi's default or your local SSID
const char* password = ""; // Use empty for Wokwi's default or your local password

// --- MQTT Configuration ---
const char* mqtt_server = "broker.hivemq.com"; // Using public broker
const int mqtt_port = 1883;
const char* mqtt_topic = "fiap/gs/statusreporter01/data";
const char* mqtt_client_id = "StatusReporter01_ESP32Client"; // Unique client ID

// --- MQTT Client ---
WiFiClient espClient;
PubSubClient client(espClient);

// --- State Control ---
volatile int buttonPressed = 0; // 0: None, 1: OK, 2: Assist, 3: Danger
unsigned long lastDebounceTime = 0;
const long debounceDelay = 50;

// --- Function Declarations ---
void setup_wifi();
void reconnect_mqtt();
void publish_mqtt_data(JsonDocument& doc);
void IRAM_ATTR handleButtonInterrupt_OK();
void IRAM_ATTR handleButtonInterrupt_Assist();
void IRAM_ATTR handleButtonInterrupt_Danger();

void setup() {
  Serial.begin(115200);
  Serial.println("\nDevice 3: Status Reporter Booting...");

  // Initialize Button Pins with Interrupts
  pinMode(BUTTON_PIN_OK, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN_OK), handleButtonInterrupt_OK, FALLING);
  
  pinMode(BUTTON_PIN_ASSIST, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN_ASSIST), handleButtonInterrupt_Assist, FALLING);

  pinMode(BUTTON_PIN_DANGER, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN_DANGER), handleButtonInterrupt_Danger, FALLING);
  
  Serial.println("Status Buttons Initialized.");

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
  client.loop();

  unsigned long currentTime = millis();

  // Handle Button Press (debounced)
  if (buttonPressed != 0) {
    String statusMessage = "Unknown";
    if (buttonPressed == 1) statusMessage = "Status OK";
    if (buttonPressed == 2) statusMessage = "Assistance Needed";
    if (buttonPressed == 3) statusMessage = "Immediate Danger";
    
    Serial.print("Button pressed: ");
    Serial.println(statusMessage);

    StaticJsonDocument<256> statusDoc;
    statusDoc["deviceId"] = mqtt_client_id;
    statusDoc["timestamp"] = currentTime;
    statusDoc["statusReport"] = statusMessage;
    statusDoc["statusCode"] = buttonPressed; // Send code as well
    
    publish_mqtt_data(statusDoc); // Publish immediately
    buttonPressed = 0; // Reset flag after sending
    delay(1000); // Prevent rapid re-triggering
  }

  // This device primarily reacts to button presses, so main loop is simple
  delay(100);
}

// --- Interrupt Service Routines ---
void IRAM_ATTR handleButtonInterrupt_OK() {
  if ((millis() - lastDebounceTime) > debounceDelay) {
    buttonPressed = 1;
    lastDebounceTime = millis();
  }
}
void IRAM_ATTR handleButtonInterrupt_Assist() {
  if ((millis() - lastDebounceTime) > debounceDelay) {
    buttonPressed = 2;
    lastDebounceTime = millis();
  }
}
void IRAM_ATTR handleButtonInterrupt_Danger() {
  if ((millis() - lastDebounceTime) > debounceDelay) {
    buttonPressed = 3;
    lastDebounceTime = millis();
  }
}

// --- WiFi and MQTT Helper Functions (Same as Device 1 & 2) ---

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

