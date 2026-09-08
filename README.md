# Analog Gauge Driver with CAN Interface

Automotive electronic device that drives up to six analog gauges using data from a CAN bus.


## Contents

- `hw/` — Hardware; KiCad files for the PCB.
- `fw/` — Firmare that runs on the PIC microcontroller.
- `sw/` — Software for flashing calibration files onto the device.
- `doc/` — Documentation.


## Features

### Interfaces
- CAN 2.0B interface, up to 500kbps
- Flexible CAN encoding scheme, configurable via CAN

### Outputs
- 1 tachometer output
- 1 speedometer output
- 4 analog outputs (0–5V)

### Power supply
- 9–16V DC input
