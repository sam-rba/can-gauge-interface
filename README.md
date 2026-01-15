# Analog Gauge Driver with CAN Interface

Automotive electronic device that drives up to six analog gauges using data from a CAN bus.


## Contents

- [Hardware](hw) KiCad files for the PCB.
- [Firmware](fw) that runs on the PIC microcontroller.
- [Software](sw) for flashing calibration files onto the device.
- [Documentation](doc) including the [report](doc/report/report.pdf).


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
