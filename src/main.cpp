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
static SPIClass  spiFSPI(FSPI);  // GP-SPI2: LCD 专用
static SPIClass  spiDAC(VSPI);   // GP-SPI3: DAC 专用

// --- BSP driver instances ---
static Ina226    ina226(&wireI2C, INA226_ADDRESS_7BIT);
static Dac8562   dac8562(&spiDAC, PIN_DAC_SYNC);
static St7789    lcd(&spiFSPI, PIN_LCD_CS, PIN_LCD_DC, PIN_LCD_RST, PIN_LCD_BLK);

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("[BMS] Starting...");

    // --- I2C ---
    wireI2C.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);

    // --- SPI: LCD on GP-SPI2 (FSPI), DAC on GP-SPI3 (VSPI) ---
    spiFSPI.begin(PIN_LCD_SCLK, -1, PIN_LCD_MOSI, PIN_LCD_CS);
    spiDAC.begin(PIN_DAC_SCLK, -1, PIN_DAC_MOSI, PIN_DAC_SYNC);

    // --- BSP Init ---
    if (!ina226.init(BMS_SHUNT_uOhm, BMS_MAX_CURRENT_mA)) {
        Serial.println("[BMS] INA226 init FAILED");
        // TODO: Set error state flag, blink LED to indicate sensor failure
    } else {
        Serial.println("[BMS] INA226 OK");
    }

    dac8562.init();
    // TODO: Check dac8562.init() return value and handle failure
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
    // TODO: Move WiFi to non-blocking task, display progress on LCD before blocking
    mqttBridgeInit();

    // --- Sensor task init ---
    taskSensorInit();

    // TODO: Initialize OneButton on PIN_FLASH_BTN for WiFiManager config reset (long press >3s)
    // TODO: Initialize WS2812 LED on PIN_RGB_LED for status indication
    // TODO: Configure ESP task watchdog (esp_task_wdt_config)
    // TODO: Initialize PSRAM and log available size (ESP.getPsramSize())

    // --- LVGL Input Device (placeholder — wire to GPIO buttons in Phase 2) ---
    {
        static lv_indev_t* s_keypad = lv_indev_create();
        lv_indev_set_type(s_keypad, LV_INDEV_TYPE_KEYPAD);
        lv_indev_set_read_cb(s_keypad, [](lv_indev_t* drv, lv_indev_data_t* data) {
            (void)drv;
            // TODO: Read actual GPIO buttons (PIN_FLASH_BTN, etc.)
            data->key = 0;
            data->state = LV_INDEV_STATE_RELEASED;
        });
    }

    // --- FreeRTOS Tasks ---
    xTaskCreatePinnedToCore(taskLvgl,   "lvgl",   8192, nullptr, 5, nullptr, 1);
    xTaskCreatePinnedToCore(taskSensor, "sensor", 4096, nullptr, 4, nullptr, 0);
    xTaskCreatePinnedToCore(taskMqtt,   "mqtt",   6144, nullptr, 2, nullptr, 0);

    Serial.println("[BMS] Tasks created, system running");
}

void loop() {
    vTaskDelay(portMAX_DELAY);
}
