# Phase 1 Goal Prompt

## 背景
将 4 个项目整合到 Esp32/BMSCoreESP32/ (Freenove ESP32-S3 WROOM N16R8)。
详细计划见: Docs/PROJECT_SPEC.md, CLAUDE.md

## 当前状态
- 项目是空的 PlatformIO Arduino 模板 (仅 src/main.cpp)
- 文档和规范已就绪 (CLAUDE.md, Docs/PROJECT_SPEC.md, boards/, scripts/)
- pio 未加入 PATH: ~/.platformio/penv/bin/pio

## 本次目标: Phase 1 基础框架
按顺序完成以下步骤:

### Step 1: 初始化 submodule
1. LVGL submodule → middleware/lvgl/
   - 远程: https://github.com/lvgl/lvgl.git
   - 从 origin/release/v9.5 新建本地分支 dev-esp32/v9.5
2. FreeRTOS submodule → middleware/FreeRTOS/
   - 远程: https://github.com/FreeRTOS/FreeRTOS-Kernel.git
   - 使用 main 分支
3. lv_bms_view submodule → src/ui/
   - 远程: https://github.com/Railgun-wiki/lv_bms_view.git
   - 从 origin/master 新建本地分支 dev-esp32
   - 执行 sparse checkout: ./scripts/setup-sparse-checkout.sh

### Step 2: platformio.ini
- board = freenove_esp32_s3_wroom_n16r8 (使用 boards/ 下的自定义板定义)
- framework = arduino
- board_build.filesystem = littlefs
- lib_deps: lvgl, ArduinoJson@^7, OneButton@^2, PubSubClient@^2
- build_flags: -DBOARD_HAS_PSRAM, -DLV_CONF_INCLUDE_SIMPLE, -DBMS_ESP32, -std=gnu++17
- build_src_filter: 包含 src/tasks/, src/bsp/, src/app/, src/ui/src/, src/mqtt/, src/soc/, fonts/

### Step 3: lv_conf.h
- 从 src/ui/lv_conf.h 复制到项目根目录
- 修改: LV_USE_OS=LV_OS_FREERTOS, LV_MEM_SIZE=32768, 删除 BMS_SIM 条件块, SDL=0

### Step 4: include/pin_config.h + include/bms_config.h
- I2C: SDA=21, SCL=22, INA226 addr=0x40 (7-bit)
- SPI (共享 HSPI): MOSI=11, SCLK=12, LCD_CS=10, LCD_DC=46, LCD_RST=9, LCD_BLK=8, DAC_SYNC=14
- UART: RX=18, TX=17
- RGB_LED=48, FLASH_BTN=0
- bms_config: shunt=2000uOhm, max_current=15000mA

### Step 5: BSP 驱动移植 (src/bsp/)
从 BMS-Core/BSP/ 移植,STM32 HAL → ESP32 Arduino:
- ina226.hpp/cpp: I2C_HandleTypeDef* → TwoWire*, HAL_I2C_Mem_Read → Wire API, 8-bit→7-bit地址
- st7789.hpp/cpp: SPI_HandleTypeDef* → SPIClass*, GPIO_TypeDef*→int, HAL_SPI_Transmit→spi->transfer
- dac8562.hpp/cpp: 同 st7789, 共享 SPI2 + 独立 CS
- lv_port_disp.hpp/cpp: 双缓冲, St7789* 适配

### Step 6: App 业务逻辑 (src/app/)
- 从 BMS-Core/App/Src/ 复制 app.cpp, app.hpp
- 适配 ESP32: HAL_Delay→delay(), 外设句柄改为 Arduino 类型

### Step 7: UI 层 (src/ui/) — 已通过 submodule + sparse checkout 就绪
- view/, controller/, hw/, fonts/ 已检出
- bms_hw.cpp 需适配: 构造参数改为 ESP32 驱动类型

### Step 8: SOC 模块 (src/soc/)
- ocv_soc.h: 纯 C, 13 点 LG HG2 OCV-SOC 查表, 整数线性插值

### Step 9: main.cpp
- 创建 I2C/SPI 外设实例
- 初始化 BSP 驱动 + bms_hw_bind() + bms_ui_init()
- SmartConfig 初始化 (非阻塞式状态机等待)
- 创建 FreeRTOS 任务: task_lvgl (Core1), task_sensor (Core0), task_mqtt (Core0)
- loop() 空 (vTaskDelay)

### Step 10: tasks/ 实现
- task_lvgl.cpp: lv_timer_handler() 循环
- task_sensor.cpp: INA226 读取 + SOC 查表 + 写入 bms_state_t (mutex 保护)
- task_mqtt.cpp: WiFi/MQTT 维护 + BMS 遥测上报 (JSON to bms/telemetry)

## 关键约束
- C++17 (BSP/App/tasks), C11 (UI view/controller)
- PascalCase 类名, camelCase 方法名, m_ 私有成员前缀
- BSP 与 App 通过指针注入解耦, App 与 UI 通过 bms_state_t 解耦
- UI 层零硬件依赖,可在 PC 模拟器复用
- GPIO 35-37 被 OPI PSRAM 占用
- DAC8562 与 ST7789 共享 SPI2, CS 隔离
- 每次工作结束提交 git, 每 3 次提交更新文档
