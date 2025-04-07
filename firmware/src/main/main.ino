#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#define DHTPIN 27
#define DHTTYPE DHT22 
#include <tflm_esp32.h>
#include <eloquent_tinyml.h>
#include "weather_model_float.h" 
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>


// Wi-Fi credentials
const char* ssid = "Network goes brr";
const char* password = "Ah01020038841";

// StandardScaler values from Python
const float TEMP_MEAN = 14.81430993f;
const float TEMP_STD = 11.77413137f;
const float HUMIDITY_MEAN = 0.6643972f;
const float HUMIDITY_STD = 0.22815949f;
DHT dht(DHTPIN, DHTTYPE);

// MQTT broker details
const char* mqtt_broker = "2e422052cd1643ee9205278aaa41846b.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_username = "Temper";
const char* mqtt_password = "Ah1234567";

// MQTT topic
const char* topic_temp = "esp32/temperature";
const char* topic_hum = "esp32/humidity";
const char* topic_pre = "esp32/prediction";

// Create instances
WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);

// Model configuration
#define N_INPUTS 2
#define N_OUTPUTS 1
#define ARENA_SIZE 10000  // Large buffer for float model
#define TF_NUM_OPS 9      // For 3 dense + 2 ReLU + 1 sigmoid

Eloquent::TF::Sequential<TF_NUM_OPS, ARENA_SIZE> tf;

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
    delay(3000);  // Wait for serial monitor
    connectWiFi();
    wifiClient.setInsecure();
    connectMQTT();
    Serial.println("\n=== Weather Model Initialization ===");
    // 1. Configure model architecture to match Python exactly
    tf.setNumInputs(N_INPUTS);
    tf.setNumOutputs(N_OUTPUTS);
    
    // Must match: Dense(8)->ReLU->Dense(4)->ReLU->Dense(1)->Sigmoid
    tf.resolver.AddFullyConnected();  // Layer 1 (8 neurons)
    tf.resolver.AddRelu();
    tf.resolver.AddFullyConnected();  // Layer 2 (4 neurons)
    tf.resolver.AddRelu();
    tf.resolver.AddFullyConnected();  // Output layer
    tf.resolver.AddLogistic();        // Sigmoid
    
    // 2. Load model with detailed error reporting
    Serial.println("Loading model...");
    auto status = tf.begin(model_float);
    
    if (!status.isOk()) {
        Serial.println("\n--- MODEL LOAD FAILED ---");
        Serial.println("Solutions to try:");
        Serial.println("1. Increase ARENA_SIZE further");
        Serial.println("2. Verify model architecture matches");
        Serial.println("3. Check model conversion process");
        while(1);
    }
    
    Serial.println("Model loaded successfully!");
    dht.begin();
}

void loop() {
   if (!mqttClient.connected()) {
    connectMQTT();
    }
    mqttClient.loop();
    float temperature = dht.readTemperature(); 
    float humidity = dht.readHumidity();      
    
    // Normalize exactly like our Notebook
    float input[2] = {
        (temperature - TEMP_MEAN) / TEMP_STD,
        (humidity - HUMIDITY_MEAN) / HUMIDITY_STD
    };
    
    float prediction = tf.predict(input);
    char tempString[8];
    dtostrf(temperature, 1, 2, tempString);
    char humString[8];
    dtostrf(humidity, 1, 2, humString);
    char result_prediction[10];
    if (prediction < 0.5f) {
       strcpy(result_prediction, "Sunny");
    } else {
      strcpy(result_prediction, "Cold");
    }
    Serial.print("\nPrediction for (");
    Serial.print(temperature);
    Serial.print("°C, ");
    Serial.print(humidity);
    Serial.print("): ");
    Serial.println(result_prediction);
    mqttClient.publish(topic_temp, tempString);
    mqttClient.publish(topic_hum, humString);
    mqttClient.publish(topic_pre, result_prediction);
    delay(2000);
}
