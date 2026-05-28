#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <lvgl.h>

#include "pin_config.h"
#include "bms_config.h"

#include "ina226.hpp"
#include "st7789.hpp"
#include "dac8562.hpp"
#include "lv_port_disp.hpp"

#include "bms_hw.h"
#include "bms_ui.h"

#include "mqtt_bridge.h"
#include "tasks.h"

// --- Peripheral instances ---
static TwoWire   wireI2C(0);
static SPIClass  spiHSPI(HSPI);

// --- BSP driver instances ---
static Ina226    ina226(&wireI2C, INA226_ADDRESS_7BIT);
static Dac8562   dac8562(&spiHSPI, PIN_DAC_SYNC);
static St7789    lcd(&spiHSPI, PIN_LCD_CS, PIN_LCD_DC, PIN_LCD_RST, PIN_LCD_BLK);

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("[BMS] Starting...");

    // --- I2C ---
    wireI2C.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);

    // --- SPI (shared HSPI for LCD + DAC) ---
    spiHSPI.begin(PIN_LCD_SCLK, -1, PIN_LCD_MOSI, PIN_LCD_CS);
    spiHSPI.beginTransaction(SPISettings(40000000, MSBFIRST, SPI_MODE0));

    // --- BSP Init ---
    if (!ina226.init(BMS_SHUNT_uOhm, BMS_MAX_CURRENT_mA)) {
        Serial.println("[BMS] INA226 init FAILED");
    } else {
        Serial.println("[BMS] INA226 OK");
    }

    dac8562.init();
    Serial.println("[BMS] DAC8562 OK");

    lcd.init();
    lcd.setRotation(St7789::Rotation::Landscape);
    Serial.println("[BMS] ST7789 OK");

    // --- LVGL ---
    lv_init();
    lv_port_disp_init(&lcd);

    // --- BMS HW bind + UI init ---
    bms_hw_bind(&ina226, &dac8562);
    bms_ui_init();

    // --- WiFi + MQTT (blocking portal before task creation) ---
    mqttBridgeInit();

    // --- Sensor task init ---
    taskSensorInit();

    // --- FreeRTOS Tasks ---
    xTaskCreatePinnedToCore(taskLvgl,   "lvgl",   8192, nullptr, 5, nullptr, 1);
    xTaskCreatePinnedToCore(taskSensor, "sensor", 4096, nullptr, 4, nullptr, 0);
    xTaskCreatePinnedToCore(taskMqtt,   "mqtt",   6144, nullptr, 2, nullptr, 0);

    Serial.println("[BMS] Tasks created, system running");
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
