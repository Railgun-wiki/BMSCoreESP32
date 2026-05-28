#include "mqtt_bridge.h"
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "bms_hw.h"
#include "pin_config.h"

static WiFiClient s_wifiClient;
static PubSubClient s_mqtt(s_wifiClient);
static unsigned long s_lastTelemetryMs = 0;
static const unsigned long TELEMETRY_INTERVAL_MS = 5000;
static const char* MQTT_TOPIC_TELEMETRY = "bms/telemetry";

static void mqttCallback(char* topic, uint8_t payload, unsigned int length) {
    // Reserved for command handling
}

void mqttBridgeInit(void) {
    LittleFS.begin(true);

    WiFiManager wm;
    wm.setConnectTimeout(30);
    wm.setConfigPortalTimeout(120);

    bool connected = wm.autoConnect("ESP32S3_BMS");
    if (!connected) {
        Serial.println("[MQTT] WiFi config timeout, running offline");
        return;
    }

    Serial.printf("[MQTT] WiFi connected: %s\n", WiFi.localIP().toString().c_str());

    // Default MQTT config (could be loaded from LittleFS JSON)
    s_mqtt.setServer("broker.emqx.io", 1883);
    s_mqtt.setCallback(mqttCallback);
}

void mqttBridgeLoop(void) {
    if (WiFi.status() != WL_CONNECTED) return;

    if (!s_mqtt.connected()) {
        if (s_mqtt.connect("ESP32S3_BMS")) {
            Serial.println("[MQTT] Connected");
        }
    }
    s_mqtt.loop();

    unsigned long now = millis();
    if (now - s_lastTelemetryMs >= TELEMETRY_INTERVAL_MS) {
        s_lastTelemetryMs = now;

        const bms_state_t* state = bms_hw_get_state();
        if (!state) return;

        JsonDocument doc;
        doc["v_mv"] = state->voltage_mV;
        doc["i_ma"] = state->current_mA;
        doc["soc_x10"] = state->soc_x10;
        doc["temp_x10"] = state->temperature_x10;
        doc["charge_active"] = state->charge_active;
        doc["discharge_active"] = state->discharge_active;

        char buf[256];
        serializeJson(doc, buf, sizeof(buf));
        s_mqtt.publish(MQTT_TOPIC_TELEMETRY, buf);
    }
}
