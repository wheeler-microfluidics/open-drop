# open-drop #
Control package for OpenDrop Digital Microfluidics Platform

## Overview ##

This package contains:

 - Firmware compatible with Arduino Uno or Mega2560.
 - Installable Python package for interfacing with Arduino firmware through
   serial port or i2c (through a serial-to-i2c proxy).

## Install ##

The Python package can be installed through `pip` using the following command:

    pip install wheeler.open-drop

## Upload firmware ##

To upload the pre-compiled firmware included in the Python package, run the
following command:

    python -m open_drop.bin.upload <board type>

replacing `<board type>` with either `uno` or `mega2560`, depending on the
model of the board.

This will attempt to upload the firmware by automatically discovering the
serial port.  On systems with multiple serial ports, use the `-p` command line
argument to specify the serial port to use.  For example:

    python -m open_drop.bin.upload -p COM3 uno

## Usage ##

After uploading the firmware to the board, the `open_drop.Proxy` class can be
used to interact with the Arduino device.

See the session log below for example usage.

    >>> from serial import Serial
    >>> from open_drop import Proxy
    >>> serial_device = Serial('/dev/ttyUSB0', baudrate=115200)
    >>> proxy = Proxy(serial_device)
    >>> proxy.ram_free()
    409
    >>> proxy.properties()
    base_node_software_version                               0.9.post8.dev141722557
    name                                                                  open_drop
    manufacturer                                                        Wheeler Lab
    url                           http://github.com/wheeler-microfluidics/open-d...
    software_version                                                            0.1
    dtype: object
    >>> # Set pin 13 as output
    >>> proxy.pin_mode(13, 1)
    >>> # Turn led on
    >>> proxy.digital_write(13, 1)
    >>> # Turn led off
    >>> proxy.digital_write(13, 0)
    >>> # Query number of available channels
    >>> proxy.channel_count()
    40
    >>> # Query state of channels array
    >>> proxy.state_of_channels
    array([0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], dtype=uint8)
    >>> # Turn on every other channel
    >>> proxy.state_of_channels = 20 * [0, 1]
    >>> # Query updated state of channels array
    >>> proxy.state_of_channels
    array([0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0,
           1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1], dtype=uint8)
    >>> # Other available remote methods:
    >>> proxy.
    proxy.analog_read                      proxy.microseconds
    proxy.analog_write                     proxy.milliseconds
    proxy.array_length                     proxy.name
    proxy.base_node_software_version       proxy.on_config_baud_rate_changed
    proxy.begin                            proxy.on_config_i2c_address_changed
    proxy.buffer_size                      proxy.on_config_serial_number_changed
    proxy.channel_count                    proxy.on_state_frequency_changed
    proxy.config                           proxy.on_state_voltage_changed
    proxy.delay_ms                         proxy.pin_mode
    proxy.delay_us                         proxy.properties
    proxy.digital_read                     proxy.ram_free
    proxy.digital_write                    proxy.read_eeprom_block
    proxy.echo_array                       proxy.reset_config
    proxy.get_buffer                       proxy.reset_state
    proxy.i2c_address                      proxy.save_config
    proxy.i2c_available                    proxy.serialize_config
    proxy.i2c_buffer_size                  proxy.serialize_state
    proxy.i2c_read                         proxy.set_i2c_address
    proxy.i2c_read_byte                    proxy.set_state_of_channels
    proxy.i2c_request                      proxy.software_version
    proxy.i2c_request_from                 proxy.state
    proxy.i2c_scan                         proxy.state_of_channels
    proxy.i2c_write                        proxy.str_echo
    proxy.load_config                      proxy.update_config
    proxy.manufacturer                     proxy.update_eeprom_block
    proxy.max_i2c_payload_size             proxy.update_state
    proxy.max_serial_payload_size          proxy.url

## Firmware development ##

The Arduino firmware/sketch is located in the `open_drop/Arduino/open_drop`
directory.  The key functionality is defined in the `open_drop::Node` class in
the file `Node.h`.

Running the following command will build the firmware using [SCons][1] for
Arduino Uno and Arduino Mega2560, and will package the resulting firmware in a
Python package, ready for distribution.

    paver sdist

### Adding new remote procedure call (RPC) methods ###

New methods may be added to the RPC API by adding new methods to the
`open_drop::Node` class in the file `Node.h`.

# Author #

Copyright 2015 Christian Fobel <christian@fobel.net>


[1]: http://www.scons.org/
