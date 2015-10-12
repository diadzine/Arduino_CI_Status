#! /usr/bin/env python3
import serial
import settings
import logging
import sys
import time
import signal
from jenkinsapi.jenkins import Jenkins, Requester
import requests


if getattr(settings, 'DEBUG', False):
    logging.basicConfig(filename='logfile', level=logging.DEBUG)
else:
    logging.basicConfig(filename='logfile', level=logging.ERROR)
logger = logging.getLogger('status')

servers = dict()


def signal_term_handler(signal, frame):
    logger.debug('SIGTERM: exitting...')
    sys.exit(0)

signal.signal(signal.SIGTERM, signal_term_handler)


def get_jenkins_instance(server):
    try:
        if server.get('baseurl', '').split('//')[1] in servers:
            return servers[server.get('baseurl', '').split('//')[1]]

        connection = Jenkins(
            server.get('baseurl', ''),
            requester=Requester(
                username=server.get('username', ''),
                password=server.get('pass_or_token', ''),
                **server.get('extra', None)
            )
        )
        servers[server.get('baseurl', '').split('//')[1]] = connection
        return connection
    except requests.HTTPError as hte:
        logger.exception(hte)
        logger.warn("Failed to connect to {}".format(server.get('url', '')))
        raise


def open_serial_connection():
    try:
        device = getattr(settings, 'DEVICE', None)
        ser = serial.Serial(device.get('PORT', ''), device.get('BAUDRATE', ''))
        logger.debug(ser)
    except serial.SerialException:
        logger.error(
            "Cannot open serial connection to {} at {}".format(
                device.get('PORT', ''),
                device.get('BAUDRATE', '')
            )
        )
        sys.exit(1)

    # Wait 1 sec while opening serial port
    time.sleep(1)

    return ser


def poll_projects(ser):
    projects = getattr(settings, 'PROJECTS', [])

    for project in projects:
        if project.get('active', False):
            server = project.get('server', None)
            connection = get_jenkins_instance(server)
            jobname = server.get('job', None)

            logger.debug(connection)

            if connection.has_job(jobname):
                job = connection.get_job(jobname)

                if not job.is_enabled():
                    cmd_string = '{} {}\r\n'.format(
                        "DISABLED",
                        project.get('id')
                    )
                else:
                    lb = job.get_last_build()

                    if lb.is_good():
                        cmd_string = '{} {}\r\n'.format(
                            "SUCCESS",
                            project.get('id')
                        )
                    else:
                        cmd_string = '{} {}\r\n'.format(
                            "FAILED",
                            project.get('id')
                        )
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
