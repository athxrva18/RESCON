#include <DHT.h>
#include <Wire.h>
#include <WiFi.h>
#include <TimeLib.h>

#define DHTPIN 14
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);


const char *ssid = "HiFi";
const char *password = "pallu1600";

#define TASK_DELAY_MS 2000  // 2 sec delay
TaskHandle_t dhtTaskHandle;

#define QUEUE_SIZE 15
QueueHandle_t dataQueue;

struct SensorData {
  time_t timestamp;
  float temperature;
  float humidity;
};


void dhtTask(void *pvParameters) {
  (void)pvParameters;  

  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      float temperature, humidity;
      temperature = dht.readTemperature();
      humidity = dht.readHumidity();

      if (!isnan(temperature) && !isnan(humidity)) {

        time_t currentTimestamp = now();
        Serial.print("Real Time     - ");
        Serial.print("Temperature: ");
        Serial.print(temperature);
        Serial.print("°C, Humidity: ");
        Serial.print(humidity);
        Serial.print("%");
        printTimestamp(currentTimestamp);
      } else {
        Serial.println("Failed to read DHT11 sensor data");
      }
    } else {
  
      Serial.println("WiFi disconnected! Data storing to QUEUE.");
        
      float temperature, humidity;
      temperature = dht.readTemperature();
      humidity = dht.readHumidity();

      struct SensorData sensorReading;
      sensorReading.timestamp = now(); 
      sensorReading.temperature = temperature;
      sensorReading.humidity = humidity;

  
      xQueueSend(dataQueue, &sensorReading, portMAX_DELAY);
      vTaskDelay(pdMS_TO_TICKS(TASK_DELAY_MS));
    }

    vTaskDelay(pdMS_TO_TICKS(TASK_DELAY_MS));
  }
}

void wifireconnected(void *pvParameters) {
  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      if (uxQueueMessagesWaiting(dataQueue) > 0) {
        struct SensorData storedReading;
        xQueueReceive(dataQueue, &storedReading, 0);
        Serial.print("Stored data   - ");
        Serial.print("Temperature: ");
        Serial.print(storedReading.temperature);
        Serial.print("°C, Humidity: ");
        Serial.print(storedReading.humidity);
        Serial.print("%");
        printTimestamp(storedReading.timestamp);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(TASK_DELAY_MS));
  }
}

void setup() {
  Serial.begin(115200);
  setTime(11,03,4,02,02,2024);

  dht.begin();

  WiFi.begin(ssid, password);

  xTaskCreate(dhtTask, "DHTTask", 10000, NULL, 1, &dhtTaskHandle);


  xTaskCreate(wifireconnected, "wifireconnected", 10000, NULL, 2, NULL);


  dataQueue = xQueueCreate(QUEUE_SIZE, sizeof(struct SensorData));
}

void loop() {
}

void printTimestamp(time_t timestamp) {
  Serial.print("     ");
  Serial.print("TimeStamp: ");
  Serial.print(year(timestamp));
  Serial.print("-");
  Serial.print(month(timestamp));
  Serial.print("-");
  Serial.print(day(timestamp));
  Serial.print(" ");
  Serial.print(hour(timestamp));
  Serial.print(":");
  Serial.print(minute(timestamp));
  Serial.print(":");
  Serial.print(second(timestamp));
  Serial.println("  ");
}
