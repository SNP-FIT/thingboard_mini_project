#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#define ACCESS_TOKEN "9e1ylY3ArjQ282ijchJP"

#include "DHT.h"
#define DHTPIN 26
#define DHTTYPE DHT11

float targetTemperature = 25;
float targetHumidity = 60;
bool autoMode = true;

const char* ssid = "BPSQUARE";
const char* password = "WhoAreYou?";

const char* mqtt_server = "mqtt.thingsboard.cloud";

WiFiClient espClient;
PubSubClient client(espClient);

DHT dht(DHTPIN, DHTTYPE);

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println(topic);
  JsonDocument doc;
  if (deserializeJson(doc, payload, length)) {
    return;
  }

  String t = topic;
  if (t == "v1/devices/me/attributes") {
    if (!doc["targetTemperature"].isNull()) {
      targetTemperature = doc["targetTemperature"];
    }

    if (!doc["targetHumidity"].isNull()) {
      targetHumidity = doc["targetHumidity"];
    }

    if (!doc["autoMode"].isNull()) {
      autoMode = doc["autoMode"];
    }
  } else if (t.startsWith("v1/devices/me/attributes/response/")) {
    if (!doc["shared"].isNull()) {
      if (!doc["shared"]["targetHumidity"].isNull()) {
        targetHumidity = doc["shared"]["targetHumidity"];
      }
      if (!doc["shared"]["targetTemperature"].isNull()) {
        targetTemperature = doc["shared"]["targetTemperature"];
      }
      if (!doc["shared"]["autoMode"].isNull()) {
        autoMode = doc["shared"]["autoMode"];
      }
    }
  }
}

unsigned long lastRead = 0;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  dht.begin();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Wifi Connected");

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  if (client.connect("ESP32_Test", ACCESS_TOKEN, "")) {
    Serial.println("Connected to Broker");
    JsonDocument doc;
    doc["fwVersion"] = "1.0.0";
    doc["model"] = "EvapGreenhouseController";
    doc["ip"] = WiFi.localIP().toString();
    char buf[160];
    serializeJson(doc, buf);
    Serial.println(buf);
    client.publish("v1/devices/me/attributes", buf);
    client.subscribe("v1/devices/me/attributes");
    client.subscribe("v1/devices/me/attributes/response/+");
    JsonDocument doc2;
    doc2["sharedKeys"] = "targetTemperature,targetHumidity,autoMode";
    char buf2[160];
    serializeJson(doc2, buf2);
    client.publish("v1/devices/me/attributes/request/1", buf2);
  } else {
    Serial.println("Failed");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if (millis() - lastRead > 5000) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();  // {"temperature":25, "humidity":30}
    // Serial.println(t);
    // Serial.println(h);
    JsonDocument doc;
    doc["temperature"] = t;
    doc["humidity"] = h;

    char buf[160];
    serializeJson(doc, buf);
    client.publish("v1/devices/me/telemetry", buf);
    // Serial.println(buf);
    Serial.print("targetTemperature = ");
    Serial.print(targetTemperature);
    Serial.print("  targetHumidity = ");
    Serial.print(targetHumidity);
    Serial.print("  autoMode = ");
    Serial.println(autoMode);
    lastRead = millis();
  }
  client.loop();
}
