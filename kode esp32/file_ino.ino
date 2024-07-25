#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

const char* ssid = "HOTSPOT GURU";
const char* password = "0251487848";

const char* serverAddress = "http://30.30.30.23:5000/submit"; // Ganti dengan IP server yang benar
const char* fanCommandAddress = "http://30.30.30.23:5000/fan_command"; // Route untuk mendapatkan status kipas

#define DHTPIN 4
#define DHTTYPE DHT11
#define FANPIN 5 // Pin untuk mengontrol kipas, ganti dengan pin yang sesuai

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(FANPIN, OUTPUT); // Set pin kipas sebagai output
  digitalWrite(FANPIN, LOW); // Memastikan kipas mati saat mulai

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void loop() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity(); // Baca kelembaban

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    delay(2000);
    return;
  }

  // Dapatkan status kipas dari server Flask
  HTTPClient http;
  http.begin(fanCommandAddress);
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Fan command response: " + response);

    // Parsing JSON response untuk mendapatkan status kipas
    // Asumsikan response berupa {"status": "on"} atau {"status": "off"}
    if (response.indexOf("\"status\":\"ON\"") > 0) {
      digitalWrite(FANPIN, HIGH); // Menyalakan kipas
    } else if (response.indexOf("\"status\":\"OFF\"") > 0) {
      digitalWrite(FANPIN, LOW); // Mematikan kipas
    }
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();

  // Mengirim data sensor ke server Flask
  http.begin(serverAddress);
  http.addHeader("Content-Type", "application/json");

  int fanStatus = digitalRead(FANPIN); // 1 jika kipas menyala, 0 jika mati
  String jsonData = "{\"temp\":" + String(temperature) + ", \"hum\":" + String(humidity) + ", \"fan\":" + String(fanStatus) + "}";
  Serial.println("Sending data: " + jsonData);

  httpResponseCode = http.POST(jsonData);

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    String response = http.getString();
    Serial.println(response);
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }

  http.end();

  delay(300000); // 5 menit
}