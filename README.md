# ESP32 IoT Monitoring Stack

The project uses battery-powered ESP32-C3 boards as transimitters to acquire temperature, relative humidity and battery voltage. The sensor data is transmitted wirelessly via ESP-NOW to an ESP32-C3 receiver and forwarded via USB serial to a Raspberry Pi. A Python-based data bridge processes the received data and stores it in an InfluxDB database.

## Overview

The goal of this project is to implement a lightweight and modular IoT monitoring stack using readily available hardware and open-source software.
The current implementation consists of the following components:

- ESP32-C3 transmitter node with a single Li-Ion cell, TP4056 charging module and voltage divider for battery voltage measurement
- SHT31 breakout board for temperature and relative humidity measurement
- ESP32-C3 receiver node
- ESP-NOW wireless communication
- Raspberry Pi 4 as an edge gateway
- Python data bridge
- InfluxDB 2.x database

The modular architecture allows individual components to be modified or extended independently.

## Architecture

The system is structured into several layers:

┌────────────────────────────────┐
│          Sensor Layer          │
│                                │
│  ESP32-C3 TX                   │
│  TP4056 module + Li-Ion cell + │
│  Voltage divider               │
│  SHT31 module (Temperature +   │
│  Relative humidity)            │
│                                │
└───────────────┬────────────────┘
                │
                │ ESP-NOW
                ▼
┌───────────────────────────────┐
│    Communication Layer        │
│                               │
│       ESP32-C3 RX             │
└───────────────┬───────────────┘
                │
                │ USB Serial / JSON
                ▼
┌───────────────────────────────┐
│      Edge Processing          │
│                               │
│  Raspberry Pi                 │
│  Python Data Bridge           │
└───────────────┬───────────────┘
                │
                │ InfluxDB API
                ▼
┌───────────────────────────────┐
│       Data Storage            │
│                               │
│        InfluxDB 2.x           │
└───────────────────────────────┘

## Components

### ESP32 Transmitter Node

The transmitter node collects environmental data and transmits it via ESP-NOW.
It periodically wakes from deep sleep, measures the connected sensors, transmits the data and returns to deep sleep.

### ESP32 Receiver Node

The receiver node continuously listens for ESP-NOW packets and forwards valid sensor data via USB serial.
The  data is transmitted as a JSON object containing:

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
The database can be deployed using Docker Compose.

## Software Requirements

### ESP32 Firmware

- Arduino IDE
- ESP32 Arduino Core 3.3.11
- ESP32-C3 Dev Module
- Adafruit SHT31 library

### Raspberry Pi

- Python 3.x
- pyserial
- influxdb-client
- Docker
- Docker Compose

### Database

- InfluxDB 2.x

## ESP-NOW Payload

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

## Serial Data Format

The ESP32 receiver forwards valid measurements as JSON via the USB serial interface.
The serial interface currently operates at:

```text
115200 baud
```

---

## Configuration

Several parameters can be configured directly in the firmware and data bridge.

### ESP32

Examples include:

- Sensor ID
- Firmware version
- Protocol version
- ESP-NOW Wi-Fi channel
- ESP-NOW transmission power
- Sensor configuration
- Deep sleep interval
- Number of transmissions per cycle

### Data Bridge

The following InfluxDB connection parameters must be configured before use:

```python
URL    = "http://localhost:8086"
TOKEN  = "YOUR_INFLUXDB_TOKEN"
ORG    = "YOUR_INFLUXDB_ORGANIZATION"
BUCKET = "YOUR_INFLUXDB_BUCKET"
```

## Versioning

### Firmware Version

The firmware version identifies the software version installed on an ESP32-C3.
It is independent of the communication protocol version and can be used to identify the exact firmware running on a device during debugging.

### Protocol Version

The protocol version identifies the format of the data transmitted between ESP32 transmitter and receiver.
The receiver validates the protocol version before processing a packet.

A protocol version change is required if the structure or interpretation of the transmitted payload changes in a way that is incompatible with the existing receiver implementation.

## Debugging

Both ESP32 nodes provide optional serial debug output.
Debug output can be enabled using:

```cpp
#define DEBUG_SERIAL 1
```

Normal operation:

```cpp
#define DEBUG_SERIAL 0
```

The debug output can be used to identify issues such as:

- ESP-NOW initialization failures
- Invalid payload size
- Protocol version mismatches
- Invalid sensor IDs
- ESP-NOW transmission failures
- Sensor initialization failures
- Firmware and protocol versions

## InfluxDB Data Model

The current implementation stores sensor data in InfluxDB using the following structure:

| Name | Type | Description | Unit |
|------|------|-------------|------|
| `MAC` | Tag | MAC address of the transmitter | – |
| `ID` | Field | Sensor ID | – |
| `Temperature` | Field | Temperature | °C |
| `Relative_Humidity` | Field | Relative humidity | % |
| `Battery` | Field | Battery voltage | mV |

The exact InfluxDB configuration is defined in the Docker Compose setup.

---

## Deployment

The system is intended to run with the ESP32 receiver connected to a Raspberry Pi via USB.

The Raspberry Pi runs:

- The Python data bridge
- The data bridge as a systemd service
- InfluxDB in a Docker container

Detailed setup instructions will be added to the `documentation/` directory.

---

### Planned

- [ ] Detailed hardware architecture diagram
- [ ] Hardware wiring diagram
- [ ] InfluxDB Docker Compose setup
- [ ] Raspberry Pi setup documentation

---
