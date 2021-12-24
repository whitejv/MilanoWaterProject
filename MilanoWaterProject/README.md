# MilanoWaterProject

This project provides IOT control and monitoring of my home water system.

Intsall Git (if using Rapi-Lite)

--> sudo apt install git


Install OpenSSL for C/C++ programs

--> sudo apt-get install libssl-dev
--> sudo apt-get install xutils-dev

Install Code Directory

--> mkdir CodeDev

Install MQTT and Update Config File and Application Libraries

--> git clone https://github.com/eclipse/paho.mqtt.c.git
--> cd paho.mqtt.c
--> make
--> sudo make install
--> cd /etc/mosquitto
--> sudo vi mosquitto.conf
-->>> add: listener 1883
-->>> add: allow_anonymous true

Install Wiringpi (needed by Blynk)

--> wget https://project-downloads.drogon.net/wiringpi-latest.deb
--> sudo dpkg -i wiringpi-latest.deb
--> dpkg-deb -x wiringpi-latest.deb WiringPi/
--> gpio -v (look below means good install)
=== gpio version: 2.52
=== Copyright (c) 2012-2018 Gordon Henderson
=== This is free software with ABSOLUTELY NO WARRANTY.
=== For details type: gpio -warranty

Install Blynk

--> git clone https://github.com/blynkkk/blynk-library.git
--> cd blynk-library/linux
--> make clean all target=raspberry
--> make target=raspberry
=== clean build means succesful install

Install the Project

--> git clone https://github.com/whitejv/MilanoWaterProject.git
--> cd MilanoWaterProject
--> mkdir bin
--> cd TankSubscriber
--> make depend
--> make
--> cd ../blynk
--> make target=raspberry
