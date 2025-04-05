#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#define DHTPIN 27
#define DHTTYPE DHT22 
#include <tflm_esp32.h>
#include <eloquent_tinyml.h>
#include "weather_model_float.h" 

// StandardScaler values from Python
const float TEMP_MEAN = 14.81430993f;
const float TEMP_STD = 11.77413137f;
const float HUMIDITY_MEAN = 0.6643972f;
const float HUMIDITY_STD = 0.22815949f;
DHT dht(DHTPIN, DHTTYPE);
// Model configuration
#define N_INPUTS 2
#define N_OUTPUTS 1
#define ARENA_SIZE 10000  // Large buffer for float model
#define TF_NUM_OPS 9      // For 3 dense + 2 ReLU + 1 sigmoid

Eloquent::TF::Sequential<TF_NUM_OPS, ARENA_SIZE> tf;

void setup() {
    Serial.begin(115200);
    delay(3000);  // Wait for serial monitor
    
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
    float temperature = dht.readTemperature();  // Example input
    float humidity = dht.readHumidity();      // Example input
    
    // Normalize exactly like our Notebook
    float input[2] = {
        (temperature - TEMP_MEAN) / TEMP_STD,
        (humidity - HUMIDITY_MEAN) / HUMIDITY_STD
    };
    
    float prediction = tf.predict(input);
    
    Serial.print("\nPrediction for (");
    Serial.print(temperature);
    Serial.print("°C, ");
    Serial.print(humidity);
    Serial.print("): ");
    Serial.println(prediction < 0.5f ? "Sunny" : "Cold");
    
    delay(2000);
}
