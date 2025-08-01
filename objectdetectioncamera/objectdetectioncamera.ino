#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ==== WiFi Credentials ====
const char* ssid = "ADMISION OPEN SHARD CENTER 397";
const char* password = "Sanyam@397!#";

// ==== n8n Webhook URL ====
const char* serverName = "https://n8n-6mdr.onrender.com/webhook/690a8ca4-5136-4216-bddf-595fd4c4cf5c";

// ==== I2C LCD Setup ====
#define SDA_PIN 14
#define SCL_PIN 15
LiquidCrystal_I2C lcd(0x27, 16, 2); // Confirmed address

// ==== Button Pin ====
#define BUTTON_PIN 12  // Choose another GPIO if needed

// ==== Camera Init ====
void startCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = 5;
  config.pin_d1       = 18;
  config.pin_d2       = 19;
  config.pin_d3       = 21;
  config.pin_d4       = 36;
  config.pin_d5       = 39;
  config.pin_d6       = 34;
  config.pin_d7       = 35;
  config.pin_xclk     = 0;
  config.pin_pclk     = 22;
  config.pin_vsync    = 25;
  config.pin_href     = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn     = 32;
  config.pin_reset    = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_QVGA;
  config.jpeg_quality = 10;
  config.fb_count     = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Camera Init Fail");
  }
}

// ==== Send Image to Webhook ====
void sendImage() {
  Serial.println("Button Pressed: Sending image...");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sending image...");

  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed.");
    lcd.setCursor(0, 1);
    lcd.print("Capture failed");
    return;
  }

  HTTPClient http;
  http.begin(serverName);
  http.addHeader("Content-Type", "application/octet-stream");

  int httpResponseCode = http.POST(fb->buf, fb->len);
  String result;

  if (httpResponseCode > 0) {
    result = http.getString();
    Serial.println("Server response:");
    Serial.println(result);
  } else {
    result = "HTTP error";
    Serial.printf("HTTP POST failed: %s\n", http.errorToString(httpResponseCode).c_str());
  }

  esp_camera_fb_return(fb);
  http.end();

  // === Parse JSON ===
  String objectName = "";
  int start = result.indexOf(":\"") + 2;
  int end = result.indexOf("\"", start);
  if (start > 1 && end > start) {
    objectName = result.substring(start, end);
  } else {
    objectName = "unclear";
  }

  // === Display on LCD and Serial ===
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Detected:");
  lcd.setCursor(0, 1);
  lcd.print(objectName.substring(0, 16));  // First 16 chars only

  Serial.print("Detected: ");
  Serial.println(objectName);
}

void setup() {
  Serial.begin(115200);

  // ==== LCD Init ====
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.init();
  lcd.backlight();

  // ==== Button Setup ====
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // ==== WiFi Connect ====
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");
  WiFi.begin(ssid, password);
  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 15000) {
    delay(500);
    Serial.print(".");
  }

  lcd.clear();
  if (WiFi.status() == WL_CONNECTED) {
    lcd.setCursor(0, 0);
    lcd.print("WiFi Connected");
    Serial.println("\nWiFi connected.");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("WiFi Failed");
    Serial.println("\nWiFi failed.");
    return;
  }

  // ==== Camera Init ====
  startCamera();
}

void loop() {
  static bool buttonPressed = false;

  // Simple edge detection
  if (digitalRead(BUTTON_PIN) == LOW && !buttonPressed) {
    buttonPressed = true;
    sendImage();
  }

  if (digitalRead(BUTTON_PIN) == HIGH && buttonPressed) {
    buttonPressed = false;
  }
}
