#include <tflm_esp32.h>
#include <eloquent_tinyml.h>
#include "weather_model_float.h"

#define N_INPUTS 2
#define N_OUTPUTS 1
#define ARENA_SIZE 2832
#define TF_NUM_OPS 3

Eloquent::TF::Sequential<TF_NUM_OPS, ARENA_SIZE> tf;

void setup() {
    Serial.begin(115200);
    tf.setNumInputs(2);
    tf.setNumOutputs(1);
    tf.resolver.AddFullyConnected();
//    tf.resolver.AddSoftmax();
    tf.resolver.AddLogistic();  
    while (!tf.begin(model_float).isOk()){ 
        Serial.println(tf.exception.toString());
    }
    Serial.println("Model Loaded successfully !");
}

void loop() {
    float temperature = 24.0;  // Replace with sensor reading
    float humidity = 0.5;

    float input[2] = {temperature/50 , humidity/100};
    float prediction = tf.predict(input);

    String weather = prediction > 0.5 ? "Sunny" : "Cold";
//    Serial.print("Prediction: ");
//    Serial.println(weather);
    Serial.println(prediction);
    
    
    delay(1000);
}
