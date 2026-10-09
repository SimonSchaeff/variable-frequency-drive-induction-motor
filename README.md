# Frequency Inverter – University Project

This repository contains the development files for a custom-built frequency inverter developed as part of a university project at Munich University of Applied Sciences (Hochschule München).

The power electronics and control electronics were designed using KiCad. The corresponding hardware design files are located in the `hardware/` directory. The software is based on an STM32 microcontroller, and the firmware source code is available in the `source_code/` directory.

The frequency inverter is designed to control asynchronous motors with a power rating of up to 3 kW. The operating frequency can be set within a range of 0 to 99 Hz, either manually using a potentiometer or wirelessly via a Bluetooth-enabled smartphone application. The application is located in the `smartphone_app/` directory.

## Disclaimer

This project is intended for educational and engineering development purposes. No warranty is provided regarding the functionality, safety, or suitability of the design for a particular application.