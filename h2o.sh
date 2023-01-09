#!/bin/bash
export LD_LIBRARY_PATH=/usr/local/lib:/home/pi/lib:/home/pi/CodeDev/paho.mqtt.c-master/build/output:/home/pi/json-c/json-c-build
echo "### Start the Processes  ###"
echo "Starting Subscriber App"
sleep 10
nohup /home/pi/MilanoWaterProject/bin/subscriber > /dev/null 2>&1 &
echo "Done"
echo "Starting Monitor App"
sleep 10
nohup /home/pi/MilanoWaterProject/bin/monitor > /dev/null 2>&1 &
sleep 10
echo "Starting Flow Monitor"
nohup /home/pi/MilanoWaterProject/bin/flowmonitor > /dev/null 2>&1 &
echo "Done"
sleep 10
echo "Starting Blynk Interface"
nohup /home/pi/MilanoWaterProject/bin/blynk > /dev/null 2>&1 &
echo "Done"
sleep 30m
