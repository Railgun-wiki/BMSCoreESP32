#pragma once

/* I2C - INA226 current/voltage sensor */
#define PIN_I2C_SDA         21
#define PIN_I2C_SCL         22
#define INA226_ADDRESS_7BIT 0x40

/* SPI - ST7789 LCD + DAC8562 (shared HSPI/SPI2, separate CS) */
#define PIN_LCD_MOSI        11
#define PIN_LCD_SCLK        12
#define PIN_LCD_CS          10
#define PIN_LCD_DC          46
#define PIN_LCD_RST         9
#define PIN_LCD_BLK         8
#define PIN_DAC_SYNC        14   /* DAC8562 chip select */

/* UART (reserved) */
#define PIN_UART_RX         18
#define PIN_UART_TX         17

/* Other */
#define PIN_RGB_LED         48
#define PIN_FLASH_BTN       0
