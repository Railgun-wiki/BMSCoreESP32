# BMSCoreESP32

ESP32-S3 BMS (Battery Management System) integrated firmware.
Combines BMS-Core business logic, LVGL UI, MQTT telemetry, and SOC estimation.

## Build

PlatformIO 未加入 PATH,使用完整路径:

```bash
~/.platformio/penv/bin/pio run                    # compile
~/.platformio/penv/bin/pio run -t upload          # flash
~/.platformio/penv/bin/pio device monitor         # serial monitor
```

## Hardware

- **Board**: Freenove ESP32-S3 WROOM **N16R8** (16MB Flash, 8MB OPI PSRAM)
- **Board def**: `boards/freenove_esp32_s3_wroom_n16r8.json` (自定义,PlatformIO 仅有 N8R8)
- **Display**: 1.14" ST7789 IPS, 135x240, RGB565, SPI
- **Sensor**: INA226, I2C, 2mOhm shunt, 15A max
- **DAC**: DAC8562, dual 16-bit, SPI (shared bus with LCD)

## Architecture

三层架构,参照 BMS-Core 的分离方式:

```
┌─────────────────────────────────────────────┐
│  src/tasks/         FreeRTOS 任务编排        │
├─────────────────────────────────────────────┤
│  src/app/           业务逻辑 (本项目维护)     │
│  src/ui/            LVGL UI (submodule)      │
│  src/mqtt/          MQTT 遥测                │
│  src/soc/           SOC 估算                 │
├─────────────────────────────────────────────┤
│  src/bsp/           硬件驱动 (本项目维护)     │
│  middleware/         LVGL + FreeRTOS (submodule) │
└─────────────────────────────────────────────┘
```

- **App (业务逻辑)**: 传感器读取、DAC 控制、充放电逻辑 — 不依赖 UI
- **UI (lv_bms_view)**: LVGL 视图/控制器 — 不依赖业务逻辑
- **连接**: 通过 `bms_state_t` 共享状态 + `bms_hw` 硬件抽象接口

FreeRTOS multi-task, dual-core ESP32-S3:
- **Core 1**: LVGL display task (priority 5, 5ms period)
- **Core 0**: Sensor polling (priority 4, 200ms), MQTT bridge (priority 2)

## Directory Structure

```
BMSCoreESP32/
├── boards/                     自定义板定义 (N16R8)
├── Docs/                       项目文档
├── include/
│   ├── pin_config.h            GPIO 引脚定义
│   └── bms_config.h            系统常量
├── lib/                        PlatformIO 私有库
├── middleware/                  中间件 (git submodule)
│   ├── lvgl/                   LVGL v9.5 (从 release/v9.5 新建 dev-esp32/v9.5)
│   └── FreeRTOS/               FreeRTOS-Kernel
├── src/
│   ├── main.cpp                setup/loop + FreeRTOS task creation
│   ├── tasks/                  FreeRTOS task implementations
│   ├── bsp/                    硬件驱动 (BSP), 本项目维护
│   ├── app/                    业务逻辑, 本项目维护 (非 submodule)
│   │   ├── app.cpp             业务入口 (传感器读取, DAC控制, 充放电逻辑)
│   │   └── app.hpp
│   ├── ui/                     LVGL UI (git submodule: lv_bms_view)
│   │   ├── src/
│   │   │   ├── view/           LVGL 视图 (纯 C)
│   │   │   ├── controller/     UI 控制器 (纯 C)
│   │   │   ├── hw/             硬件抽象层桥接
│   │   │   ├── bms_state.h     共享状态结构体
│   │   │   └── bms_ui.h/cpp    UI 初始化入口
│   │   ├── fonts/              自定义 Montserrat 子集字体
│   │   └── lv_conf.h           LVGL 配置 (复制到项目根)
│   ├── mqtt/                   WiFi + MQTT 遥测
│   └── soc/                    SOC 估算 (OCV 查表)
├── scripts/                    权重导出等脚本
├── platformio.ini
├── lv_conf.h                   LVGL 配置 (项目根, LV_CONF_INCLUDE_SIMPLE)
└── CLAUDE.md
```

## Source Projects 关系

