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
// This file includes wifi ssid and password
#include "PrivateConstants.h"

//defining pins and type 
Adafruit_BMP280 bmp;
#define DHTPIN 12   
#define DHTTYPE DHT22

#define str(x) #x
#define new_reading(var, read_func)\
    {\
      float new_##var = (read_func); \
      if (isnan(new_##var)) { \
        Serial.println("Failed to read " str(var)); \
      } else { \
        var = new_##var; \
        Serial.print(str(var) ": "); \
        Serial.println(var); \
      }\
    }


DHT dht(DHTPIN, DHTTYPE);

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

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>

<head>
    <title>Dom&aacute;c&iacute; meteostanice</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css"
        integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
    <link href="https://fonts.googleapis.com/css2?family=Alatsi&family=Roboto&display=swap" rel="stylesheet"> 
    <style>
        html {
            font-family: Roboto;
            display: inline-block;
            margin: 0px auto;
            text-align: center;
            background-color: #1b262c;
            color: #ececec;
        }

        h2 {
            font-size: 3.0rem;
            font-family: Alatsi;
        }

        p {
            font-size: 3.0rem;
        }

        .units {
            font-size: 1.2rem;
        }

        .dht-labels {
            font-size: 1.5rem;
            vertical-align: middle;
            padding-bottom: 15px;
        }

        .light-mode {
            background-color: white;
            color: black;
        }
    </style>
</head>
<body>
    <h2>Dom&aacute;c&iacute; meteostanice</h2>
    <p>
        <i class="fas fa-thermometer-half" style="color: #e51a11;"></i>
        <span class="dht-labels">Teplota</span>
        <span id="temperature">%t%</span>
        <sup class="units">&deg;C</sup>
    </p>
    <p>
        <i class="fas fa-tint" style="color: #00add6;"></i>
        <span class="dht-labels">Vlhkost</span>
        <span id="humidity">%h%</span>
        <sup class="units">&#37;</sup>
    </p>
    <p>
        <i class="fas fa-tachometer-alt" style="color: #ffffff;"></i>
        <span class="dht-labels">Tlak</span>
        <span id="pressure">%p%</span>
        <sup class="units">hPa</sup>
    </p>
    <p>
        <i class="fas fa-cloud" style="color: #0b00be;"></i>
        <span class="dht-labels">Rosn&yacute; bod</span>
        <span id="dew">%d%</span>
        <sup class="units">&deg;C</sup>
    </p>
</body>
<script>
    setInterval(function () {
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function () {
            if (this.readyState == 4 && this.status == 200) {
                let new_data = this.responseText.split(";");
                document.getElementById("temperature").innerHTML = new_data[0];
                document.getElementById("humidity").innerHTML = new_data[1];
                document.getElementById("pressure").innerHTML = new_data[2];
                document.getElementById("dew").innerHTML = new_data[3];

            }
        };
        xhttp.open("GET", "/data", true);
        xhttp.send();
    }, 10000);

</script>

</html>
)rawliteral";

// Replaces placeholder with DHT values
String processor(const String& var) {
  char type = var[0];
  switch (type) {
    case 't':
      return String(temperature);
    case 'h':
      return String(humidity);
    case 'p':
      return String(pressure);
    case 'd':
      return String(dew);
    default:
      return "Proccessor error, `" + var + "` is not an existing var";
  }
}
// Set your Static IP address
IPAddress localIP(10,10,10,12);
IPAddress gatewayIP(10,10,10,1);
IPAddress subnetMask(255,255,255,0);

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  dht.begin();
  bmp.begin(0x76);

  // Connect to Wi-Fi
  WiFi.config(localIP, gatewayIP, subnetMask);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }
  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", 
      (String(temperature) + ";" + String(humidity) + ";" + String(pressure) + ";" + String(dew)).c_str());
  });
  
  // Start server
  server.begin();
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you updated the DHT values
    previousMillis = currentMillis;

    new_reading(temperature, ((bmp.readTemperature() + dht.readTemperature()) / 2 - 0.3));
    new_reading(humidity, (dht.readHumidity() - 4.5));
    new_reading(pressure, (bmp.readPressure() / 100.0F));
    dew = temperature - ((100 - humidity) / 5) - 2;
  }
}
