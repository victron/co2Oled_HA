# Instructions to install platformIO env on HA

## prepare env
### initial
```
python3 -m venv platformIO
. platformIO/bin/activate
pip3 install platformio
pio --version # check
```

### fetch repo
```
cd ~/ota_update_pio/
export PAT='.......'
git clone https://${PAT}@github.com/victron/co2Oled_HA.git
mv co2Oled_HA co2OledHAtemp
cd co2OledHAtemp/

#####################
# manualy create files 
# vi src/secrets.h
#   - set hostname
# vi src/secrets.sh
#   - set ip
####################

# check ip -- sudo cat /var/lib/NetworkManager/dnsmasq-wlp3s0.leases
# add it to /etc/NetworkManager/dnsmasq-shared.d/static-leases.conf

. ~/platformIO/bin/activate
pio pkg install
pio pkg list
```

```
pio run -t clean
pio run
```

## ota
### get ip
```
sudo cat /var/lib/NetworkManager/dnsmasq-wlp3s0.leases
```

### edit platformio.ini
```
upload_protocol = espota
upload_port = ${sysenv.DEVICE_IP}
upload_flags =
    --port=3232
    --auth=${sysenv.OTA_PASSWORD}
```

### OTA update
```
. ~/platformIO/bin/activate
cd ~/ota_update_pio/co2OledHAtemp
. src/secrets.sh

pio run -e ota --target upload
```