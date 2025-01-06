#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>
#include <WebSocketsServer.h>

// Wi-Fi Credentials (must match the ESP32's AP credentials)
const char* ssid = "MyCar";
const char* password = "12345678";

WebSocketsServer webSocket(81);

IPAddress localIP(192, 168, 4, 2); // IP Address of ESP32 CAM

// Camera Pins and Configuration
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

void sendFrameOverWebSocket() {
  camera_fb_t * fb = esp_camera_fb_get();  // Capture frame from the camera
  if (!fb) {
      Serial.println("Camera capture failed");
      return;  
  }

  // Send frame over WebSocket
  webSocket.broadcastBIN(fb->buf, fb->len); 

  esp_camera_fb_return(fb); 
  delay(100);  // Adjust to control frame rate (FPS)
}

// WebSocket event handling for client connections
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  // We can send custom commands from the controller to the camera over WebSocket here, maybe for the light, TO DO later
}

void setup() {
    Serial.begin(115200);

    // Connect to the ESP32's AP
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to MyCar...");
    }

    //For debug purpose only, do not open serial monitor after camera connect to the esp 32
    Serial.print("Connected to MyCar. Camera IP: ");
    Serial.println(WiFi.localIP());

    webSocket.begin();

    // Camera Configuration
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_QVGA; 
    config.jpeg_quality = 12;
    config.fb_count = 1;

    if (esp_camera_init(&config) != ESP_OK) {
        Serial.println("Camera init failed!");
        return;
    }
}

void loop() {
   sendFrameOverWebSocket(); 
    webSocket.loop();
}
