# Adafruit TCS3448 Library [![Build Status](https://github.com/adafruit/Adafruit_TCS3448/workflows/Arduino%20Library%20CI/badge.svg)](https://github.com/adafruit/Adafruit_TCS3448/actions)[![Documentation](https://github.com/adafruit/ci-arduino/blob/master/assets/doxygen_badge.svg)](http://adafruit.github.io/Adafruit_TCS3448/html/index.html)

This is the Adafruit TCS3448 14-channel multi-spectral sensor library for
Arduino.

The TCS3448 register map and automatic channel multiplexer are compatible with
the AS7343. This library subclasses the Adafruit AS7343 driver, selects the
TCS3448 I2C address, and adds TCS3448-named data types and a coherent
non-blocking result read.

The public API and core coherent-read implementation are under review. Numbered
hardware tests cover initialization, both GPIO directions, RGB/UVA response,
SMUX modes, gain, integration time, saturation, interrupts, persistence,
power/wait operation, auto-zero, 100/120 Hz flicker, I2C speed, and continuous
stability. The onboard white LED has a visible blink example and a focused
off-on-off optical hardware test.

Adafruit invests time and resources providing this open source code. Please
support Adafruit and open-source hardware by purchasing products from Adafruit!

## Dependencies

* [Adafruit AS7343](https://github.com/adafruit/Adafruit_AS7343)

## Documentation and Doxygen

Documentation is produced by Doxygen. Contributions should include
documentation for any new features that they add.

## About this Driver

Written by Ladyada for Adafruit Industries.

MIT license; see `LICENSE` for more information. All text above must be included
in any redistribution.
