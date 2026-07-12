/*
 * ============================================================
 *  AI-BASED SMART GARBAGE SEGREGATION BIN
 *  ESP32-CAM (AI-Thinker) — CLEAN UART0 VERSION
 *  Debug prints removed from Serial (UART0) since it shares
 *  the same wire as the CAPTURE command / result byte to Arduino.
 *  Flash LED blink patterns are used for status instead.
 * ============================================================
 */

#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>

const char* WIFI_SSID     = "Oppo Reno 11";
const char* WIFI_PASSWORD = "0123456789";
const char* SERVER_URL    = "http://10.201.203.12:5000/classify";
// UPDATE THIS IP after running ipconfig on laptop

#define FLASH_LED_PIN 4
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

bool cameraReady  = false;
bool wifiConnected = false;

void setup() {
  // Serial is UART0 — this is the ONLY thing allowed to write to it
  // besides the single result byte. No debug prints after this point.
  Serial.begin(9600);
  delay(2000);

  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);

  cameraReady = initCamera();
  blinkStatus(cameraReady ? 1 : 5);   // 1 blink = camera OK, 5 = camera failed

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(1000);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int retries = 0;
  while (WiFi.status() != WL_CONNECTED && retries < 40) {
    delay(500);
    retries++;
  }
  wifiConnected = (WiFi.status() == WL_CONNECTED);
  blinkStatus(wifiConnected ? 2 : 5);  // 2 blinks = WiFi OK, 5 = WiFi failed

  blinkStatus(3);  // 3 blinks = setup complete, ready
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd == "CAPTURE") {
      char result = captureAndClassify();
      // ONLY this single byte goes to Arduino — nothing else on Serial.
      Serial.write(result);
      Serial.flush();
    }
  }
}

char captureAndClassify() {
  if (!cameraReady || !wifiConnected) {
    blinkStatus(4);  // 4 blinks = not ready, returning fallback
    return '9';
  }

  digitalWrite(FLASH_LED_PIN, HIGH);
  delay(150);
  camera_fb_t* discard = esp_camera_fb_get();
  if (discard) esp_camera_fb_return(discard);
  delay(50);
  camera_fb_t* fb = esp_camera_fb_get();
  digitalWrite(FLASH_LED_PIN, LOW);

  if (!fb) {
    blinkStatus(4);
    return '9';
  }

  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "image/jpeg");
  http.setTimeout(8000);
  int httpCode = http.POST(fb->buf, fb->len);
  esp_camera_fb_return(fb);

  char result = '9';
  if (httpCode == 200) {
    String response = http.getString();
    response.trim();
    if (response.length() == 1) {
      char c = response[0];
      if ((c >= '0' && c <= '4') || c == '9') {
        result = c;
      }
    }
  }
  http.end();
  return result;
}

bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0; config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0=Y2_GPIO_NUM; config.pin_d1=Y3_GPIO_NUM; config.pin_d2=Y4_GPIO_NUM;
  config.pin_d3=Y5_GPIO_NUM; config.pin_d4=Y6_GPIO_NUM; config.pin_d5=Y7_GPIO_NUM;
  config.pin_d6=Y8_GPIO_NUM; config.pin_d7=Y9_GPIO_NUM;
  config.pin_xclk=XCLK_GPIO_NUM; config.pin_pclk=PCLK_GPIO_NUM;
  config.pin_vsync=VSYNC_GPIO_NUM; config.pin_href=HREF_GPIO_NUM;
  config.pin_sscb_sda=SIOD_GPIO_NUM; config.pin_sscb_scl=SIOC_GPIO_NUM;
  config.pin_pwdn=PWDN_GPIO_NUM; config.pin_reset=RESET_GPIO_NUM;
  config.xclk_freq_hz=20000000; config.pixel_format=PIXFORMAT_JPEG;
  config.frame_size=FRAMESIZE_QVGA; config.jpeg_quality=12; config.fb_count=1;
  esp_err_t err = esp_camera_init(&config);
  return (err == ESP_OK);
}

// Blinks the flash LED N times as a status code, since Serial can't
// carry debug text without corrupting the Arduino link.
void blinkStatus(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(FLASH_LED_PIN, HIGH);
    delay(150);
    digitalWrite(FLASH_LED_PIN, LOW);
    delay(150);
  }
  delay(500);
}
