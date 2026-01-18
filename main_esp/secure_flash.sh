#!/bin/bash
set -e # Przerwij przy błędzie

# --- KONFIGURACJA ---
PORT="/dev/ttyUSB0"
BAUD="921600"
KEY_FILE="reversed_chip_key.bin"

# Adresy - muszą być identyczne jak w menuconfig!
BOOT_ADDR="0x1000"
PART_ADDR="0x10000"  # <--- Sprawdź czy tu jest 0x10000
APP_ADDR="0x20000"

# Pliki
BOOT_BIN="build/bootloader/bootloader.bin"
PART_BIN="build/partition_table/partition-table.bin"
APP_BIN="build/wifi_station.bin"

echo "1. Szyfrowanie..."
espsecure.py encrypt_flash_data --keyfile "$KEY_FILE" --address "$BOOT_ADDR" -o build/boot-enc.bin "$BOOT_BIN"
espsecure.py encrypt_flash_data --keyfile "$KEY_FILE" --address "$PART_ADDR" -o build/part-enc.bin "$PART_BIN"
espsecure.py encrypt_flash_data --keyfile "$KEY_FILE" --address "$APP_ADDR" -o build/app-enc.bin "$APP_BIN"

echo "2. Flashowanie..."
esptool.py --port "$PORT" --baud "$BAUD" write_flash \
    "$BOOT_ADDR" build/boot-enc.bin \
    "$PART_ADDR" build/part-enc.bin \
    "$APP_ADDR" build/app-enc.bin

echo "3. Gotowe! Startuję monitor..."
idf.py monitor