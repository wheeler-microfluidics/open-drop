![https://ci.appveyor.com/api/projects/status/github/wheeler-microfluidics/open-drop?branch=master&svg=true](https://ci.appveyor.com/api/projects/status/github/wheeler-microfluidics/open-drop?branch=master&svg=true)
# open-drop #
Control package for OpenDrop Digital Microfluidics Platform

## Overview ##

This package contains:

 - Firmware compatible with Arduino Uno or Mega2560.
 - Installable Python package for interfacing with Arduino firmware through
   serial port or i2c (through a serial-to-i2c proxy).

## Install ##

The Python package can be installed through `pip` using the following command:

    pip install open-drop

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

--------------------------------------------------

## Usage ##

After uploading the firmware to the board, the `open_drop.Proxy` class can be
used to interact with the Arduino device.

See the session log below for example usage.

### Example interactive session ###

    >>> from serial import Serial
    >>> from open_drop import Proxy

Connect to serial device.

    >>> serial_device = Serial('/dev/ttyUSB0', baudrate=115200)

Initialize a device proxy using existing serial connection.

    >>> proxy = Proxy(serial_device)

Query the number of bytes free in device RAM.

    >>> proxy.ram_free()
    409

Query descriptive properties of device.

    >>> proxy.properties()
    base_node_software_version                               0.9.post8.dev141722557
    name                                                                  open_drop
    manufacturer                                                        Wheeler Lab
    url                           http://github.com/wheeler-microfluidics/open-d...
    software_version                                                            0.1
    dtype: object

Use Arduino API methods interactively.

    >>> # Set pin 13 as output
    >>> proxy.pin_mode(13, 1)
    >>> # Turn led on
    >>> proxy.digital_write(13, 1)
    >>> # Turn led off
    >>> proxy.digital_write(13, 0)

Query number of available channels.

    >>> proxy.channel_count()
    40

Query state of channels array.

    >>> proxy.state_of_channels
    array([0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], dtype=uint8)

Turn on every other channel.

    >>> proxy.state_of_channels = 20 * [0, 1]

Query updated state of channels array.

    >>> proxy.state_of_channels
    array([0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0,
           1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1], dtype=uint8)

### Configuration and state ###

The device stores a *configuration* and a *state*.  The configuration is
serialized and stored in EEPROM, allowing settings to persist across device
resets.  The state is stored in device memory and is reinitialized each time
the device starts up.

Print (non-default) configuration values.

    >>> print proxy.config
    serial_number: 2
    baud_rate: 115200
    i2c_address: 17

    >>> proxy.config.
    proxy.config.max_waveform_frequency  proxy.config.max_waveform_voltage
    >>> proxy.config.max_waveform_voltage
    200
    >>> proxy.config.max_waveform_frequency
    10000

Set voltage and frequency.

    >>> result_code = proxy.update_state(voltage=100, frequency=1e3)
    >>> print proxy.state
    voltage: 100.0
    frequency: 1000.0

#### Validation ####

Note that negative values are not allowed for voltage or frequency.

    >>> result_code = proxy.update_state(voltage=-1)  # Negative voltage 
    >>> print proxy.state
    voltage: 100.0
    frequency: 1000.0

Voltage/frequency updates are restricted to allowable range.

    >>> result_code = proxy.update_state(voltage=300)  # Voltage greater than max
    >>> print proxy.state  # Voltage remains unchanged
    voltage: 100.0
    frequency: 1000.0

Max values can be increased by updating the configuration.

    >>> result_code = proxy.update_config(max_waveform_voltage=300)
    >>> result_code = proxy.update_state(voltage=300)  # Voltage now <= max
    >>> print proxy.state  # Voltage changed
    voltage: 300.0
    frequency: 1000.0

To persist changes to configuration across device reset - *not* state - use
`save_config` method.

    >>> proxy.save_config()

### Other methods ###

Below is a list of the attributes of the `open_drop.Proxy` Python class.  Note
that many of the [Arduino API][1] functions (e.g., `pin_mode`, `digital_write`,
etc.) are exposed through the RPC API.

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

--------------------------------------------------

## Firmware development ##

The Arduino firmware/sketch is located in the `open_drop/Arduino/open_drop`
directory.  The key functionality is defined in the `open_drop::Node` class in
the file `Node.h`.

Running the following command will build the firmware using [SCons][2] for
Arduino Uno and Arduino Mega2560, and will package the resulting firmware in a
Python package, ready for distribution.

    paver sdist

### Adding new remote procedure call (RPC) methods ###

New methods may be added to the RPC API by adding new methods to the
`open_drop::Node` class in the file `Node.h`.

# Author #

Copyright 2015 Christian Fobel <christian@fobel.net>


[1]: https://www.arduino.cc/en/Reference/HomePage
[2]: http://www.scons.org/
