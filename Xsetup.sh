#!/bin/bash
set -x

# Determine original invoking user's home directory
ORIGINAL_USER=${SUDO_USER:-$(logname)}
USER_HOME=$(getent passwd $ORIGINAL_USER | cut -d: -f6)

# Install OpenSSL for C/C++ programs
sudo apt-get update
sudo apt-get install -y libssl-dev xutils-dev

# Install FTP Daemon and configure it
# Adjusted the path to the vsftpd.conf file
sudo apt-get install -y vsftpd
sudo cp $USER_HOME/MilanoWaterProject/misc/vsftpd.conf /etc/.
sudo service vsftpd restart

# Install JSON Lib, CMake, Mosquitto MQTT Service
sudo apt-get install -y libjson-c-dev 
sudo apt-get install -y cmake 
sudo apt-get install -y mosquitto mosquitto-clients

# Enable Mosquitto service and check its version
sudo systemctl enable mosquitto.service
mosquitto -v

# Install MQTT and update its config and application libraries
git clone https://github.com/eclipse/paho.mqtt.c.git
cd paho.mqtt.c
make
sudo make install
cd /etc/mosquitto
echo "listener 1883
allow_anonymous true" | sudo tee -a mosquitto.conf

# Install Python LIBs & Py Rainbird Project
git clone https://github.com/allenporter/pyrainbird.git
cd pyrainbird
pip install -r requirements_dev.txt --ignore-requires-python
pip install . --ignore-requires-python
pip install paho-mqtt
cd ../

# Create Directories for Project
mkdir -p FTP/files
chmod a-w FTP
mkdir MWPLogData

echo "Setup script has finished executing."
