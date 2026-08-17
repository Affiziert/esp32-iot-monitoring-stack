/*
 * ESP-NOW RX Node
 *
 * ArduinoIDE configuration:
 * - ESP32 Arduino Core: 3.3.11
 * - Board: ESP32C3 Dev Module
 * - CPU Frequency: 80 MHz
 * - USB CDC On Boot: Enabled
 * - Serial Baud Rate: 115200
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// =================== Device Config ===================
// Firmware version
static constexpr char firmware_version[] = "1.0.0";

// Protocol version
static constexpr uint8_t  protocol_version   = 1;   // Must match the transmitter version

// Wi-Fi settings
static const uint8_t wifi_channel = 6; // Wi-Fi channel
static constexpr wifi_power_t wifi_power = WIFI_POWER_15dBm; // Transmission power. Some ESP32-C3 boards show an unstable connection above transmission power WIFI_POWER_15dBm.

// Maximum sensor ID value (because uint8_t range: 0-255)
static constexpr uint8_t max_sensor_ID = 255; // Reserved for future sensor ID validation

// Enable serial debug output
// 0 = normal operation
// 1 = debug mode
#define DEBUG_SERIAL 0 

// =================== Payload Definition ===================
// Data structure transmitted via ESP-NOW
// Packed to ensure a fixed payload size of 8 bytes
struct __attribute__((packed)) Payload {
  uint8_t  version;
  uint8_t  sensorId;
  int16_t  temp;
  uint16_t humidity;
  uint16_t battery;
};

static_assert(sizeof(Payload) == 8,"Payload size must be 8 bytes");

// =================== Function Implementations ===================
// Initialize ESP-NOW Function
static void initEspNow() {
  WiFi.mode(WIFI_STA); // Set Wi-Fi settings
  WiFi.setChannel(wifi_channel);
  WiFi.setTxPower(wifi_power);

  if (esp_now_init() != ESP_OK) { // Initialize ESP-NOW
    if (DEBUG_SERIAL == 1){
      delay (100);
      Serial.println("ESP-NOW: Initialising failed");
    }
    while (true) {
      delay(1000);
    }
  }
}

// ESP-NOW callback handler
void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if (len != (int)sizeof(Payload)) {  // Ignore packets with unexpected payload size
    if (DEBUG_SERIAL == 1){
     Serial.println("Callback ERROR: Invalid payload size");
    } 
    return;
  }

  Payload p;
  memcpy(&p, data, sizeof(p)); // Copy received bytes into payload structure

  if (p.version != protocol_version) {  // Ignore packets with unsupported protocol version
    if (DEBUG_SERIAL == 1) {
     Serial.println("Callback ERROR: Protocol version mismatch");
    }
    return;
  }

  if (p.sensorId == 0) {  // Ignore packets with invalid sensor ID
   if (DEBUG_SERIAL == 1) {
      Serial.println("Callback ERROR: Invalid sensor ID");
    }
    return;
  }

  const uint8_t* mac = info->src_addr; // Get transmitter MAC address

  Serial.printf( // Output received data as JSON for serial processing
  "{\"id\":%u,"
  "\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\","
  "\"temp\":%.2f,\"rh\":%.2f,\"batt\":%u}\n",
  p.sensorId,
  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
  p.temp / 100.0f,
  p.humidity / 100.0f,
  p.battery
  );
}

void setup() {
  Serial.begin(115200); // Initialize serial interface
  while (!Serial) {}

  initEspNow();  // Initialize ESP-NOW
  esp_now_register_recv_cb(onRecv); // Register ESP-NOW transmission callback
  delay (100);
  
  // Initialization completed successfully
  if (DEBUG_SERIAL == 1){
    Serial.println("ESP-NOW RX Node initialized successfully.");
    Serial.print("Firmware version: ");
    Serial.println(firmware_version);

    Serial.print("Protocol version: ");
    Serial.println(protocol_version);

    delay (100);
  }
}

void loop() {}