# ESP8266 GUI-O IoT communication bridge

This project implements a GUI-O IoT communication bridge on ESP8266
platform.

Its primary aim is to provide a communication bridge that can be used
by other devices (e.g., a microcontroller or a PC connected to the
bridge via serial UART) implementing the GUI-O IoT application back-end.

However, it could also serve as an example and a starting template for
implementing a GUI-O IoT application back-end on the ESP8266 itself.


## 1 Requirements and setup

The source code for the communication bridge is available as an
Arduino sketch, which should be compiled and uploaded to the target board
using Arduino IDE.

The following external libraries are also required:

* *ESP8266 Arduino Core*: core library for using Arduino on ESP8266
* *TaskScheduler*: cooperative multi-tasking framework for Arduino
* *ESPAsyncTCP*: async TCP library for ESP8266 Arduino
* *ESPAsyncWebServer*: async HTTP server for ESP8266 Arduino
* *ArduinoJson*: JSON library for Arduino

For details, see the corresponding sub-sections below.


### 1.1 Quick start

1. Install Arduino IDE
2. Install all required libraries (see the following sub-section)
3. Open the `gui_esp8266.ino` Arduino sketch
4. Adjust program's settings in `config.h`
5. Select your ESP8266 board in `Tools -> Boards`
6. Compile the sketch
7. Upload the program to the board


### 1.2 Requirements - details

NOTE: the versions indicated below correspond to the ones that were
used during the project's development.


#### 1.2.1 ESP8266 Arduino Core

Core library for using Arduino on ESP8266. Also provides board definitions
for some ESP8266 boards.

*URL*: https://github.com/esp8266/Arduino

*Version*: 2.7.4

*Installation:* Installed via Arduino Boards manager. For details, see
the project's [README](https://github.com/esp8266/Arduino/blob/master/README.md).


#### 1.2.2 TaskScheduler

Cooperative multi-task framework for Arduino. Used to schedule
asynchronous tasks.

*URL:* https://github.com/arkhipenko/TaskScheduler

*Version:* 3.2.0

*Installation:* Installed directly using Arduino's Library Manager.


#### 1.2.3 ESPAsyncWebServer

Async TCP library for ESP8266 Arduino. Required by `ESPAsyncWebServer`.

*URL:* https://github.com/me-no-dev/ESPAsyncTCP

*Version:* git snapshot, `1547686`

*Installation:* Installed from git snapshot.


#### 1.2.4 ESPAsyncWebServer

Async HTTP server for ESP8266 Arduino. Used to implement HTTP server
in AP mode, for receiving pairing requests from GUI-O app.

*URL:* https://github.com/me-no-dev/ESPAsyncWebServer

*Version:* git snapshot, `f6fff3f`

*Installation:* Installed from git snapshot.


#### 1.2.5 ArduinoJson

JSON serialization and deserialization library for Arduino. Used in AP
mode for pairing requests and responses.

*URL:* https://arduinojson.org

*Version:* 6.17.2

*Installation:* Installed directly from Arduino Library Manager.


#### 1.2.6 PubSubClient

A publish/subscribe MQTT client library. Used in STA mode for communication
with MQTT broker.

*URL:* https://pubsubclient.knolleary.net

*Version:* 2.8.0

*Installation:* Installed directly from Arduino Library Manager.


## 2 Tested platforms

The code has been tested with the following ESP8266-based boards:

* `WeMos D1 R1`: using Arduino's board profile with the same name


## 3 Implementation details

The bridge implements two main operation modes - AP mode for pairing,
and STA mode for actual bridge functionality.

If device pairing parameters are not available in the EEPROM (the very
first run, or parameters were cleared), the device will start in the
AP mode. If pairing parameters are available, the device will start in
the STA mode. It is possible to reboot a paired bridge into AP mode
via digital pin (see `Section 3.3.3`) or `!REBOOT_AP` serial command
(see `Section 3.5`).


### 3.1 AP mode

In the AP mode, the bridge operates a soft access point for WiFi, with
SSID set to `guio_MAC` (where MAC is hex digest of the device's MAC
address) and pre-defined (built-in) password.

The bridge runs a HTTP server on a port :80, with a pairing request
handler installed at `/pair` URL. During the pairing procedure, this
handler receives JSON with pairing data, processes it, and returns the
response JSON as per GUI-O app pairing protocol. Once the pairing is
complete, the device restarts in STA mode.

The operation in AP mode is indicated by constant blinking of the
signalization LED (Section 3.3.2).

In AP mode, the bridge fully responds to the button pin (Section 3.3.2).

