#!/bin/bash
#######################################
# file for NON pio
#######################################


# arduino-cli lib install "Adafruit SSD1306"
# arduino-cli lib install "Sensirion I2C SCD4x"

project_name='co2Oled_HA'
. ./secrets.sh
espota='/home/vic/.arduino15/packages/esp8266/hardware/esp8266/3.1.2/tools/espota.py'
build_cache='~/ota_update/build_cache'
builds_dir='~/ota_update/build'

git pull
git log -1
echo "------- start combile ---------"
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 --build-cache-path ${build_cache} --output-dir ${builds_dir} ./${project_name}.ino
if [ $? -ne 0 ]; then
  echo "Команда завершилась з помилкою. Завершення скрипта."
  exit 1
fi
python3 ${espota} --ip ${DEVICE_IP} --auth=${OTA_PASSWORD} --file ${builds_dir}/${project_name}.ino.bin
