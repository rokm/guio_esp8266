#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Toggle Counter demo application for GUI-O ESP8266 bridge.
#
# Copyright (C) 2020, Rok Mandeljc
#
# SPDX-License-Identifier: BSD-3-Clause

import argparse
import math
import datetime

import asyncio
import contextvars
import serial_asyncio

import logging


global_count = contextvars.ContextVar("global_count", default=0)


class Application:
    def __init__(self, transport, init_line):
        self._transport = transport
        self._loop = transport.loop

        self._logger = logging.getLogger("application")

        self.active = True
        self._count = 0  # toggle count
        self._global_count = global_count.get()  # instantiate from ctx

        # Show loading animation
        self.send_command("@sls")

        # Parse the init line
        tokens = init_line.split(" ")
        assert tokens[0] == "@init"
        assert tokens[1].startswith("DPW:")
        assert tokens[2].startswith("DPH:")
        self._screenW = int(tokens[1][4:])
        self._screenH = int(tokens[2][4:])

        self._logger.info("Initializing application...")
        self._logger.info(f"Target screen size: {self._screenW}x{self._screenH}")

        # Clear everything
        self.send_command("@cls")
        self.send_command("@clh")

        # UI scaling, background color
        self.send_command("@guis SCA:1 BGC:#FFFFFF")

        fontSize = 20

        # Create labels for current time
        self.send_command(
            f'|LB UID:lbTime1 X:50 Y:5 FSZ:{fontSize} TXT:"Current time (backend):"'
        )
        self.send_command(f'|LB UID:lbTime2 X:50 Y:10 FSZ:{fontSize} TXT:""')

        # Create a toggle with acknowledge-request time of 1 second
        self.send_command("|TG UID:tg1 X:50 Y:30 RTO:1000")

        # Create labels for toggle count
        self.send_command(
            f'|LB UID:lbCount1 X:50 Y:40 FSZ:{fontSize} TXT:"Toggles (session): {self._count}"'
        )
        self.send_command(
            f'|LB UID:lbCount2 X:50 Y:45 FSZ:{fontSize} TXT:"Toggles (total): {self._global_count}"'
        )

        # Button with a label
        btnW = int(math.floor(self._screenW * 0.90))
        btnH = int(math.floor(self._screenH * 0.10))
        self.send_command(f"|BT UID:btExit X:50 Y:65 W:{btnW} H:{btnH} RTO:1000")
        self.send_command(f"|LB UID:lbExit X:50 Y:65 FSZ:{fontSize} TXT:Exit")

        # Time update task
        self._timeUpdateFreq = 1  # 1 per second
        self._taskTimeUpdate = asyncio.ensure_future(
            self.task_update_time(), loop=self._loop
        )

        # Hide loading animation
        self.send_command("@hls 500")

    async def task_update_time(self):
        while self.active:
            now = datetime.datetime.now()
            self._logger.debug(f"Updating time: {now}")
            self.send_command(f'@lbTime2 TXT:"{now:%Y-%m-%d %H:%M:%S}"')
            await asyncio.sleep(1.0 / self._timeUpdateFreq)  # Fire with desired freq

    def send_command(self, command):
        self._transport.write(b"$")
        self._transport.write(command.encode("ascii"))
        self._transport.write(b"\n")

    def handle_line(self, line):
        self._logger.debug(f"Processing line: {line}")

        tokens = line.split(" ")

        if tokens[0] in ("?@tg1", "@tg1"):
            # Toggle 'tg1' toggled
            if tokens[0].startswith("?"):
                self.send_command("@tg1 CRE:1")  # send ACK

            state = int(tokens[1])
            self._logger.info(f"Toggle state update: {state}")

            self._count += 1
            self._global_count += 1
            global_count.set(self._global_count)  # keep in sync

            # Update toggle count labels
            self.send_command(f'@lbCount1 TXT:"Toggles (session): {self._count}"')
            self.send_command(f'@lbCount2 TXT:"Toggles (total): {self._global_count}"')

        elif tokens[0] in ("?@btExit", "@btnExit"):
            # Close button pressed
            if tokens[0].startswith("?"):
                self.send_command("@btExit CRE:1")  # send ACK

            self._logger.info("Exit button pressed!")

            # Shutdown
            self._shutdown()

    def _shutdown(self):
        self._logger.info("Shutting down application instance...")

        self.active = False

        # Cancel the time update task
        self._taskTimeUpdate.cancel()

        # Reset the UI
        self.send_command("@cls")
        self.send_command("@clh")


class GuioCommHandler(asyncio.Protocol):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._program = None
        self._buffer = b""
        self._logger = logging.getLogger("serial")

    def connection_made(self, transport):
        self.transport = transport
        self._logger.info("Serial port opened!")
        self._logger.debug(f"Serial transport: {transport}")

        # Set serial parameters
        # transport.serial.rts = False

    def data_received(self, data):
        self._logger.debug(f"Serial received {len(data)} bytes: {data}")

        while data:
            idx = data.find(b"\r\n")
            if idx == -1:
                # No delimiter; append everything
                self._buffer += data
                break
            else:
                # Append up to delimiter...
                self._buffer += data[:idx]

                # ... process line ...
                if self._buffer:
                    self._handle_line(self._buffer)
                else:
                    self._logger.warning("Received empty line!")

                # ... and clear both data and line buffer
                data = data[idx+2:]
                self._buffer = b""

    def connection_lost(self, exc):
        self._logger.info("Serial port closed!")
        self.transport.loop.stop()

    def _handle_line(self, line):
        self._logger.debug(f"Received line: {line}")

        line = line.decode("utf-8", errors="replace")
        if line.startswith("$"):
            # GUI-O command passthrough
            line = line[1:]
            self._logger.debug(f"GUI-O message: {line}")

            if line.startswith("@init "):
                self._program = None
                self._program = Application(self.transport, line)
            elif self._program:
                if self._program.active:
                    self._program.handle_line(line)
                else:
                    self._logger.warning(
                        "Received GUI-O message with inactive application instance!"
                    )
            else:
                self._logger.warning(
                    "Received GUI-O message with no application instance!"
                )
        elif line.startswith("!"):
            # ESP8266 module protocol
            line = line[1:]
            self._logger.info(f"ESP8266 message: {line}")
        else:
            # Debug line
            self._logger.debug(f"debug message: {line}")


def main():
    # Logging
    logging.basicConfig(encoding="utf-8", level=logging.INFO)
    logger = logging.getLogger("main")

    # Command-line parser
    parser = argparse.ArgumentParser(
        description="Toggle counter GUI-O demo.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
        argument_default=argparse.SUPPRESS,
    )
    parser.add_argument(
        "--port",
        metavar="serial_port",
        type=str,
        help="Serial port.",
        default="/dev/ttyUSB0",
    )
    parser.add_argument(
        "--baudrate",
        metavar="baudrate",
        type=int,
        help="Communication baudrate.",
        default=115200,
    )
    args = parser.parse_args()

    # Main event loop
    loop = asyncio.get_event_loop()

    # Establish serial communication
    port = args.port
    baudrate = args.baudrate

    logger.info(
        f"Initializing serial communication (port: {port}, baudrate: {baudrate}..."
    )
    logger.info(f"Serial port: {port}")
    logger.info(f"Baud rate: {baudrate}")
    loop.run_until_complete(
        serial_asyncio.create_serial_connection(
            loop, GuioCommHandler, port, baudrate=baudrate
        )
    )

    # Enter event loop
    logger.info("Entering main loop...")
    loop.run_forever()
    loop.close()


if __name__ == "__main__":
    main()
