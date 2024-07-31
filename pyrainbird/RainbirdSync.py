import argparse
import asyncio
import datetime
import logging
import os
from typing import Any

import aiohttp
from pyrainbird import async_client
from pyrainbird.exceptions import RainbirdDeviceBusyException

import paho.mqtt.client as mqtt

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        logger.info("Connected to MQTT broker")
    else:
        logger.info(f"Connection to MQTT broker failed: {rc}")

def on_message(client, userdata, msg):
    message = msg.payload.decode()
    if message == f"check{userdata['controller_id']}":
        userdata['check_flag'] = True
    elif message == "STOP":
        userdata['stop_flag'] = True
    elif message == "ADVANCE":
        userdata['advance_flag'] = True

async def check_zones(mqtt_server, controller_id):
    userdata = {'check_flag': False, 'stop_flag': False, 'advance_flag': False, 'controller_id': controller_id}
    mqtt_client = mqtt.Client(userdata=userdata)
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
    mqtt_client.username_pw_set("pi", "raspberry")  # set your MQTT username and password
    mqtt_client.connect(mqtt_server, 1883)  # set your MQTT server and port
    mqtt_client.subscribe("mwp/command/rainbird/command")
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
                    logger.info("Device is busy, waiting for 2 seconds before retrying.")
                    await asyncio.sleep(2)
                    continue
                
                logger.info(f"States: {states}")  # debugging line
                mqtt_client.publish(f"mwp/response/rainbird/controller{controller_id}/active_zone", str(states))  
                userdata['check_flag'] = False

            if userdata['stop_flag']:
                try:
                    await controller.stop_irrigation()  # Assuming this method exists
                    logger.info("Irrigation stopped")
                    mqtt_client.publish(f"mwp/response/rainbird/controller{controller_id}/status", "Irrigation Stopped")
                except RainbirdDeviceBusyException:
                    logger.info("Device is busy, waiting for 2 seconds before retrying.")
                    await asyncio.sleep(2)
                    continue
                
                userdata['stop_flag'] = False

            if userdata['advance_flag']:
                try:
                    await controller.advance_zone(0)  # Adjust the parameter as needed
                    logger.info("Advanced to the next zone")
                    mqtt_client.publish(f"mwp/response/rainbird/controller{controller_id}/status", "Zone Advanced")
                except RainbirdDeviceBusyException:
                    logger.info("Device is busy, waiting for 2 seconds before retrying.")
                    await asyncio.sleep(2)
                    continue
                
                userdata['advance_flag'] = False

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
    parser.add_argument(
        "-v", "--verbose", action='store_true',
        help="Enable verbose mode to print output to the console as well as to the file"
    )
    args = parser.parse_args()

    # Create a logger
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.INFO)

    # Create a file handler
    handler = logging.FileHandler('output.log')
    handler.setLevel(logging.INFO)

    # Create a logging format
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    handler.setFormatter(formatter)

    # Add the handler to the logger
    logger.addHandler(handler)

    # Check if verbose mode is enabled, if yes, print to console as well
    if args.verbose:
        consoleHandler = logging.StreamHandler()
        consoleHandler.setFormatter(formatter)
        logger.addHandler(consoleHandler)

    if args.production:
        mqtt_server = "192.168.1.250"  # replace with your production MQTT server
    elif args.development:
        mqtt_server = "192.168.1.249"  # replace with your development MQTT server
    else:
        logger.info("You must specify either -P for production or -D for development.")
        exit(1)

    # To run the function
    asyncio.run(check_zones(mqtt_server, args.controller))
