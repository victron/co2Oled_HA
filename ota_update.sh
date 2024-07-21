arduino-cli lib install "Adafruit SSD1306"
arduino-cli lib install "Sensirion I2C SCD4x"

git pull
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 --build-cache-path ~/ota_update/build_cache --output-dir ~/ota_update/build ./*.ino
