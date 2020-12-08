# Demo GUI-O application: Toggle counter

A demo application that demonstrates the principles behind implementing
a GUI-O IoT application using the `ESP8266 bridge`. The application
back-end is a python script that runs on a PC with an ESP8266 bridge
connected via the USB cable.

The demo application implements a display of the local time on the
backend system (the PC), and a toggle button along with the toggle
counter; it counts the number of toggles since the start of each session
(i.e., the front-end application initialization) as well as the number
of toggles since the back-end program was started.

This demonstrates principles behind communication with the front-end
via the ESP8266 bridge:

* the use of the GUI-O UI line protocol for creating and controlling
  the UI on the front-end side; the GUI-O commands are prefixed with `$`
  as per the bridge's pass-through protocol implementation
* the use of `!`-prefixed lines to communicate with the bridge itself
  (e.g., periodic `!PING` command to query the bridge's status)
* the reception and dicarding of lines that are not prefixed with either
  `!` or `$`, as those are auxiliary/debug messages from the bridge

On the GUI-O protocol level, this program demonstrates the following
concepts:

* the reception and processing of events from the front-end (i.e., the
  toggle events), and update of the UI in response to them
* updating of the UI in response to asynchronous events that originate
  within the backend itself (i.e., the update of local time display)


## Requirements

The program was developed and tested with python 3.9 on Fedora 33 linux
system.

The requirements are:

* `python` 3.7 or later
* `pyserial-asyncio`

NOTE: at the time of writing, `pyserial-asyncio` appears to be supported
only on linux systems
