// Import required libraries
#include <WiFi.h>
#include <Hash.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

// Replace with your network credentials
const char* ssid = "....";
const char* password = "....";

#define DHTPIN 23    // Digital pin D23 ESP32 connected to the DHT sensor

// Uncomment the type of sensor in use:
#define DHTTYPE    DHT11     // DHT 11
//#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);

// current temperature & humidity, updated in loop()
float t = 0.0;
float h = 0.0;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;    // will store last time DHT was updated

// Updates DHT readings every 10 seconds
const long interval = 500;  

const char index_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en" >
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Bootstrap demo</title>
    <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.2.3/dist/css/bootstrap.min.css" rel="stylesheet" integrity="sha384-rbsA2VBKQhggwzxH7pPCaAqO46MgnOM80zW1RWuH61DGLwZJEdK2Kadq2F9CUG65" crossorigin="anonymous">
    <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.2.3/dist/js/bootstrap.bundle.min.js" integrity="sha384-kenU1KFdBIe4zVF0s0G1M5b4hcpxyD9F7jL+jjXkk+Q2h455rYXK/7HAuoJl+0I4" crossorigin="anonymous"></script>

</head>
  <body style="background-color: #191825">

    <h2 class="fw-bold mx-auto text-center" style="margin-top: 10px; font-size: 40px; color: aliceblue;">SERVER DHT</h2>

    
    <div class="card mx-auto text-center" style="width: 20rem; align-items: center; margin-top: 20px; background-color: #0C134F; color: aliceblue;">
        <div class="card-body d-flex align-items-center justify-content-center">
          <p class="card-text fw-bold" style="font-size: 30px;">TEMPERATURE</p>
        </div>
      </div>

      <div class="card mx-auto text-center" style="width: 20rem; height: 4.5rem; align-items: center; margin-top: 10px; background-color: #1D267D; color: aliceblue;">
        <div class="card-body d-flex align-items-center justify-content-center">
          <p class="card-text fw-bold" style="font-size: 20px;" id="temperature">%TEMPERATURE%</p>
          <p class="card-text fw-bold mx-auto" style="font-size: 20px; margin-bottom: 15.3px;"><span>°C</span></p>
        </div>
      </div>

      <div class="card mx-auto text-center" style="width: 20rem; align-items: center; margin-top: 20px; background-color: #0C134F; color: aliceblue;">
        <div class="card-body d-flex align-items-center justify-content-center">
          <p class="card-text fw-bold" style="font-size: 30px;">HUMIDITY</p>
        </div>
      </div>

      <div class="card mx-auto text-center" style="width: 20rem; height: 4.5rem; align-items: center; margin-top: 10px; background-color: #1D267D; color: aliceblue;">
        <div class="card-body d-flex align-items-center justify-content-center">
          <p class="card-text fw-bold" style="font-size: 20px;" id="humidity">%HUMIDITY%</p>
          <p class="card-text fw-bold mx-auto" style="font-size: 20px; margin-bottom: 15.3px;"><span>%</span></p>
        </div>
      </div>

      <div class="card mx-auto text-center" style="width: 20rem; align-items: center; margin-top: 20px; background-color: #0C134F; color: aliceblue;">
        <div class="card-body d-flex align-items-center justify-content-center">
          <p class="card-text fw-bold" style="font-size: 30px;">WAKTU</p>
        </div>
      </div>

      <div class="card mx-auto text-center" style="width: 20rem; height: 4.5rem; align-items: center; margin-top: 10px; background-color: #1D267D; color: aliceblue;">
        <div class="card-body d-flex align-items-center justify-content-center">
          <p class="card-text fw-bold" style="font-size: 20px;" id="clock"></p>
        </div>
      </div>
  </body>
</html>

<script>
    function updateTime() {
  var now = new Date();
  var hours = now.getHours();
  var minutes = now.getMinutes();
  var seconds = now.getSeconds();

  hours = addLeadingZero(hours);
  minutes = addLeadingZero(minutes);
  seconds = addLeadingZero(seconds);

  var timeString = hours + ":" + minutes + ":" + seconds;
  document.getElementById("clock").textContent = timeString;
}

function addLeadingZero(number) {
  return number < 10 ? "0" + number : number;
}

setInterval(updateTime, 1000);

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 500 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 500 ) ;

</script>)rawliteral";

// Replaces placeholder with DHT values
String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATURE"){
    return String(t);
  }
  else if(var == "HUMIDITY"){
    return String(h);
  }
  return String();
}

void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  dht.begin();
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }

  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(t).c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(h).c_str());
  });

  // Start server
  server.begin();
}
 
void loop(){  
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
  }
}
