# Arduino_CI_Status

This project allows to turn on 4 leds and change their color according to a status.
At the begining, it was designed to monitor jobs on Continous Integration server, in my case, Jenkins CI.

There are 2 software for this:
  * Arduino sketch, listening on serial port, managing the leds.
  * Python script, polling CI servers, writing status on serial port.

![Final product](/arduino_ci_status.jpg)

You can change anything in the code to deal with your expectations.
For example, replace Jenkins polling with Nagios polling or new email in mailbox.
