#include <WiFi.h>
#include <HTTPClient.h>

// Wi-Fi credentials
const char* ssid = "Redmi Note 10";
const char* password = "123456789";

// ThingSpeak API settings
const char* serverName = "http://api.thingspeak.com/update";
String apiKey = "H14ZLJN9GGADIPSJ";

// Constants for TDS Sensor
#define TdsSensorPin A0  // Analog pin connected to the TDS sensor
#define VREF 3.3          // ESP32 ADC reference voltage is 3.3V
#define SCOUNT 30         // Sample count for averaging

// Pin for the buzzer
#define BuzzerPin 27       // Replace 27 with the pin number connected to your buzzer

// Variables for TDS calculation
int analogBuffer[SCOUNT];
int analogBufferIndex = 0;
float averageVoltage = 0;
float tdsValue = 0;

void collectAnalogReadings();
void processTDSData();
void checkTDSValue();
void sendDataToThingSpeak(float tdsValue);
float getMedianVoltage(int *values, int size);
void sortArray(int *array, int size);

void setup() {
  Serial.begin(115200);
  pinMode(TdsSensorPin, INPUT);
  pinMode(BuzzerPin, OUTPUT);
  digitalWrite(BuzzerPin, LOW);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
}

void loop() {
  collectAnalogReadings();  // Collect data from the TDS sensor
  processTDSData();         // Process readings and calculate TDS value

  // Send data to ThingSpeak every 10 seconds
  static unsigned long lastThingSpeakTime = 0;
  if (millis() - lastThingSpeakTime > 10000) {
    lastThingSpeakTime = millis();
    sendDataToThingSpeak(tdsValue);
  }

  checkTDSValue(); // Check TDS value and control the buzzer
  delay(1000); // Small delay to avoid rapid looping
}

// Function to collect analog readings from the TDS sensor
void collectAnalogReadings() {
  static unsigned long lastSampleTime = 0;
  if (millis() - lastSampleTime > 40) {
    lastSampleTime = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);
    analogBufferIndex = (analogBufferIndex + 1) % SCOUNT;
  }
}

// Function to process TDS sensor data and calculate TDS value
void processTDSData() {
  static unsigned long lastPrintTime = 0;
  if (millis() - lastPrintTime > 800) {
    lastPrintTime = millis();
    averageVoltage = getMedianVoltage(analogBuffer, SCOUNT) * (VREF / 4095.0); // Adjust for ESP32 12-bit ADC
    tdsValue = (133.42 * averageVoltage * averageVoltage * averageVoltage
                - 255.86 * averageVoltage * averageVoltage
                + 857.39 * averageVoltage) * 0.5;

    Serial.print("Voltage: ");
    Serial.print(averageVoltage, 2);
    Serial.print(" V, TDS Value: ");
    Serial.print(tdsValue, 0);
    Serial.println(" ppm");
  }
}

// Function to check TDS value and control the buzzer
void checkTDSValue() {
  if (tdsValue > 200) {
    digitalWrite(BuzzerPin, HIGH); // Turn on the buzzer
  } else {
    digitalWrite(BuzzerPin, LOW);  // Turn off the buzzer
  }
}

// Function to send TDS data to ThingSpeak
void sendDataToThingSpeak(float tdsValue) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;
    
    String httpRequestData = "api_key=" + apiKey + "&field1=" + String(tdsValue, 2);
    http.begin(client, serverName);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpResponseCode = http.POST(httpRequestData);
    
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    http.end();
  } else {
    Serial.println("WiFi disconnected. Cannot send data.");
  }
}

// Function to calculate the median voltage for noise reduction
float getMedianVoltage(int *values, int size) {
  int temp[size];
  memcpy(temp, values, size * sizeof(int));
  sortArray(temp, size);
  return temp[size / 2];
}

// Function to sort an array using bubble sort (for simplicity)
void sortArray(int *array, int size) {
  for (int i = 0; i < size - 1; i++) {
    for (int j = 0; j < size - i - 1; j++) {
      if (array[j] > array[j + 1]) {
        int temp = array[j];
        array[j] = array[j + 1];
        array[j + 1] = temp;
      }
    }
  }
}
