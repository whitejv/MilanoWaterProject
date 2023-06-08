import argparse
import asyncio
import datetime
import inspect
import logging
import os
from typing import Any

import aiohttp
from pyrainbird import async_client
from pyrainbird.exceptions import RainbirdDeviceBusyException

import paho.mqtt.client as mqtt

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT broker")
    else:
        print(f"Connection to MQTT broker failed: {rc}")

def on_message(client, userdata, msg):
    if msg.payload.decode() == f"check{userdata['controller_id']}":
        userdata['check_flag'] = True

async def check_zones(mqtt_server, controller_id):
    userdata = {'check_flag': False, 'controller_id': controller_id}
    mqtt_client = mqtt.Client(userdata=userdata)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
    mqtt_client.username_pw_set("pi", "raspberry")  # set your MQTT username and password
    mqtt_client.connect(mqtt_server, 1883)  # set your MQTT server and port
    mqtt_client.subscribe("rainbird/command")
    mqtt_client.loop_start()

    rainbird_ips = ["192.168.1.80", "192.168.1.79"]  # replace with your Rainbird IP addresses
    rainbird_passwords = ["G0rainbird", "G0rainbird"]  # replace with your Rainbird passwords

    async with aiohttp.ClientSession() as session:
        controller = async_client.CreateController(
            session,
            rainbird_ips[controller_id - 1],
            rainbird_passwords[controller_id - 1]
        )

        while True:
            if userdata['check_flag']:
                try:
                    states = await controller.get_zone_states()
                except RainbirdDeviceBusyException:
                    print("Device is busy, waiting for 2 seconds before retrying.")
                    await asyncio.sleep(2)
                    continue
                
                print(f"States: {states}")  # debugging line
                mqtt_client.publish(f"rainbird/controller{controller_id}/active_zone", str(states))  
                userdata['check_flag'] = False

            await asyncio.sleep(1)

    mqtt_client.loop_stop()

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-P", "--production", action='store_true',
        help="Use the production MQTT server"
    )
    parser.add_argument(
        "-D", "--development", action='store_true',
        help="Use the development MQTT server"
    )
    parser.add_argument(
        "-C", "--controller",
        type=int,
        choices=[1, 2],
        required=True,
        help="Controller number (1 or 2)"
    )
    args = parser.parse_args()

    if args.production:
        mqtt_server = "192.168.1.250"  # replace with your production MQTT server
    elif args.development:
        mqtt_server = "192.168.1.249"  # replace with your development MQTT server
    else:
        print("You must specify either -P for production or -D for development.")
        exit(1)

    # To run the function
    asyncio.run(check_zones(mqtt_server, args.controller))
