## ToysToLifeLib

A C++ (Arduino/PlatformIO) library for building, and parsing the USB packet protocols used by Toys-to-Life game peripherals.
It gives you simple type-safe functionality for creating USB packets to do things such as:
- Change Pad Color(s)
- Handshake / Authentication
- Tag Read/Write


## Installation

This is a [PlatformIO](https://platformio.org/) library targeting `espressif32` and `raspberrypi` boards under the `arduino` framework.
Add it to a project's `platformio.ini`:

```ini
lib_deps =
    https://github.com/FromDarkHell/ToysToLifeLib.git
```

## Usage

See [examples/packet-maker](examples/packet-maker) for a runnable PlatformIO demo that builds packets
for both protocols and prints them over serial.