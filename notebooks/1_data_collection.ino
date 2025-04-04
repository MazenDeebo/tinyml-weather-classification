#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN 27
#define DHTTYPE DHT22 


DHT dht(DHTPIN, DHTTYPE);

void setup() {
    Serial.begin(115200);
    dht.begin();
    Serial.println("Temperature,Humidity");  // CSV header
}

void loop() {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (isnan(temperature) || isnan(humidity)) {
        Serial.println("Error,Error");
        return;
    }

    Serial.print(temperature);
    Serial.print(",");
    Serial.println(humidity);

    delay(2000);  // Log data every 2 seconds
}
