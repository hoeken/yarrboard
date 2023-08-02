#!/bin/sh

echo "Making release $1"

#openssl dgst -sign ~/Dropbox/misc/yarrboard.pem -keyform PEM -sha256 -out .pio/build/esp32dev/firmware.sign -binary .pio/build/esp32dev/firmware.bin
#cat .pio/build/esp32dev/firmware.sign .pio/build/esp32dev/firmware.bin > .pio/build/esp32dev/signed.bin
#cp ".pio/build/esp32dev/signed.bin" "releases/yarrboard-${1}.bin"

#copy our files over
cp ".pio/build/esp32dev/firmware.bin" "releases/yarrboard-${1}.bin"
cp ".pio/build/esp32dev/spiffs.bin" "releases/spiffs-${1}.bin"

#version bump
#sed -i "s|[0-9]*\.[0-9]*\.[0-9]*|$1|g" firmware.json

#checklist
echo ""
echo "Did you remember to?"
echo ""
echo "Bump the version # in platformio.ini"
echo "Bump the version # in index.html"
echo "Bump the version # in firmware.json"
echo "Build fresh firmware"
echo "Build fresh filesystem image"
echo "Add your changelist to firmware.json"