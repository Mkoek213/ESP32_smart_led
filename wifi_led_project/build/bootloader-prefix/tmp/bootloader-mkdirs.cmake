# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/mikolaj/esp/esp-idf/components/bootloader/subproject"
  "/home/mikolaj/work/ESP32_smart_led/wifi_led_project/build/bootloader"
  "/home/mikolaj/work/ESP32_smart_led/wifi_led_project/build/bootloader-prefix"
  "/home/mikolaj/work/ESP32_smart_led/wifi_led_project/build/bootloader-prefix/tmp"
  "/home/mikolaj/work/ESP32_smart_led/wifi_led_project/build/bootloader-prefix/src/bootloader-stamp"
  "/home/mikolaj/work/ESP32_smart_led/wifi_led_project/build/bootloader-prefix/src"
  "/home/mikolaj/work/ESP32_smart_led/wifi_led_project/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/mikolaj/work/ESP32_smart_led/wifi_led_project/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/mikolaj/work/ESP32_smart_led/wifi_led_project/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
