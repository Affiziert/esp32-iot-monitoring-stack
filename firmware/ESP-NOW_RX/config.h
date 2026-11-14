#pragma once

// Wi-Fi settings
static const uint8_t wifi_channel = 6; // Wi-Fi channel
static constexpr wifi_power_t wifi_power = WIFI_POWER_15dBm; // Transmission power. Some ESP32-C3 boards show an unstable connection above transmission power WIFI_POWER_15dBm.

// Maximum sensor ID value (because uint8_t range: 0-255)
static constexpr uint8_t max_sensor_ID = 255; // Reserved for future sensor ID validation

// Enable serial debug output
// 0 = normal operation
// 1 = debug mode
#define DEBUG_SERIAL 0 