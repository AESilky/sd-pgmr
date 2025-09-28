# Human Interface Device (HID) and Navigation (NAV) Functionality

## HID

The HID consists of the display panel, rotary encoder (knob), and
cursor switches. The hardware itself is under the control of the
Hardware Runtime, but the overall functionality of the interface
is controlled by this module.

In normal operation, status of the servos and sensors is displayed. There
is also an area that is reserved for outputting debug and informational
messages.

## NAV

The NAV (Navigation) consists of GPS and accelerometer, gyroscope, and
magnetometer sensors that are monitored and value messages posted to the
host.
