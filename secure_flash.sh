#!/bin/bash

# --- KONFIGURACJA ---
PORT="/dev/ttyUSB0"
BAUD="921600"
KEY_FILE="reversed_chip_key.bin"

# Adresy (muszą być zgodne z menuconfig!)
BOOTLOADER_ADDR="0x1000"
PARTITION_ADDR="0x10000"
APP_ADDR="0x20000"

# Ścieżki do plików wejściowych (z buildu)
BOOT_BIN="build/bootloader/bootloader.bin"
PART_BIN="build/partition_table/partition-table.bin"
APP_BIN="build/wifi_station.bin"

# Ścieżki do plików wyjściowych (zaszyfrowanych)
BOOT_ENC="build/bootloader-enc-rev.bin"
PART_ENC="build/partition-table-enc-rev.bin"
APP_ENC="build/app-enc-rev.bin"

# --- PROCES ---

set -e # Przerwij skrypt w razie błędu

echo "1. Budowanie projektu..."
idf.py build

echo "2. Szyfrowanie binarek kluczem $KEY_FILE..."
espsecure.py encrypt_flash_data --keyfile "$KEY_FILE" --address "$BOOTLOADER_ADDR" -o "$BOOT_ENC" "$BOOT_BIN"
espsecure.py encrypt_flash_data --keyfile "$KEY_FILE" --address "$PARTITION_ADDR" -o "$PART_ENC" "$PART_BIN"
espsecure.py encrypt_flash_data --keyfile "$KEY_FILE" --address "$APP_ADDR" -o "$APP_ENC" "$APP_BIN"

echo "3. Flashowanie urządzenia na porcie $PORT..."
esptool.py --port "$PORT" --baud "$BAUD" write_flash \
    "$BOOTLOADER_ADDR" "$BOOT_ENC" \
    "$PARTITION_ADDR" "$PART_ENC" \
    "$APP_ADDR" "$APP_ENC"

echo "4. Uruchamianie monitora..."
idf.py monitor