#!/bin/bash
export LD_LIBRARY_PATH=/usr/local/lib:/home/pi/lib:/home/pi/CodeDev/paho.mqtt.c-master/build/output:/home/pi/json-c/json-c-build
echo "### Start the Processes  ###"
echo "Starting Tank Monitor App"
sleep 10
nohup /home/pi/MilanoWaterProject/bin/tankmonitor -P > /dev/null 2>&1 &
echo "Done"
echo "Starting Well Monitor App"
sleep 10
nohup /home/pi/MilanoWaterProject/bin/wellmonitor -P > /dev/null 2>&1 &
echo "Done"
sleep 10
nohup /home/pi/MilanoWaterProject/bin/well3monitor -P > /dev/null 2>&1 &
echo "Done"
sleep 10
echo "Starting Irrigation Monitor App"
nohup /home/pi/MilanoWaterProject/bin/irrigationmonitor -P > /dev/null 2>&1 &
echo "Done"
sleep 10
echo "Starting House Monitor App"
nohup /home/pi/MilanoWaterProject/bin/housemonitor -P > /dev/null 2>&1 &
echo "Done"
sleep 10
echo "Starting Data Monitor App"
nohup /home/pi/MilanoWaterProject/bin/monitor -P > /dev/null 2>&1 &
sleep 10
echo "Starting Blynk Interface App"
nohup /home/pi/MilanoWaterProject/bin/blynkWater -P > /dev/null 2>&1 &
echo "Done"
echo "Starting RainbirdSync for Controller 1"
sleep 10
nohup python3 /home/pi/MilanoWaterProject/pyrainbird/RainbirdSync.py -P -C 1 > /dev/null 2>&1 &
echo "Done"
sleep 10
echo "Starting RainbirdSync for Controller 2"
sleep 10
nohup python3 /home/pi/MilanoWaterProject/pyrainbird/RainbirdSync.py -P -C 2 > /dev/null 2>&1 &
echo "Done"
sleep 30m
