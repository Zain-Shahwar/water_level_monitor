#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>

// WiFi credentials
const char* ssid = "xxxxx";
const char* password = "xxxxxxx";

WebServer server(80);

// Ultrasonic Sensor Pins
#define TRIG_PIN  26
#define ECHO_PIN  25
#define LED_PIN   14
#define DISTANCE_THRESHOLD 20

float duration_us, distance_cm;

// HTML content
const char webpage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
  <title>Water Level Monitoring System</title>
  <style>
    body { text-align: center; }
    .header { font-size: 24px; margin-top: 20px; }
    .container {
      width: 300px;
      height: 500px;
      border: 3px solid #0d6efd;
      border-radius: 15px;
      margin: 20px auto;
      position: relative;
      background-color: #e3f2fd;
    }
    .water-level {
      width: 100%;
      position: absolute;
      bottom: 0;
      text-align: center;
      color: white;
      border-top-left-radius: 15px;
      border-top-right-radius: 15px;
    }
    .status-text {
      position: absolute;
      width: 100%;
      bottom: 10px;
      text-align: center;
      font-size: 20px;
      color: #0d6efd;
    }
    .sensor-reading {
      font-size: 18px;
      margin-top: 10px;
      text-align: center; // Align text to the center
    }
    .current-time {
  font-size: 18px;
  margin-top: 10px;
  text-align: left;
}
  </style>
</head>
<body>
  <div id="currentTime" class="current-time">Loading time...</div>
  <div class="header">Water Level Monitoring</div>
  <div id="sensorReading" class="sensor-reading">Sensor Reading: 0 cm</div>
  <div class="container">
    <div id="waterLevel" class="water-level" style="height: 50%; background-color: #0d6efd;">50%</div>
    <div class="status-text">Water Level</div>
  </div>
  

  <script>
    setInterval(function() {getData();}, 1000);
    function getData() {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
          var distance = parseFloat(this.responseText);
          var levelPercent = calculateLevel(distance);
          var waterLevelDiv = document.getElementById("waterLevel");
          waterLevelDiv.innerHTML = levelPercent + "%";
          waterLevelDiv.style.height = levelPercent + "%";
          document.getElementById("sensorReading").innerHTML = "Sensor Reading: " + distance + " cm";
          if(levelPercent > 90) {
            waterLevelDiv.style.backgroundColor = "#28a745"; // Green
          } else if(levelPercent <= 25) {
            waterLevelDiv.style.backgroundColor = "#dc3545"; // Red
          } else {
            waterLevelDiv.style.backgroundColor = "#0d6efd"; // Blue
          }
        }
      };
      xhttp.open("GET", "data", true);
      xhttp.send();
    }
    function calculateLevel(distance) {
      var fullDepth = 250; // Assuming 250 cm is the depth of your tank
      var level = fullDepth - distance;
      var levelPercent = (level / fullDepth) * 100;
      return Math.min(Math.max(levelPercent.toFixed(2), 0), 100); // Round to 2 decimal places
    }
    function updateTime() {
    var now = new Date();
    var dateString = now.toLocaleDateString();
    var timeString = now.toLocaleTimeString();
    document.getElementById("currentTime").innerHTML = dateString + " " + timeString;
  }

  // Update time every second
  setInterval(updateTime, 1000);
  </script>
</body>
</html>
)=====";


void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", webpage);
  });

  server.on("/data", HTTP_GET, []() {
    server.send(200, "text/plain", String(distance_cm));
  });

  server.begin();
}

void loop() {
  unsigned long currentMillis = millis();
  static unsigned long previousMillis = 0;
  const long interval = 1000;

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    measureDistance();
    Serial.print("Distance: ");
    Serial.print(distance_cm);
    Serial.println(" cm");

    if (distance_cm < DISTANCE_THRESHOLD)
      digitalWrite(LED_PIN, HIGH);
    else
      digitalWrite(LED_PIN, LOW);

    // Calculate water level percentage
    float fullDepth = 250.0; // Adjust this value based on your tank depth
    float level = fullDepth - distance_cm;
    float levelPercent = (level / fullDepth) * 100;
    levelPercent = constrain(levelPercent, 0, 100);

    // Send data to Flask server
    HTTPClient http;
    http.begin("http://192.xxxxxx:5000/update"); // Replace with your Flask server's IP and port
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    int httpResponseCode = http.POST("level=" + String(levelPercent));
  
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(response);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  }

  server.handleClient();
}

void measureDistance() {
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  duration_us = pulseIn(ECHO_PIN, HIGH);
  distance_cm = 0.017 * duration_us;
}
