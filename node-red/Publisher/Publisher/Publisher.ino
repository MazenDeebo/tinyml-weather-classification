#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "DHT.h"

// Wi-Fi credentials
const char* ssid = "Network goes brr";
const char* password = "Ah01020038841";

// MQTT broker details
const char* mqtt_broker = "2e422052cd1643ee9205278aaa41846b.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "Temper";
const char* mqtt_password = "Ah1234567";

// MQTT topic
const char* topic_temp = "esp32/temperature";
const char* topic_hum = "esp32/humidity";

// DHT Sensor settings
#define DHTPIN 27
#define DHTTYPE DHT22  // DHT 22

DHT dht(DHTPIN, DHTTYPE);

// Create instances
WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);

// Function to connect to Wi-Fi
void connectWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to Wi-Fi");
}

// Function to connect to MQTT Broker
void connectMQTT() {
  mqttClient.setServer(mqtt_broker, mqtt_port);
  while (!mqttClient.connected()) {
    Serial.println("Connecting to MQTT Broker...");
    if (mqttClient.connect("ESP32Client", mqtt_username, mqtt_password)) {
      Serial.println("Connected to MQTT Broker");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" trying again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  connectWiFi();
  wifiClient.setInsecure(); // Use with caution; consider adding a root certificate for production
  connectMQTT();
  dht.begin();
}

void loop() {
  if (!mqttClient.connected()) {
    connectMQTT();
  }
  mqttClient.loop();

  // Read temperature and humidity
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Check if any reads failed
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  char tempString[8];
  dtostrf(temperature, 1, 2, tempString);
  char humString[8];
  dtostrf(humidity, 1, 2, humString);

  mqttClient.publish(topic_temp, tempString);
  mqttClient.publish(topic_hum, humString);

  Serial.print("Temperature: ");
  Serial.print(tempString);
  Serial.println(" °C");
  Serial.print("Humidity: ");
  Serial.print(humString);
  Serial.println(" %");
  delay(1000); // Publish every second
}
