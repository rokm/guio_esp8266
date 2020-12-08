# ESP8266 GUI-O IoT communication bridge

This project implements an embedded communication bridge for
[GUI-O IoT applications](https://www.gui-o.com). The bridge is
implemented as a stand-alone hardware module based on the
[ESP8266](https://www.espressif.com/en/products/socs/esp8266), and
provides support for the main aspects of communication between the
GUI-O application front-end and back-end:

* pairing with the front-end device
* persistent storage and retrieval of pairing data for use on subsequent
  device restarts
* bi-directional communication channel with the front-end device via
  WiFi and MQTT broker:
    * pass-through protocol for the GUI-O UI protocol messages
* bi-directional communication channel with the back-end device via
  serial (UART) connection:
    * pass-through protocol for the GUI-O UI protocol messages
    * command & control protocol for controlling the bridge from the
      back-end device
    * emission of diagnostic messages to the back-end device (debug mode)

The front-end device is a smart-phone running the
[GUI-O smart-phone application](https://www.gui-o.com)
that provides universal/programmable UI. The back-end device is an
arbitrary device (microcontroller, PC, etc.) that is connected to the
bridge's serial UART. The back-end implements all data collection and
processing logic, as well as application's display logic. Once the
front-end and the bridge are paired, the back-end can control the
front-end via GUI-O UI serial line protocol, which is transferred by
the bridge from the serial connection to the MQTT broker and the
front-end device (and vice versa).

The project consists of the following components, contained in their
respective sub-folders:

* *guio_esp8266*: Arduino sketch that implements the communication bridge
  for the ESP8266 platform ([README](guio_esp8266/README.md))
* *toggle_counter*: a demo python application back-end for a PC
  ([README](toggle_counter/README.md))
