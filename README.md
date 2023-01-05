# MilanoWaterProject

## This project provides IOT control and monitoring of my home water system.

## To Use Remote Shell to Install

- -> Open Terminal on Mac/Windows/Raspberry PI/Linus
- -> Login to Target:  ssh pi@raspi.local
- -> Password: raspberry


## Intsall Git (if using Rapi-Lite)

- -> sudo apt install git


## Install OpenSSL for C/C++ programs

- -> sudo apt-get install libssl-dev
- -> sudo apt-get install xutils-dev

## Install FTP Daemon

- -> sudo apt install vsftpd
- -> sudo nano /etc/vsftpd.conf
- ->>anonymous_enable=NO
- ->>local_enable=YES
- ->>write_enable=YES
- ->>local_umask=022
- ->>chroot_local_user=YES
- ->>user_sub_token=$USER
- ->>local_root=/home/$USER/FTP
- -> mkdir -p /home/<user>/FTP/files
- -> chmod a-w /home/<user>/FTP
- -> sudo service vsftpd restart

## Install CMake

- -> sudo apt-get install cmake
 
## Install Mosquitto MQTT Service

- -> sudo apt install -y mosquitto mosquitto-clients
- -> sudo systemctl enable mosquitto.service
- -> mosquitto -v

## Install MQTT and Update Config File and Application Libraries

- -> git clone https://github.com/eclipse/paho.mqtt.c.git
- -> cd paho.mqtt.c
- -> make
- -> sudo make install
- -> cd /etc/mosquitto
- -> sudo vi mosquitto.conf
- ->> press 'i' for insert mode
- ->>> add: listener 1883
- ->>> add: allow_anonymous true
- >> press esc
- ->> Shift+zz
![image](https://user-images.githubusercontent.com/41390348/167849852-2fd8cb29-3461-4562-9e7c-22be091cd4f3.png)

## Install Wiringpi (needed by Blynk)

#### New Method (works for 32bit and may work for 64bit)
- -> sudo apt install wiringpi
#### Alternative Method is to Clone
- ->git clone https://github.com/WiringPi/WiringPi.git
- ->cd WiringPi
- ->./build

#### Old Method (depricated)
- -> wget https://project-downloads.drogon.net/wiringpi-latest.deb
- -> sudo dpkg -i wiringpi-latest.deb
- -> dpkg-deb -x wiringpi-latest.deb WiringPi/

#### Verify Installed Correctly
- -> gpio -v (look below means good install)
=== gpio version: 2.52
=== Copyright (c) 2012-2018 Gordon Henderson
=== This is free software with ABSOLUTELY NO WARRANTY.
=== For details type: gpio -warranty

## Install Blynk

- -> git clone https://github.com/blynkkk/blynk-library.git
- -> cd blynk-library/linux
- -> make clean all target=raspberry
- -> make target=raspberry
=== clean build means succesful install

## Install the Project

- -> git clone https://github.com/whitejv/MilanoWaterProject.git
- -> cd MilanoWaterProject
- -> mkdir bin
### CMake Process
- -> cmake -S . (to rebuild the Root Makefile
- -> make clean
- -> make depend
- -> make
- -> make install
- -> cd bin
- -> ls -al (check the dates)
 
### Manual Build Process
- -> cd TankSubscriber
- -> make depend
- -> make
- -> cd TankMonitor
- -> make depend
- -> make
 - -> cd FlowMonitor
- -> make depend
- -> make
- -> cd ../Blynk
- -> make target=raspberry
- -> cd ../bin
- -> ls -al (check the dates)

## Add to Cron for Start on Reboot

- -> crontab -e
- -> add the following line to the bottom of the cron file: @reboot sleep 20 && bash /home/pi/MilanoWaterProject/h2o.sh
- -> cd MilanoWaterProject
- -> chmod +x h2o.sh
- -> reboot
