#include <Wire.h>
#include <MPU6050.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

MPU6050 mpu;

// ================== WiFi + MQTT ==================
const char* ssid     = "Wokwi-GUEST";   // WiFi ของ Wokwi
const char* password = "";

const char* mqtt_host = "mqtt.thingsboard.cloud";
const int   mqtt_port = 1883;
const char* mqtt_user = "p65ghf56lzq2oyumt2o3";  // ← ใส่ Token ที่ได้จาก Thingsboard
const char* mqtt_topic = "v1/devices/me/telemetry";

// ================== PIN + PARAMETER ==================
#define RELAY_PIN 26
float threshold = 1.3;
float alpha = 0.7;

float ax, ay, az, vibration = 0, smoothVibration = 0;
bool machineRunning = true;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ================== FUNCTIONS ==================
float calculateVibration(float x, float y, float z) {
  return sqrt(x*x + y*y + z*z);
}
float smooth(float input, float prev) {
  return (alpha * input) + ((1 - alpha) * prev);
}

void connectWiFi() {
  Serial.print("Connecting WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println(" Connected!");
}

void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting MQTT...");
    if (mqttClient.connect("esp32-vibration", mqtt_user, "")) {
      Serial.println(" Connected!");
    } else {
      Serial.print("Failed, rc="); Serial.println(mqttClient.state());
      delay(3000);
    }
  }
}

void publishData(float vib, float smooth_vib, bool running) {
  StaticJsonDocument<200> doc;
  doc["vibration"]       = vib;
  doc["smoothVibration"] = smooth_vib;
  doc["ax"]              = ax;
  doc["ay"]              = ay;
  doc["az"]              = az;
  doc["status"]          = running ? "RUNNING" : "STOP";
  
  char payload[200];
  serializeJson(doc, payload);
  mqttClient.publish(mqtt_topic, payload);
}

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  mpu.initialize();

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  connectWiFi();
  mqttClient.setServer(mqtt_host, mqtt_port);
  connectMQTT();

  Serial.println("=== SYSTEM START ===");
}

// ================== LOOP ==================
void loop() {
  if (!mqttClient.connected()) connectMQTT();
  mqttClient.loop();

  int16_t ax_raw, ay_raw, az_raw;
  mpu.getAcceleration(&ax_raw, &ay_raw, &az_raw);

  ax = ax_raw / 16384.0;
  ay = ay_raw / 16384.0;
  az = az_raw / 16384.0;

  vibration = calculateVibration(ax, ay, az);
  smoothVibration = smooth(vibration, smoothVibration);

  if (smoothVibration > threshold) {
    if (machineRunning) {
      Serial.println("⚠️ ABNORMAL → STOP");
      digitalWrite(RELAY_PIN, LOW);
      machineRunning = false;
    }
  } else {
    if (!machineRunning) {
      Serial.println("✅ NORMAL → RUNNING");
      digitalWrite(RELAY_PIN, HIGH);
      machineRunning = true;
    }
  }

  publishData(vibration, smoothVibration, machineRunning);

  Serial.print("Vib: "); Serial.print(smoothVibration);
  Serial.print(" | Status: "); Serial.println(machineRunning ? "RUNNING" : "STOP");

  delay(500);  // ส่งทุก 0.5 วินาที
}