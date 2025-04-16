#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include "esp_camera.h"
#include "camera_pins.h"

#define PLANT_MODULE_ID "8c6b5dcb-6451-4bfd-b9c2-bbf18e4c449a"

// MQTT Broker details
const char* MQTT_SERVER = "test.mosquitto.org";
const int MQTT_PORT = 1883;

// WiFi credentials
const char* ssid = "ParkWest.SynergyWifi.com";
const char* password = "Synergy.203.280.2029";

// Global topics & variables
char mqtt_topic[50];
unsigned int t1 = 0;
unsigned int t2 = 0;
unsigned int latency = 0;

// Create WiFi and MQTT clients
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

// Callback function for incoming MQTT messages
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.println(topic);
  
  // For demonstration, convert payload to a string (if printable)
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Payload: ");
  Serial.println(message);
}

// Attempt MQTT connection
void reconnect() {
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID or use a unique one
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect (no username or password in this example)
    if (mqtt.connect(clientId.c_str())) {
      Serial.println(" connected");
      // Resubscribe to topics as needed
      mqtt.subscribe("ESP32/Cam/Result");
    } else {
      Serial.print(" failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void publishImageWithMarkers() {
  // Capture image from camera
  camera_fb_t *fb = esp_camera_fb_get();  
  if (!fb) {
    Serial.println("Camera capture failed! Restarting ESP...");
    ESP.restart();
    return;
  }

  // show size, based on the quality this goes down
  Serial.printf("Captured Image, size: %d bytes\n", fb->len);

  strcpy(mqtt_topic, "ESP32/Cam/ImagePart");

  // Send "START" marker
  mqtt.publish(mqtt_topic, "START", false);
  Serial.println("START Marker Sent");
  Serial.print("Frame buffer address start: ");
  Serial.println((uintptr_t)(fb->buf), HEX);
  Serial.print("Frame length: ");
  Serial.println(fb->len);
  Serial.print("Frame buffer address end: ");
  Serial.println((uintptr_t)(fb->buf + fb->len), HEX);

  // Publish entire image payload in one go
  bool success = mqtt.publish(mqtt_topic, fb->buf, fb->len, false);
  if (success) {
    Serial.println("Sent full image in one message");
  } else {
    Serial.println("Failed to send full image in one message");
  }

  // Send "END" marker
  mqtt.publish(mqtt_topic, "END", false);
  Serial.println("Sent END marker");

  esp_camera_fb_return(fb);  // Free the frame buffer memory
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  // Initialize camera configuration
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG; 
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 25;
  config.fb_count = 1;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed!");
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  if (s->id.PID == OV2640_PID) {
    Serial.println("Detected Camera Lens: OV2640");
  }

  // Connect to WiFi
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Initialize MQTT settings
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);

  // Increase MQTT buffer to accommodate the full image payload
  mqtt.setBufferSize(8192 * 7);

  // Immediately attempt to connect if not already connected
  reconnect();
}

void loop() {
  // Ensure the MQTT client remains connected
  if (!mqtt.connected()) {
    reconnect();
  }
  mqtt.loop();

  // Publish the full image
  publishImageWithMarkers();

  // Increase delay between each picture (currently set to 10 seconds)
  delay(10000);
}
