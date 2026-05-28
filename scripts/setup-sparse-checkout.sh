#!/bin/bash
# Setup sparse-checkout for lv_bms_view submodule (src/ui/)
# Only checks out files needed for ESP32 embedded build
set -e

SUBMODULE_PATH="src/ui"

# Ensure submodule is initialized
git submodule update --init "$SUBMODULE_PATH"

cd "$SUBMODULE_PATH"

# Configure sparse-checkout (non-cone mode for individual file patterns)
git sparse-checkout init --no-cone
git sparse-checkout set --no-cone \
    '/src/view/' \
    '/src/controller/' \
    '/src/hw/' \
    '/fonts/' \
    '/src/bms_state.h' \
    '/src/bms_ui.h' \
    '/src/bms_ui.cpp' \
    '/lv_conf.h'

echo "Sparse-checkout configured for $SUBMODULE_PATH"
echo "Excluded: src/main.c, src/hal/, src/sim/, src/freertos/, docs/, lvgl/, FreeRTOS/"
