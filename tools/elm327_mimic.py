#!/usr/bin/env python3

import serial
import logging
import random
from time import time
import argparse
import RPi.GPIO as GPIO

targets = []
byte_count = 1

logging.basicConfig(
        format='%(asctime)s.%(msecs)03d [%(targets)s %(targets_hex)s] %(message)s',
    level=logging.INFO,
    datefmt='%Y-%m-%d %H:%M:%S')

old_factory = logging.getLogRecordFactory()

def record_factory(*args, **kwargs):
    record = old_factory(*args, **kwargs)
    record.targets = ','.join([f'{targets[x]}' for x in range(byte_count)])
    record.targets_hex = ','.join([f'0x{targets[x]:02x}' for x in range(byte_count)])
    return record

logging.setLogRecordFactory(record_factory)

def move_data_towards_target(data, target):
    if data == target:
        target = random.randint(0,0xFF)

    elif data < target:
        data += 1
    else:
        data -= 1
    return data, target

def format_data(datas, bytes):
    s = ''
    for x in range(bytes):
        s += f'{datas[x]:02x} '
    return s

def main():
    parser = argparse.ArgumentParser(description="Simulate an ELM327 connected to a car")
    parser.add_argument("-p", "--port", default='/dev/ttyAMA0', help="Set serial port")
    parser.add_argument("-b", "--baudrate", type=int, default=38400, help="Set baudrate")
    parser.add_argument("-d", "--disconnected", action="store_true", help="Respond as if the car is disconnected")
    args = parser.parse_args()

    last_run_ms = 0
    curr_ms = 0
    wait_ms = 0

    max_data_bytes = 4
    extra_data_pids = {
        "0102": 2,
        "0110": 2,
    }

    global targets
    global byte_count
    for i in range(max_data_bytes):
        targets.append(random.randint(0, 0xFF))
    datas = targets[:]

    with serial.Serial(args.port,args.baudrate) as ser:
        # Toggle RESET pin on attiny
        GPIO.setwarnings(False)
        GPIO.setmode(GPIO.BCM)
        GPIO.setup(22, GPIO.OUT)
        GPIO.output(22, GPIO.LOW)
        GPIO.output(22, GPIO.HIGH)
        while True:
            s = ''
            while True:
                    s += ser.read().decode()
                    if s[-1] == '\r': break

            logging.info("< {}".format(s.replace('\r','\\r')))
            command = b''
            curr_ms = time() * 1000
            # We can safely skip configuration commands in our sim (AT*)
            if s.startswith('AT'):
                command = b'>'
            else:
                byte_count = extra_data_pids.get(s[0:4], 1)
                # Pick a random amount of time to wait between data changes
                if curr_ms - last_run_ms >= wait_ms:
                    last_run_ms = time() * 1000
                    wait_ms = random.randint(100,3000)
                    for x in range(byte_count):
                        datas[x], targets[x] = move_data_towards_target(datas[x],targets[x])

                if args.disconnected:
                    command = 'NO DATA \r\r>'.encode()
                else: 
                    command = '7E8 03 4{:s} {:s} {:s}\r\r>'.format(s[1], s[2:4], format_data(datas, byte_count)).encode()

            logging.info("> {}".format(command.decode().replace('\r','\\r')))
            # Reply to device with command    
            ser.write(command)

if __name__ == "__main__":
    main()

