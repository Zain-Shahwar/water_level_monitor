#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <NewPing.h>

// Network credentials
const char* ssid = "AR_HOME";
const char* password = "abdulrehmaN1122";

const int trigPin = D1;
const int echoPin = D2;
const int maxDistance = 300; // Maximum distance we want to ping for (in centimeters).

NewPing sonar(trigPin, echoPin, maxDistance);

ESP8266WebServer server(80);

long lastDistance = 0; // Variable to store the latest distance measured

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
      text-align: center;
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
      var fullDepth = 300; // Assuming 300 cm is the depth of your tank
      var level = fullDepth - distance;
      var levelPercent = (level / fullDepth) * 100;
      return Math.min(Math.max(levelPercent.toFixed(0), 0), 100);
    }
    function updateTime() {
      var now = new Date();
      var dateString = now.toLocaleDateString();
      var timeString = now.toLocaleTimeString();
      document.getElementById("currentTime").innerHTML = dateString + " " + timeString;
    }
    setInterval(updateTime, 600);
  </script>
</body>
</html>
)=====";


void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi. IP Address: ");
  Serial.println(WiFi.localIP());

  // Web server routes
  server.on("/", handleRoot);
  server.on("/data", handleDistance);

  // Start server
  server.begin();
}

void loop() {
  lastDistance = getDistance();
  Serial.print("Distance: ");
  Serial.print(lastDistance);
  Serial.println(" cm");
  delay(500); // Delay for stability
  server.handleClient();
}

void handleRoot() {
  server.send_P(200, "text/html", webpage);
}

void handleDistance() {
  server.send(200, "text/plain", String(lastDistance));
}

long getDistance() {
  return sonar.ping_cm();
}
