#pragma once

// Sensor ID (Make unique for each ESP device (0–255))
static constexpr uint8_t sensor_ID = 1;

// Receiver MAC address
uint8_t receiverMac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

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