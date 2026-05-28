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

static void mqttCallback(char* topic, unsigned char* payload, unsigned int length) {
    // TODO: Parse MQTT command messages on "bms/command" topic
    // TODO: Support commands: charge_on/off, discharge_on/off, set_voltage, set_current, reboot
}

void mqttBridgeInit(void) {
    LittleFS.begin(true);
    // TODO: Check LittleFS.begin() return value

    WiFiManager wm;
    wm.setConnectTimeout(30);
    wm.setConfigPortalTimeout(120);

    bool connected = wm.autoConnect("ESP32S3_BMS");
    if (!connected) {
        Serial.println("[MQTT] WiFi config timeout, running offline");
        return;
    }

    Serial.printf("[MQTT] WiFi connected: %s\n", WiFi.localIP().toString().c_str());

    // TODO: Load MQTT config from LittleFS JSON instead of hardcoded values
    // TODO: Add MQTT username/password authentication
    // TODO: Configure Last Will and Testament (LWT) for availability
    s_mqtt.setServer("broker.emqx.io", 1883);
    s_mqtt.setCallback(mqttCallback);
}

void mqttBridgeLoop(void) {
    if (WiFi.status() != WL_CONNECTED) return;
    // TODO: Implement WiFi reconnection logic with backoff

    if (!s_mqtt.connected()) {
        if (s_mqtt.connect("ESP32S3_BMS")) {
            // TODO: Add retry backoff (currently retries every 10ms)
            // TODO: Subscribe to "bms/command" topic after reconnect
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
        doc["p_mw"] = state->power_mW;
        doc["soc_x10"] = state->soc_x10;
        doc["temp_x10"] = state->temperature_x10;
        doc["charge_active"] = state->charge_active;
        doc["discharge_active"] = state->discharge_active;

        char buf[256];
        serializeJson(doc, buf, sizeof(buf));
        s_mqtt.publish(MQTT_TOPIC_TELEMETRY, buf);
    }
}
