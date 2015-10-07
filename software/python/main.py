#! /usr/bin/env python3
import serial
import settings
import logging
import sys
import time
import signal
import random


logging.basicConfig(filename='logfile',level=logging.DEBUG)
logger = logging.getLogger('status')


def signal_term_handler(signal, frame):
    logger.debug('SIGTERM: exitting...')
    sys.exit(0)

signal.signal(signal.SIGTERM, signal_term_handler)



def open_serial_connection():
    try:
        device = getattr(settings, 'DEVICE', None)
        ser = serial.Serial(device.get('PORT', ''), device.get('BAUDRATE', ''))
        logger.debug(ser)
    except serial.SerialException as se:
        logger.error(
            "Cannot open serial connection to {} at {}".format(
                device.get('PORT', ''),
                device.get('BAUDRATE', '')
            )
        )
        sys.exit(1)
    return ser


def poll_projects(ser):
    projects = getattr(settings, 'PROJECTS', [])

    for project in projects:
        if project.get('active', False):
            cmd = random.choice(['OK', 'WARNING', 'ERROR', 'UNKNOWN'])
            cmd_string = '{} {}\r\n'.format(cmd, project.get('id'))
        else:
            cmd_string = 'OFF {}\r\n'.format(project.get('id'))

        logger.debug(cmd_string)
        ser.write(bytes(cmd_string.encode()))


if __name__ == '__main__':
    try:
        ser = open_serial_connection()
        logger.debug(ser)
        while True:
            poll_projects(ser)
            time.sleep(getattr(settings, 'POLL_INTERVAL', 1))
    except KeyboardInterrupt:
        if ser and ser.isOpen():
            ser.close()
        logger.debug('Ctrl + c: exitting...')
        sys.exit(0)
