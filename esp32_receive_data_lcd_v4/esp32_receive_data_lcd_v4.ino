#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

const char* ssid = "AR_HOME";
const char* password = "abdulrehmaN1122";
const char* serverName = "http://192.168.110.82/data";

// Define LED pins
const int ledPin13 = 13; // Red LED
const int ledPin14 = 14; // Blue LED
const int ledPin12 = 12; // Green LED

// Define LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Set the LCD address to 0x27 for a 16 chars and 2 line display

void setup() {
  Serial.begin(115200);

  pinMode(ledPin13, OUTPUT);
  pinMode(ledPin14, OUTPUT);
  pinMode(ledPin12, OUTPUT);

  // Initialize LCD
  lcd.init();                      
  lcd.backlight();
  lcd.setCursor(0, 0); 
  lcd.print("Water level: ");

  WiFi.mode(WIFI_MODE_STA); // Set WiFi to station mode
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void loop() {
  static unsigned long lastAttemptTime = 0;
  const unsigned long requestInterval = 2000; // Reduced to 2 seconds between requests

  if (millis() - lastAttemptTime > requestInterval) {
    lastAttemptTime = millis();

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Reconnecting to WiFi...");
      WiFi.disconnect();
      WiFi.reconnect();
      return;
    }

    HTTPClient http;
    http.begin(serverName); // Open connection
    http.addHeader("Connection", "close"); // Close connection after completion

    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      String payload = http.getString();
      long distance = payload.toInt();
      Serial.print("Distance: ");
      Serial.println(distance);

      // LED control based on distance
      if (distance > 95) {
        digitalWrite(ledPin13, HIGH); // Red
        digitalWrite(ledPin14, LOW);
        digitalWrite(ledPin12, LOW);
      } else if (distance < 30) {
        digitalWrite(ledPin13, LOW);
        digitalWrite(ledPin14, HIGH); // Blue
        digitalWrite(ledPin12, LOW);
      } else {
        digitalWrite(ledPin13, LOW);
        digitalWrite(ledPin14, LOW);
        digitalWrite(ledPin12, HIGH); // Green
      }

      float waterLevelPercentage = (1.0f - (float)distance / 110.0f) * 100.0f;
      waterLevelPercentage = constrain(waterLevelPercentage, 0.0f, 100.0f);

      // Update tank level percentage on the LCD
      lcd.setCursor(12, 0);
      if (waterLevelPercentage < 100) lcd.print(" "); // For alignment
      if (waterLevelPercentage < 10) lcd.print(" "); // For alignment
      lcd.print(waterLevelPercentage, 0); // Display percentage without decimal places
      lcd.print("%");

      // Update bars on the LCD
      int barsToShow = waterLevelPercentage / (100.0 / 16); // 16 bars for 100%
      lcd.setCursor(0, 1);
      for(int i = 0; i < 16; i++) {
        lcd.write(i < barsToShow ? byte(255) : ' '); // Solid block or space
      }
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
      lcd.setCursor(0, 1);
      lcd.print("Er:");
      lcd.print("Dev not found");  // Show HTTP error code
      lcd.print("     ");  // Clear remaining characters on the line
    }

    http.end(); // Close connection
  } else {
    // Short delay in the loop
    delay(100); // Reduced delay
  }
}