参照 BMS-Core 的引入方式:

```
BMS-Core 的做法:
├── App/Src/app.cpp          ← 业务逻辑 (直接写在项目中)
├── App/lvgl-bms-view/       ← UI 子模块 (git submodule: lv_bms_view)
├── BSP/                     ← BSP 驱动 (直接写在项目中)
└── Middlewares/lvgl/        ← LVGL (git submodule)

BMSCoreESP32 的做法:
├── src/app/app.cpp          ← 业务逻辑 (直接写在项目中, 移植自 BMS-Core)
├── src/ui/                  ← UI 子模块 (git submodule: lv_bms_view)
├── src/bsp/                 ← BSP 驱动 (直接写在项目中, 移植自 BMS-Core)
└── middleware/lvgl/         ← LVGL (git submodule)
```

| Module | 来源 | 类型 | 备注 |
|--------|------|------|------|
| src/app/ | BMS-Core/App/Src/ | 源码 | 移植业务逻辑 |
| src/ui/ | Railgun-wiki/lv_bms_view | submodule | UI 层,新建 ESP32 分支 |
| src/bsp/ | BMS-Core/BSP/ | 源码 | STM32 HAL → ESP32 Arduino |
| src/mqtt/ | UART-MQTT-Trans/ | 源码 | 从 .ino 提取 |
| src/soc/ | DL/scripts/ | 源码 | OCV 查表 |
| middleware/lvgl | lvgl/lvgl | submodule | 从 release/v9.5 新建 ESP32 分支 |
| middleware/FreeRTOS | FreeRTOS/FreeRTOS-Kernel | submodule | |

## Submodule 与分支策略

### LVGL
- 远程: `https://github.com/lvgl/lvgl.git`
- 基线: `release/v9.5`
- **新建分支**: `dev-esp32/v9.5` (从 release/v9.5 分出,针对 ESP32 优化)
- 不使用 `dev-f103/v9.5` 分支 (那是 F103 的大幅精简)

### lv_bms_view
- 远程: `https://github.com/Railgun-wiki/lv_bms_view.git`
- 基线: `master`
- **新建分支**: `dev-esp32` (从 master 分出,针对 ESP32 适配)
- 不使用 `opt-f103` / `local-f103-opt` 分支 (那是 F103 优化)

### FreeRTOS
- 远程: `https://github.com/FreeRTOS/FreeRTOS-Kernel.git`
- 分支: `main`

### Sparse Checkout (lv_bms_view)
`lv_bms_view` 仓库包含 PC 模拟器代码,嵌入式构建只需部分文件。
初始化 submodule 后必须执行 sparse checkout:

```bash
./scripts/setup-sparse-checkout.sh
```

检出: `src/view/`, `src/controller/`, `src/hw/`, `fonts/`, `bms_state.h`, `bms_ui.*`, `lv_conf.h`
排除: `src/main.c`, `src/hal/`, `src/sim/`, `src/freertos/`, `docs/`, `lvgl/`, `FreeRTOS/`

### 分支修改流程
```bash
# LVGL: 新建 ESP32 分支
cd middleware/lvgl
git checkout -b dev-esp32/v9.5 origin/release/v9.5
# ... ESP32 优化 ...
git push origin dev-esp32/v9.5

# lv_bms_view: 新建 ESP32 分支
cd src/ui
git checkout -b dev-esp32 origin/master
# ... ESP32 适配 ...
git push origin dev-esp32
```

## Git 工作流

### 提交规范
- **每次工作结束后必须提交 git**
- **每 3 次提交需更新文档** (CLAUDE.md, Docs/PROJECT_SPEC.md)
- 提交信息: `<type>: <description>` (feat/fix/refactor/docs/chore)

### 分支策略
- `main` — 稳定版本
- `feature/<描述>` — 新功能
- submodule 修改必须先创建分支

## Coding Conventions

详见 `Docs/PROJECT_SPEC.md` 第 2 节。要点:
- C++17 (BSP/App), C11 (UI view/controller)
- PascalCase 类名, camelCase 方法名
- BSP 驱动与 App 层通过指针注入解耦
- App 层与 UI 层通过 `bms_state_t` 解耦
