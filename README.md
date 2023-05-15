# MilanoWaterProject

## This project provides IOT control and monitoring of my home water system.

## Compute Module 4 Installation 
### On the CM4 (see Jeff Geerling How to flash Raspberry Pi OS onto the Compute Module 4 eMMC with usbboot)
- -> Remove Cover, Place Jumper, Plug in USB to LINUX System, Power On

### On a Linux System (RPI or Other)
- ->sudo apt install libusb-1.0-0-dev
- ->sudo git clone --depth=1 https://github.com/raspberrypi/usbboot
- ->cd usbboot
- ->sudo make
- ->sudo ./rpiboot
- ->{may take a few minutes but CM4 will eventually appear as an external drive.}
- ->Use the RPI Loader to Load Preffered OS
- ->Once OS is loaded then boot volume will appear
- ->Modify config.txt to include the line dtoverlay=dwc2,dr_mode=host to enable USB

### Back on the CM4

- ->Remove Jumper
- ->Reboot


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

## Install JSON Lib

- -> sudo apt install libjson-c-dev

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
- -> sudo nano mosquitto.conf
- ->>> add: listener 1883
- ->>> add: allow_anonymous true

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
- -> mkdir MWPLogData
- -> cd MilanoWaterProject
- -> mkdir /home/pi/MWPLogData (Create this on first time install on a new system)
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
 
 ## Configuring VS Code for Debug
- -> C++ extension for VS Code. Install the C/C++ extension by searching for 'c++' in the Extensions view (⇧⌘X).
- -> CMake Tools extension for VS Code. Install the CMake Tools extension by searching for 'CMake tools' in the Extensions view (⇧⌘X).
- -> sudo apt-get install build-essential gdb (Installs GDB Debugger if not already installed)
 
### Select a kit
- -> Before you can use the CMake Tools extension to build a project, you need to configure it to know about the compilers on your system. Do that by    scanning for 'kits'. A kit represents a toolchain, which is the compiler, linker, and other tools used to build your project. To scan for kits:

- -> Open the Command Palette (⇧⌘P)
- -> run CMake: Select a Kit. The extension will automatically scan for kits on your computer and create a list of compilers found on your system.
 
### Select a variant
- -> A variant contains instructions for how to build your project. By default, the CMake Tools extension provides four variants, each corresponding to a default build type: Debug, Release, MinRelSize, and RelWithDebInfo. These options do the following:

- -> Debug: disables optimizations and includes debug info. Release : Includes optimizations but no debug info. MinRelSize : Optimizes for size. No debug info. RelWithDebInfo : Optimizes for speed and includes debug info
 
### CMake: Configure
- -> Now that you've selected a kit and a variant, open the Command Palette (⇧⌘P)
- -> run the CMake: Configure command to configure your project. This generates build files in the project's build folder using the kit and variant you selected.

### Build Program
- -> After configuring your project, you're ready to build. Open the Command Palette (⇧⌘P)
- -> run the CMake: Build command, or select the Build button from the Status bar.
- -> You can select which targets you'd like to build by selecting CMake: Set Build Target from the Command Palette. By default, CMake Tools builds all targets. The selected target will appear in the Status bar next to the Build button.

### Debug Program
- -> To run and debug your project, open *.c and put a breakpoint in, then open the Command Palette (⇧⌘P)
- -> run CMake: Debug. The debugger will stop on the std::cout line:
