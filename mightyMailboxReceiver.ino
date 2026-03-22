#include <RadioLib.h>
#include <SPI.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ===== Wi-Fi / MQTT =====
const char* WIFI_SSID = "WIFI_SSID";
const char* WIFI_PASSWORD = "WIFI_PASSWORD";
const char* MQTT_HOST = "MQTT_HOST";   // e.g. "192.168.1.50"
const uint16_t MQTT_PORT = 1883;
const char* MQTT_USER = "MQTT_USER";
const char* MQTT_PASSWORD = "MQTT_PASSWORD";

const char* MQTT_TOPIC_STATE = "mightymailbox/state";
const char* MQTT_TOPIC_AVAIL = "mightymailbox/availability";

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// ===== LoRa / SPI =====
SPIClass spi = SPIClass(FSPI);

SX1262 radio = new Module(
  10,  // NSS (CS)
  7,   // DIO1
  8,   // RESET
  9,   // BUSY
  spi
);

// ===== Last LoRa tracking =====
unsigned long lastValidLoraMillis = 0;
unsigned long lastMinutePublishMillis = 0;
int lastLoraMinutes = 0;
bool everReceivedValidLora = false;

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi connected, IP: ");
  Serial.println(WiFi.localIP());
}

void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting MQTT...");
    String clientId = "MightyMailboxRX-" + String((uint32_t)ESP.getEfuseMac(), HEX);

    bool ok = mqttClient.connect(
      clientId.c_str(),
      MQTT_USER,
      MQTT_PASSWORD,
      MQTT_TOPIC_AVAIL,
      0,
      true,
      "offline"
    );

    if (ok) {
      Serial.println("connected");
      mqttClient.publish(MQTT_TOPIC_AVAIL, "online", true);
    } else {
      Serial.print("failed, rc=");
      Serial.println(mqttClient.state());
      delay(2000);
    }
  }
}

bool parseMailboxPayload(
  const String& input,
  int& batteryMv,
  int& d1,
  int& d2
) {
  batteryMv = -1;
  d1 = -1;
  d2 = -1;

  int batPos = input.indexOf("BAT=");
  int d1Pos  = input.indexOf("D1=");
  int d2Pos  = input.indexOf("D2=");

  if (batPos < 0 || d1Pos < 0 || d2Pos < 0) {
    return false;
  }

  int batEnd = input.indexOf(',', batPos);
  int d1End  = input.indexOf(',', d1Pos);

  String batStr = (batEnd > 0) ? input.substring(batPos + 4, batEnd) : input.substring(batPos + 4);
  String d1Str  = (d1End > 0)  ? input.substring(d1Pos + 3, d1End)  : input.substring(d1Pos + 3);
  String d2Str  = input.substring(d2Pos + 3);

  batteryMv = batStr.toInt();
  d1 = d1Str.toInt();
  d2 = d2Str.toInt();

  return (batteryMv >= 0 && d1 >= 0 && d2 >= 0);
}

bool publishFullState(int batteryMv, int d1, int d2, float rssi, float snr, int lastLoraMin) {
  char json[192];
  snprintf(
    json,
    sizeof(json),
    "{\"battery_mv\":%d,\"d1\":%d,\"d2\":%d,\"rssi\":%.1f,\"snr\":%.2f,\"last_lora_min\":%d}",
    batteryMv, d1, d2, rssi, snr, lastLoraMin
  );

  bool ok = mqttClient.publish(MQTT_TOPIC_STATE, json, true);
  Serial.print("MQTT publish full state: ");
  Serial.println(ok ? "OK" : "FAIL");
  return ok;
}

bool publishLastLoraOnly(int lastLoraMin) {
  char json[64];
  snprintf(
    json,
    sizeof(json),
    "{\"last_lora_min\":%d}",
    lastLoraMin
  );

  bool ok = mqttClient.publish(MQTT_TOPIC_STATE, json, true);
  Serial.print("MQTT publish last_lora_min only: ");
  Serial.println(ok ? "OK" : "FAIL");
  return ok;
}

void setup() {
  Serial.begin(115200);
  delay(300);

  connectWiFi();
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setKeepAlive(90);
  mqttClient.setSocketTimeout(2);
  connectMQTT();

  spi.begin(
    12, // SCK
    13, // MISO
    11, // MOSI
    10  // SS/CS
  );

  Serial.print("Initializing SX1262... ");
  int state = radio.begin(868.0);
  Serial.println(state);

  if (state != RADIOLIB_ERR_NONE) {
    Serial.println("SX1262 init failed. Check wiring + SPI pins.");
    while (true) delay(1000);
  }

  radio.setDio2AsRfSwitch(true);
  radio.setSpreadingFactor(10);
  radio.setBandwidth(125.0);
  radio.setCodingRate(5);
  radio.setOutputPower(14);

  state = radio.startReceive();
  Serial.print("startReceive(): ");
  Serial.println(state);

  // Start the minute timer from boot
  lastMinutePublishMillis = millis();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (!mqttClient.connected()) {
    connectMQTT();
  }
  mqttClient.loop();

  String str;
  int state = radio.receive(str);

  if (state == RADIOLIB_ERR_NONE) {
    float rssi = radio.getRSSI();
    float snr = radio.getSNR();

    Serial.print("Received: ");
    Serial.println(str);
    Serial.print("RSSI: ");
    Serial.print(rssi);
    Serial.print(" dBm, SNR: ");
    Serial.print(snr);
    Serial.println(" dB");

    int batteryMv, d1, d2;
    if (parseMailboxPayload(str, batteryMv, d1, d2)) {
      everReceivedValidLora = true;
      lastValidLoraMillis = millis();
      lastMinutePublishMillis = millis();
      lastLoraMinutes = 0;

      publishFullState(batteryMv, d1, d2, rssi, snr, lastLoraMinutes);
    } else {
      Serial.println("Payload parse failed");
    }
  }

  // If no valid LoRa has been received for more than 1 minute,
  // increment lastLoraMinutes every minute and publish it.
  unsigned long now = millis();

  if (!everReceivedValidLora) {
    // Optional: count from boot even before first valid packet ever arrives
    if (now - lastMinutePublishMillis >= 60000UL) {
      lastMinutePublishMillis += 60000UL;
      lastLoraMinutes++;
      Serial.print("No valid LoRa yet, lastLoraMinutes = ");
      Serial.println(lastLoraMinutes);
      publishLastLoraOnly(lastLoraMinutes);
    }
  } else {
    if (now - lastValidLoraMillis >= 60000UL) {
      if (now - lastMinutePublishMillis >= 60000UL) {
        lastMinutePublishMillis += 60000UL;
        lastLoraMinutes++;
        Serial.print("Minutes since last valid LoRa = ");
        Serial.println(lastLoraMinutes);
        publishLastLoraOnly(lastLoraMinutes);
      }
    }
  }

  radio.startReceive();
}