In AP mode, the bridge ignores `$`-prefixed serial messages (Section 3.4)
and fully responds to the built-in `!`-prefixed commands (Section 3.5).
The `!PING` command always returns `STATUS_AP_READY` code as defined in
`program_base.h`.


### 3.2 STA mode

In the STA mode, the bridge attempts to establish connection to the
WiFi and subsequently the MQTT broker, using the device pairing parameters
retrieved from the EEPROM. The status of connection is periodically checked
and re-connection attempts are performed as necessary. The lack of
connection is indicated via blinking signalization LED (Section 3.3.2),
while during active connection, the LED is permanently turned on.

In STA mode, the bridge fully responds to the button pin (Section 3.3.2).

In STA mode, the bridge forwards the `$`-prefixed messages (Section 3.4)
to the configured MQTT topic (with `$`-prefix stripped). Similarly, it
forwards any messages received via the configurd MQTT topic back to the
serial connection and prefixes the message with `$`-prefix.

The bridge also fully responds to the built-in `!`-prefixed commands
(Section 3.5). The `!PING` command returns a `STATUS_STA_` code that
describes the current connection status, as defined in `program_base.h`.


### 3.3 Peripherals

The bridge makes use of the following peripherals:


#### 3.3.1 Serial UART

The (default) `Serial` object is used for serial communication, which
encompasses:

* sending debug messages to connected back-end device (if built in
  debug mode)
* receiving and sending `$`-prefixed lines from/to the back-end device
  (GUI-O protocol pass-through)
* receiving and sending `!`-prefixed lines from/to the back-end device
  (built-in command set)

NOTE: after a hard reset, the `ESP8266` bootloader sends its status via
serial UART at baud rate 7488 bps. When the serial port is configured to
operate at other baud rates, that initial message will appear garbled.

The debug messages from the bridge can be disabled by commenting out the
definition of `_GUIO_DEBUG` macro in `config.h`.

The baud rate for serial UART is defined via `_GUIO_SERIAL_BAUDRATE`
macro in `config.h`.


#### 3.3.2 Signalization LED

By default, the LED corresponding to `LED_BUILTIN` is used for signaling
the program's state and operation. The LED pin can be changed via
`_GUIO_LED_MAIN` macro in `config.h`.


#### 3.3.3 Button pin for switching to AP mode

A digital pin (`D04` by default) is used as a button for switching into
AP mode (short press) or clearing the parameters in EEPROM (long press).
The pin can be changed via the `_GUIO_AP_BUTTON` macro in `config.h`.


### 3.4 Pass-through mode for GUI-O protocol

The bridge implements a pass-through protocol for the GUI-O messages.
When the bridge is running in the STA mode, all lines received over
serial UART that are prefixed with the dollar sign (`$`) are published
to the configured MQTT topic on the MQTT broker (assuming connection
is established). The lines are published with the `$`-prefix and
trailing newline characters removed.

In the AP mode, the received `$`-prefixed messages are ignored.

The messages received via subscribed MQTT topic are forwarded to the
serial connection, with the added `$`-prefix. The trailing newline
characters are removed from the message, and a CRLF newline is added
when message is written to the serial UART.

The `$`-prefix allows the back-end device, connected to the bridge
via serial UART, to quickly determine whether an incoming line
is a GUI-O protocol line, a reply to the built-in command (see below),
or an auxiliary/diagnostic message from the bridge.


### 3.5 Built-in command set for direct communication

The built-in commands and responses are prefixed with an exclamation
mark (!). The following commands can be sent to the bridge from the
connected device via serial UART:

* `!REBOOT`: reboot the ESP8266 bridge into preferred mode
* `!REBOOT_AP`: reboot the ESP8266 bridge into AP mode
* `!CLEAR_PARAMS`: clear the parameters in EEPROM and reboot into AP mode
* `!PING`: the bridge responds with a `!PONG status`, where `status`
  is an integer status code with meanings defined in `program_base.h`.

The above command set works in both AP and STA mode.


## 4 Issues and limitations

The following sections outline the known issues and limitations of
the current implementation.


### 4.1 No support for MQTT over TLS

The TLS-encrypted MQTT connection is unsupported at the moment, and the
connection to MQTT broker is always performed via port 1883.

The GUI-O application pairing protocol in its current version lacks the
options to specify the target MQTT broker port and TLS/non-TLS mode. In
addition, the pairing protocol would need to be extended with an option
to provide the client certificate, and its storage/retrieval from device's
EEPROM would need to be implemented on the ESP8266 side.
