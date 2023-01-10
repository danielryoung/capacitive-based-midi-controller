This is some of the code that's been written for sending MIDI code with a teensy microcontroller, using the MPR121 board to sense capacitive touchpoints.  The MPR121 can be daisychained via I2C to control up to 48 touchpoints.  The teensy was used because it can be easily recognized as a USB MIDI device when connected to MIDI software.

The current code examples are for sending a note on/off message with a single MPR121, and the CC code repeatedly sends a CC message while the capacitive touchpoint is in use.  It is written for two MPR121s chained together.

Be aware of the libraries necessary to run this code.  The most import ones:
Adafruit MPR121 code.  Works with generic MPR121 boards
Ticker library for timing control.
Wire library
