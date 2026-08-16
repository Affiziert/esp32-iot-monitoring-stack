#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// ================ Gerätekonfiguration ================

// Festlegung des Funkkanals
static const uint8_t WIFI_CHANNEL = 6;

// Festlegung Sendeleistung - Manche Boards haben bei einer Sendeleistung > WIFI_POWER_15dBm eine unstabile Verbindung
static constexpr wifi_power_t WIFI_POWER = WIFI_POWER_15dBm;

// Festlegung der Protokoll-Version
static constexpr uint8_t  protocol_version = 1;

// Festlegung der maximalen Anzahl an Sensor-IDs
static constexpr uint8_t MAX_SENSORS = 256;

// CPU-Takt kann von 160 MHz auf 80 MHz reduziert werden

#define DEBUG_SERIAL 0   // 1 = Debug, 0 = Feldbetrieb

// ================ Gerätekonfiguration-Ende ================

// Erstellung der Payload-Struktur (auf 8 Bytes festgelegt)
// uint nur für positive Werte, int für negative
struct __attribute__((packed)) Payload {
  uint8_t  version;
  uint8_t  sensorId;
  int16_t  temp;
  uint16_t humidity;
  uint16_t battery;
};

// =======================================================
void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  // Kontrolle, ob Nachrichtenlänge stimmt
  if (len != (int)sizeof(Payload)) return;

  // Kopieren der Empfangsdaten in Struct Payload
  Payload p;
  memcpy(&p, data, sizeof(p));

  // Versionsprüfung (muss an Sender angepasst werden)
  if (p.version != protocol_version) return;

  // ID-Prüfung (ID 0 ist ungültig)
  if (p.sensorId == 0) return; 

  // Pointer auf MAC-Adresse des Senders und Payload ausgebe als JSON
  const uint8_t* mac = info->src_addr;
  Serial.printf(
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
  // Beginn der seriellen Ausgabe und warten, bis diese aufgebaut ist
  Serial.begin(115200);
  while (!Serial) {}

  // Einstellung des WiFi-Modus, -Funkkanals und Sendeleistung
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(WIFI_CHANNEL);
  WiFi.setTxPower(WIFI_POWER);

  // Initialisierung ESP-Now
  if (esp_now_init() != ESP_OK) {
    delay (200);
    Serial.println("RX: ESP-NOW RX initialising failed");
    delay (200);
    ESP.restart();
  }

  // Callback registrieren (Eventbasiert)
  esp_now_register_recv_cb(onRecv);

  // Initialisierung erfolgreich
  delay (200);
  Serial.println("ESP-NOW RX ready");

}

void loop() {}

