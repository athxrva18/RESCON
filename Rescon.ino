#include <DHT.h>
#include <Wire.h>
#include <WiFi.h>
#include <TimeLib.h>

#define DHTPIN 14
#define DHTTYPE DHT11

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// WiFi credentials
const char *ssid = "HiFi";
const char *password = "pallu1600";

// Define the task parameters
#define TASK_DELAY_MS 2000  // 2 sec delay
TaskHandle_t dhtTaskHandle;

// Queue parameters
#define QUEUE_SIZE 15
QueueHandle_t dataQueue;

// Structure to store DHT11 data with timestamp
struct SensorData {
  time_t timestamp;
  float temperature;
  float humidity;
};

// Set the initial timestamp to (11, 23, 34, 02, 02, 2024)


// Task function to read DHT11 sensor
void dhtTask(void *pvParameters) {
  (void)pvParameters;  // Unused parameter

  for (;;) {
    // Check if WiFi is connected
    if (WiFi.status() == WL_CONNECTED) {
      float temperature, humidity;
      temperature = dht.readTemperature();
      humidity = dht.readHumidity();

      if (!isnan(temperature) && !isnan(humidity)) {
        // Get current timestamp
        time_t currentTimestamp = now();

        // Print live sensor readings with timestamp
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
      // WiFi disconnected, store values in queue
      Serial.println("WiFi disconnected! Data storing to QUEUE.");
        
      float temperature, humidity;
      temperature = dht.readTemperature();
      humidity = dht.readHumidity();

      struct SensorData sensorReading;
      sensorReading.timestamp = now();  // Store timestamp
      sensorReading.temperature = temperature;
      sensorReading.humidity = humidity;

      // Send data to the queue
      xQueueSend(dataQueue, &sensorReading, portMAX_DELAY);
      vTaskDelay(pdMS_TO_TICKS(TASK_DELAY_MS));
    }

    vTaskDelay(pdMS_TO_TICKS(TASK_DELAY_MS));
  }
}

// Task function to process WiFi reconnection
void wifireconnected(void *pvParameters) {
  for (;;) {
    // Check if WiFi is connected
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

  // Set the initial timestamp
  setTime(11,03,4,02,02,2024);

  // Initialize the DHT sensor
  dht.begin();

  // Initialize WiFi
  WiFi.begin(ssid, password);

  // Task to read DHT11 sensor
  xTaskCreate(dhtTask, "DHTTask", 10000, NULL, 1, &dhtTaskHandle);

  // Task to process WiFi reconnection
  xTaskCreate(wifireconnected, "wifireconnected", 10000, NULL, 2, NULL);

  // Create a queue to store sensor data
  dataQueue = xQueueCreate(QUEUE_SIZE, sizeof(struct SensorData));
}

void loop() {
  // Not needed for FreeRTOS
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
