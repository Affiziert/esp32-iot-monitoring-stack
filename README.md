# ESP32 IoT Monitoring Stack

The project uses battery-powered ESP32-C3 boards as transmitters to measure temperature, relative humidity, and battery voltage. The sensor data is transmitted wirelessly via ESP-NOW to an ESP32-C3 receiver and forwarded via USB serial to a Raspberry Pi. A Python-based data bridge processes the received data and stores it in an InfluxDB database running in a Docker Compose environment.

---

## Overview

The goal of this project is to implement a lightweight and modular IoT monitoring stack using readily available hardware and open-source software.
The current implementation consists of the following components:

- ESP32-C3 transmitter node with a single Li-Ion cell (3.7 V), TP4056 charging module and voltage divider for battery voltage measurement
- SHT31 module for temperature and relative humidity measurement
- ESP32-C3 receiver node
- ESP-NOW wireless communication
- Raspberry Pi 4 as an edge gateway
- Python data bridge
- InfluxDB 2.x database

The modular architecture allows individual components to be modified or extended independently.

---

## Architecture

<img width="2739" height="769" alt="ESP31_IoT_Monitoring_Stack_Architecture" src="https://github.com/user-attachments/assets/02fb3d43-3915-4a11-8638-959cc0b40d00" />

---

## Hardware Requirements

### Sensor Node

| Component | Quantity | Purpose |
|-----------|----------|---------|
| ESP32-C3 | 1 | Transmitter |
| SHT31 module | 1 | Temperature and relative humidity measurement |
| Li-Ion cell (3.7 V)| 1 | Power supply |
| 18650 battery holder  | 1 | Battery connection |
| TP4056 module | 1 | Li-Ion charging |
| Resistors | 2 | Voltage divider for battery voltage measurement |
| Capacitors | 1 | ADC input filtering |

### Receiver Node

| Component | Quantity | Purpose |
|-----------|----------|---------|
| ESP32-C3 | 1 | Receiver |
| USB cable | 1 | Serial connection to Raspberry Pi |

### Edge Gateway

| Component | Quantity | Purpose |
|-----------|----------|---------|
| Raspberry Pi 4 | 1 | Edge gateway |
| microSD card | 1 | Raspberry Pi OS and data |
| SSD (optional) | 1 | Additional storage / Docker data |
| USB cable (optional) | 1 | Connection to SSD |
---

## Software Requirements

### ESP32 Firmware

- Arduino IDE
- ESP32 Arduino Core 3.3.11
- Adafruit SHT31 library

### Raspberry Pi

- Raspberry Pi OS
- Python 3.x
- pyserial
- influxdb-client
- Docker Compose
  
### Database

- InfluxDB 2.x

---

## Setup

Several parameters can be configured directly in the firmware and data bridge.

### ESP32-C3 TX and RX

Hardware - Schaltplan/Widerstandswerte/Kondensatorwerte

You can configure:
- Firmware version
- Protocol version
- ESP-NOW Wi-Fi channel
- ESP-NOW transmission power
- Sensor ID
- Receiver MAC address
- Deep sleep interval
- Number of transmissions per cycle
- I2C pins and address for the sensor
- ADC pin for battery voltage
- Number of voltage measurements per transmission cycle (to calculate the average)

### Data Bridge

The following InfluxDB connection parameters must be configured before use:

```python
URL    = "http://localhost:8086"
TOKEN  = "YOUR_INFLUXDB_TOKEN"
ORG    = "YOUR_INFLUXDB_ORGANIZATION"
BUCKET = "YOUR_INFLUXDB_BUCKET"
```

---

## Components

### ESP32 Transmitter Node

The transmitter node collects environmental data and transmits a fixed-size payload of 8 bytes via ESP-NOW.
It periodically wakes from deep sleep, measures the connected sensors, transmits the data and returns to deep sleep.

### ESP32 Receiver Node

The receiver node continuously listens for ESP-NOW packets and converts the received data into a JSON object for transmission via USB serial.
The MAC address of the transmitter is obtained from the ESP-NOW reception information.

The JSON object contains:

- Sensor ID
- Transmitter MAC address
- Temperature
- Relative humidity
- Battery voltage

### Raspberry Pi Data Bridge

The Python data bridge runs on a Raspberry Pi and acts as the interface between the ESP32 receiver and InfluxDB.
Its main tasks are:

1. Read JSON data from the serial interface.
2. Parse and validate the received data.
3. Convert the data into an InfluxDB data point.
4. Write the data to the configured InfluxDB bucket.

### InfluxDB

InfluxDB is used as the time-series database for storing the sensor measurements.
The database is deployed using Docker Compose.

---

## Communication Protocol

The ESP-NOW communication uses a fixed-size binary payload of 8 bytes.

| Byte | Data Type | Description |
|------|-----------|-------------|
| 0 | `uint8_t` | Protocol Version |
| 1 | `uint8_t` | Sensor ID |
| 2–3 | `int16_t` | Temperature × 100 |
| 4–5 | `uint16_t` | Relative Humidity × 100 |
| 6–7 | `uint16_t` | Battery Voltage [mV] |

Temperature and relative humidity are transmitted as scaled integer values to reduce the payload size and avoid transmitting floating-point values.
The payload structure is explicitly packed and its size is checked at compile time.

---

## Debugging

Both ESP32 nodes provide optional serial debug output.

The debug output can be used to identify issues such as:

- ESP-NOW initialization failures
- Invalid payload size
- Protocol version mismatches
- Invalid sensor IDs
- ESP-NOW transmission failures
- Sensor initialization failures
- Firmware and protocol version information
  
Debug output can be enabled using:

```cpp
#define DEBUG_SERIAL 1
```

Normal operation:

```cpp
#define DEBUG_SERIAL 0
```

The serial interface currently operates at:

```text
115200 baud
```

### Versioning

#### Firmware Version

The firmware version identifies the software version installed on an ESP32.
It is independent of the communication protocol version and can be used to identify the exact firmware running on a device during debugging.

#### Protocol Version

The protocol version identifies the format of the data transmitted between ESP32 transmitter and receiver.
The receiver validates the protocol version before processing a packet.

A protocol version change is required if the structure or interpretation of the transmitted payload changes in a way that is incompatible with the existing receiver implementation.

---

### Planned

- [ ] Setup documentation
- [ ] Hardware wiring diagram
- [ ] InfluxDB Docker Compose setup
- [ ] Raspberry Pi setup documentation
- [ ] Protocol Version 2 for the integration of generic sensors
- [ ] Grafana Dashboard
- [ ] MQTT implementation between Receiver and Raspberry Pi

---
