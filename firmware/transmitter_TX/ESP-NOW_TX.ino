/*
 * ESP-NOW TX Node
 *
 * ArduinoIDE configuration:
 * - ESP32 Arduino Core: 3.3.11
 * - Board: ESP32C3 Dev Module
 * - CPU Frequency: 160 MHz
 * - USB CDC On Boot: Enabled
 * - Serial Baud Rate: 115200
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi_types.h>
#include "esp_sleep.h"

#include <Wire.h>
#include <Adafruit_SHT31.h>

// =================== Device Config ===================
// Sensor ID and protocol version
static constexpr uint8_t  sensor_ID          = 1;   // Make unique for each ESP device (0–255)
static constexpr uint8_t  protocol_version   = 1;   // Must match the receiver version

// Receiver MAC address 
uint8_t receiverMac[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// Wi-Fi settings
static const uint8_t wifi_channel = 6; // Wi-Fi channel
static constexpr wifi_power_t wifi_power = WIFI_POWER_15dBm; // Transmission power. Some ESP32-C3 boards show an unstable connection above transmission power WIFI_POWER_15dBm.

// Number of transmissions per transmission cycle
static const uint8_t trans_per_cycle = 1;

// I2C pin configuration for SHT31 sensor
static const int I2C_SDA = 3;
static const int I2C_SCL = 4;

// SHT31 I2C address (usually 0x44, alternatively 0x45)
static const uint8_t SHT31_addr = 0x44;

// ADC pin configuration for battery voltage measurement
static const int bat_vol_pin = 0;

// Number of voltage measurements per transmission cycle (to calculate the average)
static const uint8_t vol_meas_per_cycle = 16;

// Deep Sleep time in seconds
static constexpr uint32_t sleep_time_sec = 600;   // 10 minutes 

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

// =================== Runtime Variables ===================
// Sensor and battery measurements
int16_t  t_x100;
uint16_t rh_x100;
uint16_t bat_voltage;

// =================== Sensors ===================
// Create a sensor instance
Adafruit_SHT31 sht31 = Adafruit_SHT31();

// =================== Function Implementations ===================
// Initialize Deep Sleep Function
static void DeepSleep(){
  esp_now_deinit();   // Deinitialization for Deep Sleep
  WiFi.mode(WIFI_OFF);
  esp_sleep_enable_timer_wakeup((uint64_t)sleep_time_sec * 1000000ULL); // Converting seconds to microseconds
  esp_deep_sleep_start();
}

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
  delay (60000);
  }

  esp_now_peer_info_t peer = {}; // Configure the ESP-NOW peer for unicast communication
  memcpy(peer.peer_addr, receiverMac, 6); // Set receiver MAC address
  peer.channel = wifi_channel; // Set Wi-Fi channel
  peer.encrypt = false; // Set encryption
  
  // Add receiver as an ESP-NOW peer
  if (esp_now_add_peer(&peer) != ESP_OK) {
    if (DEBUG_SERIAL == 1){
      delay (100);
      Serial.println("ESP-NOW: Add peer failed");
    }
  delay (60000);
  }
}

// Initialize Sensor Function
static void initSht31() {
  Wire.begin(I2C_SDA, I2C_SCL); // I2C connection to the sensor
  Wire.setTimeOut(100); // Timeout after 100 ms

  if (!sht31.begin(SHT31_addr)) { // Initialize sensor
    if (DEBUG_SERIAL == 1){
      delay (100);
      Serial.println("SHT31: Not found");
    }
  delay (60000);
  }
}

// Sensor Measurement Function
static void readSht31Scaled(int16_t &t_x100, uint16_t &rh_x100) {
  float temp = sht31.readTemperature(); // Read temperature
  float humidity = sht31.readHumidity(); // Read humidity

  if (isnan(temp) || isnan(humidity)) { // Check if temp or humidity is NaN
    t_x100  = INT16_MIN; // Flag value
    rh_x100 = 0xFFFF; // Flag value
    return;
  }

  // Convert floating-point values to scaled integers to reduce payload size
  // Temperature and humidity are stored with a resolution of 0.01 °C / %
  long temp_scaled = lroundf(temp * 100.0f);
  long humidity_scaled = lroundf(humidity * 100.0f);
  
  // Limit values to the range of the target data types
  if (temp_scaled >  32767) temp_scaled =  32767;
  if (temp_scaled < -32768) temp_scaled = -32768;
  if (humidity_scaled < 0) humidity_scaled = 0;
  if (humidity_scaled > 65535) humidity_scaled = 65535;

  t_x100  = (int16_t)temp_scaled;
  rh_x100 = (uint16_t)humidity_scaled;
}

// Battery Voltage Measurement Function
static void readBattery(uint16_t &bat_voltage){
  uint32_t sum = 0;

  for (int i = 0; i < vol_meas_per_cycle; i++) { 
    sum += analogReadMilliVolts(bat_vol_pin);
    delay(2);
  }
  uint32_t adcVoltage_mV = sum / vol_meas_per_cycle; // Calculate average

  // Reconstruct battery voltage from the 1:1 voltage divider
  // Divider: 1 MΩ : 1 MΩ, with 100 nF filtering capacitor
  bat_voltage = adcVoltage_mV * 2;

  if (DEBUG_SERIAL == 1){
  Serial.print("Battery: ");
  Serial.print(bat_voltage);
  Serial.println(" mV");
  }
}

// Send Payload Function
static void send_payload(){
  Payload p; // Build payload with device information and sensor data
  p.version   = protocol_version;
  p.sensorId  = sensor_ID;

  readSht31Scaled(t_x100, rh_x100); // Get sensor data
  p.temp      = t_x100;
  p.humidity  = rh_x100;

  readBattery(bat_voltage); // Get battery voltage in mV
  p.battery   = bat_voltage;

  // Repeat transmission to increase the probability of successful reception
  // The receiver currently does not filter duplicate packets
  for (int i = 0; i < trans_per_cycle; i++) {
    esp_now_send(receiverMac, (uint8_t*)&p, sizeof(p));
    delay(50);
  }
}

// ESP-NOW callback handler
void onSent(const wifi_tx_info_t* info, esp_now_send_status_t status) {
  if (DEBUG_SERIAL == 1) {
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "TX: OK" : "TX: FAIL - Is the receiver ok?");
  }
}

void setup() {
  if (DEBUG_SERIAL == 1){   // Initialize serial interface for debug mode
    Serial.begin(115200);
    delay (1000);
  }

  initEspNow();  // Initialize ESP-NOW
  initSht31(); // Initialize SHT31 sensor

  if (DEBUG_SERIAL == 1){   // Register ESP-NOW transmission callback
    esp_now_register_send_cb(onSent);
    delay (100);
  }

  send_payload();   // Read sensor data and transmit payload

  // Initialization completed successfully
  if (DEBUG_SERIAL == 1){
    Serial.println("ESP-NOW TX Node initialized successfully.");
    delay (100);
  }

  DeepSleep();   // Enter deep sleep until next measurement cycle

}

void loop() {}