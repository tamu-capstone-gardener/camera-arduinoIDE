#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "esp_wpa2.h"
#include <PubSubClient.h>
#include "esp_camera.h"
#include "camera_pins.h"

#define PLANT_MODULE_ID "****"
#define MQTT_USERNAME "planthub"
#define MQTT_PASSWORD "****"
#define WIFI_USERNAME "****"
#define WIFI_PASSWORD "****"
#define WIFI_SSID "*****"

// MQTT Broker details
const char* MQTT_SERVER = "*****";
const int MQTT_PORT = ****;  // Use your broker's port

// WiFi credentials
const char* ssid = "****";
const char* password = "******";

// Global topics & variables
char mqtt_topic[50];
unsigned int t1 = 0;
unsigned int t2 = 0;
unsigned int latency = 0;

// Create WiFi and MQTT clients
WiFiClientSecure wifiClient;
PubSubClient mqtt(wifiClient);

// for TAMU WiFi
// if using make sure to include esp_wpa2 
void connectToWiFi() {
  WiFi.disconnect(true);  // Reset WiFi

  WiFi.mode(WIFI_STA);
  esp_wifi_sta_wpa2_ent_enable();

  // Set enterprise credentials
  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)WIFI_USERNAME, strlen(WIFI_USERNAME));
  esp_wifi_sta_wpa2_ent_set_username((uint8_t *)WIFI_USERNAME, strlen(WIFI_USERNAME));
  esp_wifi_sta_wpa2_ent_set_password((uint8_t *)WIFI_PASSWORD, strlen(WIFI_PASSWORD));

  WiFi.begin(WIFI_SSID);  // Just SSID — no password here

  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("WiFi Status: ");
    Serial.println(WiFi.status());
  }

  Serial.println("\nConnected to WiFi.");
  Serial.println(WiFi.localIP());
}

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
    if (mqtt.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
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
  camera_fb_t *fb = esp_camera_fb_get();  // Capture image
  if (!fb) {
      Serial.println("Camera capture failed! Restarting ESP...");
      ESP.restart();
      return;
  }

  Serial.printf("Captured Image, size: %d bytes\n", fb->len);

  const int chunkSize = 1024 * 4;  // 8KB per chunk
  int totalSize = fb->len;
  int NoI = totalSize / chunkSize;
  sprintf(mqtt_topic, "planthub/%s/photo", PLANT_MODULE_ID);

  //   // Publish a simple test message
  // if (mqtt.publish(mqtt_topic, "Test Message", false)) {
  //   Serial.println("Test Message Sent Successfully");
  // } else {
  //   Serial.println("Test Message Failed");
  // }

  // Send "start" marker
  mqtt.publish(mqtt_topic, "START", false);
  Serial.println("START Marker Sent");
  Serial.print("Frame buffer address start: ");
  Serial.println((uintptr_t)(fb->buf), HEX);
  Serial.print("Frame length: ");
  Serial.println(fb->len);
  Serial.print("Frame buffer address end: ");
  Serial.println((uintptr_t)(fb->buf + fb->len), HEX);

  // Transmit image chunks  
  for (int i = 0; i < NoI; i++) {
    Serial.print("Transmitting from address: ");
    Serial.print((uintptr_t)(fb->buf + (i * chunkSize)), HEX);
    Serial.print(" to ");
    Serial.println((uintptr_t)(fb->buf + (i * chunkSize) + chunkSize), HEX);

    // Publish each chunk (remove qos parameter)
    bool success = mqtt.publish(mqtt_topic, fb->buf + (i * chunkSize), chunkSize, false);
    if (!success) {
      Serial.println("Failed to send image chunk!");
      break;
    }
    Serial.printf("Sent chunk %d/%d (%d bytes)\n", i, totalSize / chunkSize, chunkSize);
    delay(10);  // Small delay to prevent buffer overflow
  }

  // Send the remaining bytes (if any)
  int remaining = totalSize - (NoI * chunkSize);
  if (remaining > 0) {
    Serial.print("Transmitting last bytes from address: ");
    Serial.print((uintptr_t)(fb->buf + (NoI * chunkSize)), HEX);
    Serial.print(" to ");
    Serial.println((uintptr_t)(fb->buf + (NoI * chunkSize) + remaining), HEX);
    mqtt.publish(mqtt_topic, fb->buf + (NoI * chunkSize), remaining, false);
    Serial.println("Sent last bytes");
  }

  // Send "end" marker
  mqtt.publish(mqtt_topic, "END", false);
  Serial.println("Sent END marker");

  esp_camera_fb_return(fb);  // Free the frame buffer memory
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  // Initialize camera configuration (remains similar)
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
  connectToWiFi();
  // WiFi.begin(ssid, password);
  // WiFi.setSleep(false);
  // while (WiFi.status() != WL_CONNECTED) {
  //     delay(500);
  //     Serial.print(".");
  // }
  // Serial.println("");
  // Serial.println("WiFi connected");

  // Initialize MQTT settings
  wifiClient.setInsecure();
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);

  mqtt.setBufferSize(8192*7);

  // Immediately attempt to connect if not already connected
  reconnect();
}


void loop() {
  // Ensure the MQTT client remains connected
  if (!mqtt.connected()) {
    reconnect();
  }
  mqtt.loop();

  // Publish the image
  publishImageWithMarkers();

  // Optional delay between images
  delay(30000);
}

