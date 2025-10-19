#!/bin/bash
source ~/platformIO/bin/activate
project_name="${PWD##*/}"
source src/secrets.sh
git pull
echo '------------------------------------------------------------------------------'
git log -1
echo '------------------------------------------------------------------------------'

echo "------- start combile --- ${project_name}  ---------"
pio run -e ota -t upload
