// Import required libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <Adafruit_BMP280.h>
#include <Wire.h>
#include <FS.h> // Include the SPIFFS library
// This file includes wifi ssid and password
#include "PrivateConstants.h"

// Uncomment to enable debugging
//#define DEBUG

#define str(x) #x
#define new_reading(var, read_func)               \
  {                                               \
    float new_##var = (read_func);                \
    if (isnan(new_##var))                         \
    {                                             \
      Serial.println("Failed to read " str(var)); \
    }                                             \
    else                                          \
    {                                             \
      var = new_##var;                            \
      Serial.print(str(var) ": ");                \
      Serial.println(var);                        \
    }                                             \
  }

#define DHTPIN 12
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

//defining pins and type
Adafruit_BMP280 bmp;

// current temperature & humidity, updated in loop()
float temperature = 0.0,
      humidity = 0.0,
      pressure = 0.0,
      dew = 0.0;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

unsigned long previousMillis = 0;

// Updates DHT readings every 10 seconds
const long interval = 10000;

// Replaces placeholder with DHT values
String processor(const String &var)
{
  if (var == "temperature")
    return String(temperature);
  if (var == "humidity")
    return String(humidity);
  if (var == "pressure")
    return String(pressure);
  if (var == "dew")
    return String(dew);
  if (var == "last_update_time")
    return String(previousMillis);
  return "\"Proccessor error, `" + var + "` is not an existing variable template\"";
}

// Set your Static IP address
IPAddress localIP(10, 10, 10, 12);
IPAddress gatewayIP(10, 10, 10, 1);
IPAddress subnetMask(255, 255, 255, 0);
IPAddress dns(10, 10, 10, 1);

void setup()
{
// Serial port for debugging purposes
#ifdef DEBUG
  Serial.begin(115200);
#endif
  dht.begin();
  bmp.begin(0x76);

  // Connect to Wi-Fi
  WiFi.config(localIP, gatewayIP, subnetMask, dns);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println(".");
  }
  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());

  SPIFFS.begin();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", "text/html", false, processor);
  });

  // Route for API
  server.on("/api", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/api.json", "application/json", false, processor);
  });

  // Start server
  server.begin();
}

void loop()
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    // save the last time you updated the DHT values
    previousMillis = currentMillis;

    new_reading(temperature, ((bmp.readTemperature() + dht.readTemperature()) / 2 - 0.3));
    new_reading(humidity, (dht.readHumidity() - 4.5));
    new_reading(pressure, (bmp.readPressure() / 100.0F));
    dew = temperature - ((100 - humidity) / 5) - 2;
  }
}
