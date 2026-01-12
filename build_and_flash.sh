#!/bin/bash
# Quick build and flash script for ESP32

set -e  # Exit on error

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== ESP32 Smart LED - Build & Flash Tool ===${NC}"

# Default port
PORT=${1:-/dev/ttyUSB0}

echo -e "${GREEN}Using port: $PORT${NC}"

# Check if ESP-IDF is sourced
if [ -z "$IDF_PATH" ]; then
    echo -e "${RED}ERROR: ESP-IDF not sourced!${NC}"
    echo "Please run: . \$HOME/esp/esp-idf/export.sh"
    exit 1
fi

cd /home/mikolaj/work/ESP32_smart_led/main_esp

echo -e "${BLUE}Step 1: Cleaning previous build...${NC}"
idf.py fullclean

echo -e "${BLUE}Step 2: Building firmware...${NC}"
idf.py build

echo -e "${BLUE}Step 3: Flashing to ESP32...${NC}"
idf.py -p $PORT flash

echo -e "${GREEN}✓ Firmware flashed successfully!${NC}"
echo -e "${BLUE}Starting serial monitor (Ctrl+] to exit)...${NC}"
echo ""

idf.py -p $PORT monitor
