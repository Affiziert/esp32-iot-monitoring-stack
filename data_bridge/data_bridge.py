"""
ESP-NOW Data Bridge

Reads JSON messages from the ESP-NOW receiver via USB serial
and stores sensor measurements in InfluxDB.

Expected JSON format:
{
    "id": 1,
    "mac": "AA:BB:CC:DD:EE:FF",
    "temp": 25.34,
    "rh": 52.78,
    "batt": 4014
}

Requirements:
- Python 3.x
- pyserial
- influxdb-client
- InfluxDB 2.x
"""

import json
import logging
import time

import serial
from serial import SerialException
from influxdb_client import InfluxDBClient, Point
from influxdb_client.client.write_api import SYNCHRONOUS

# =================== Configuration ===================
# Application version
APP_VERSION = "1.0.0"

# Serial connection to the ESP-NOW receiver
SERIAL_PORT = "/dev/serial/by-id/usb-Espressif_USB_JTAG_serial_debug_unit_XX:XX:XX:XX:XX:XX-if00" # Adjust to your port
BAUDRATE    = 115200

# InfluxDB connection settings
URL    = "http://localhost:8086"
TOKEN  = "YOUR_INFLUXDB_TOKEN"
ORG    = "YOUR_INFLUXDB_ORGANIZATION"
BUCKET = "YOUR_INFLUXDB_BUCKET"

# =================== Runtime Configuration ===================
# Required JSON fields expected from the ESP receiver
REQUIRED_FIELDS = {"mac", "id", "temp", "rh", "batt"}

# Maximum delay for automatic serial reconnect attempts
RECONNECT_DELAY_MAX = 60

# =================== Logging ===================
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
)
log = logging.getLogger(__name__)

# =================== Function Implementations ===================
# Build an InfluxDB point from a validated JSON payload
def build_point(data: dict) -> Point:
    missing = REQUIRED_FIELDS - data.keys()

    if missing:
        raise KeyError(f"Missing fields: {missing}")

    return (
        Point("Sensor_Data")
        .tag("MAC", data["mac"])
        .field("ID", data["id"])
        .field("Temperature", float(data["temp"]))
        .field("Relative_Humidity", float(data["rh"]))
        .field("Battery", float(data["batt"]))
    )

# Main application loop
# Reads JSON messages from the serial interface and stores them in InfluxDB
def main() -> None:

    log.info("ESP-NOW Data Bridge v%s started", APP_VERSION)
    log.info("Serial Port: %s @ %d baud", SERIAL_PORT, BAUDRATE)
    log.info("InfluxDB Bucket: %s", BUCKET)

    with InfluxDBClient(url=URL, token=TOKEN, org=ORG) as client:

        # Synchronous write mode for reliable data storage
        write_api = client.write_api(write_options=SYNCHRONOUS)

        # Initial reconnect delay in seconds
        delay = 1

        while True:
            try:
                log.info("Connecting to %s ...", SERIAL_PORT)

                # Open serial connection to the ESP-NOW receiver
                with serial.Serial(SERIAL_PORT, BAUDRATE, timeout=1) as ser:

                    log.info("Connection established.")
                    delay = 1

                    while True:
                        raw = ser.readline()

                        if not raw:
                            continue

                        try:
                            # Parse JSON message from receiver
                            data = json.loads(raw.decode("utf-8").strip())

                            # Convert JSON payload into InfluxDB format
                            point = build_point(data)

                            # Write measurement to InfluxDB
                            write_api.write(
                                bucket=BUCKET,
                                org=ORG,
                                record=point
                            )

                        except json.JSONDecodeError as e:
                            log.warning("Invalid JSON (%s): %r", e, raw)

                        except KeyError as e:
                            log.warning("Incomplete data: %s - Line: %r", e, raw)

                        except Exception as e:
                            log.error("Write error: %s", e)

            except SerialException as e:
                log.error(
                    "Serial error: %s - Retry in %ds",
                    e,
                    delay
                )

                time.sleep(delay)

                # Exponential backoff for reconnect attempts
                delay = min(delay * 2, RECONNECT_DELAY_MAX)


if __name__ == "__main__":
    main()
