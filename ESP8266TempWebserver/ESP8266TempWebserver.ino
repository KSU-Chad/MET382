/*********
  This project has been adapted from Random Nerd Tutorials and Adafruit.
  
  Rui Santos
  Complete project details at https://randomnerdtutorials.com/esp8266-nodemcu-access-point-ap-web-server/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

 Adafruit
 SPDX-FileCopyrightText: 2011 Limor Fried/ladyada for Adafruit Industries

 SPDX-License-Identifier: MIT

 Thermistor Example #3 from the Adafruit Learning System guide on Thermistors 
 https://learn.adafruit.com/thermistor/overview by Limor Fried, Adafruit Industries
 MIT License - please keep attribution and consider buying parts from Adafruit
*********/

// Import required libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

//***********************************
//Modify the following values to fit the application
const char* ssid     = "TempTracker1";
const char* password = "12345678";
#define THERMISTORNOMINAL **** // nominal resistance at 25 °C
#define BCOEFFICIENT **** // The beta coefficient of the thermistor (usually 3000-4000)
#define SERIESRESISTOR **** // the value of the 'other' voltage deivider resistor
//***********************************

#define DHTPIN 5     // Digital pin connected to the DHT sensor
#define DHTTYPE    DHT11     // DHT 11
#define THERMISTORPIN A0  // Define thermistor pin

DHT dht(DHTPIN, DHTTYPE);

// current temperature & humidity, updated in loop()
float t = 0.0;
float h = 0.0;
float reading;
float steinhart;
float r = 0.0;
float s = 0.0;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncEventSource events("/events");

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;    // will store last time DHT was updated

// Updates DHT readings every 10 seconds
const long interval = 2000;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    .topnav { overflow: hidden; background-color: #4B1D3F; color: white; font-size: 1.5rem; }
    .content { padding: 15px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
    .reading { font-size: 2.8rem; }
    .card.temperature { color: #0e7c7b; }
    .card.humidity { color: #17bebb; }
    .card.resistance { color: #3fca6b; }
    .card.thermistor { color: #d62246; }
  </style>
</head>
<body>
  <div class="topnav">
    <h3>Temperature Tracker</h3>
  </div>
  <div class="content">
    <div class="cards">
      <div class="card temperature">
        <h4> TEMPERATURE</h4><p><span class="reading"><span id="temperature">%TEMPERATURE%</span> &deg;C</span></p>
      </div>
      <div class="card humidity">
        <h4> HUMIDITY</h4><p><span class="reading"><span id="humidity">%HUMIDITY%</span> &percnt;</span></p>
      </div>
      <div class="card resistance">
        <h4> RESISTANCE</h4><p><span class="reading"><span id="resistance">%RESISTANCE%</span> K&ohm;</span></p>
      </div>
      <div class="card thermistor">
        <h4> THERMISTOR TEMPERATURE</h4><p><span class="reading"><span id="thermistor">%THERMISTOR%</span> &deg;C</span></p>
      </div>
    </div>
  </div>

</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 2000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 2000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("resistance").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/resistance", true);
  xhttp.send();
}, 2000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("thermistor").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/thermistor", true);
  xhttp.send();
}, 2000 ) ;

</script>
</html>)rawliteral";

// Replaces placeholder with DHT values
String processor(const String& var) {
  //Serial.println(var);
  if (var == "TEMPERATURE") {
    return String(t);
  }
  else if (var == "HUMIDITY") {
    return String(h);
  }
  else if (var == "RESISTANCE") {
    return String(r);
  }
  else if (var == "THERMISTOR") {
    return String(s);
  }
  return String();
}

void setup() {
  // Serial port for debugging purposes
  Serial.begin(115200);
  dht.begin();

  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(t).c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(h).c_str());
  });
    server.on("/resistance", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(r).c_str());
  });
  server.on("/thermistor", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(s).c_str());
  });
  // Handle Web Server Events
//  events.onConnect([](AsyncEventSourceClient *client){
//    if(client->lastId()){
//      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
//    }
//    // send event with message "hello!", id current millis
//    // and set reconnect delay to 1 second
//    client->send("hello!", NULL, millis(), 1000);
//  });
  server.addHandler(&events);
  
  // Start server
  server.begin();
}

void loop() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    // save the last time you updated the DHT values
    previousMillis = currentMillis;
    // Read temperature as Celsius (the default)
    float newT = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    //float newT = dht.readTemperature(true);
    // if temperature read failed, don't change t value
    if (isnan(newT)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      t = newT;
      Serial.println(t);
    }
    // Read Humidity
    float newH = dht.readHumidity();
    // if humidity read failed, don't change h value
    if (isnan(newH)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      h = newH;
      Serial.println(h);
    }
    // Read Thermistor
    reading = analogRead(THERMISTORPIN);
    Serial.print("Analog reading ");
    Serial.println(reading);
    // convert the value to resistance
    //    reading = (SERIESRESISTOR*reading)/(1023 - reading) ;
    reading = (1023 / reading - 1) ;   // (1023/ADC - 1)
    reading = SERIESRESISTOR / reading;  // 10K / (1023/ADC - 1)
    r = reading/1000;
    Serial.print("Thermistor resistance ");
    Serial.println(reading);

    steinhart = reading / THERMISTORNOMINAL;     // (R/Ro)
    steinhart = log(steinhart);                  // ln(R/Ro)
    steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
    steinhart += 1.0 / (25 + 273.15); // + (1/To)
    steinhart = 1.0 / steinhart;                 // Invert
    steinhart -= 273.15;                         // convert absolute temp to C
    s=steinhart;

    Serial.print("Temperature ");
    Serial.print(steinhart);
    Serial.println(" °C");

     // Send Events to the Web Server with the Sensor Readings
//    events.send("ping",NULL,millis());
//    events.send(String(t).c_str(),"temperature",millis());
//    events.send(String(h).c_str(),"humidity",millis());
//    events.send(String(r).c_str(),"resistance",millis());
//    events.send(String(s).c_str(),"thermistor",millis());
  }
}
