# CAN Bus Dashboard with Arduino Nano & ESP32 + LVGL Round Display

## Overview

This project implements a custom automotive dashboard using:

**MCP2515** to read from CAN BUS
**Arduino Nano Every** to read data from MCP2515 and transmit data via UART.
**AZ Delivery ESP32** with a **Waveshare 1.85"** round TFT display to render RPM, temperature, and battery voltage using **LVGL** graphics library.

The system simulates or receives data like RPM, engine temperature, and battery voltage, processes it on the ESP32, and displays it with a gauge-style UI similar to a car dashboard. It includes emergency state handling and serial communication error detection.

---

## Hardware Used

### Arduino Nano Every
- Sends CAN data (RPM, Temperature, Voltage) over Serial UART (Serial TX).
- Data format: ASCII messages like `R3250`, `T87.5`, `V13.2`.

### ESP32 and Round Display
- Model: ESP32 1.85" Round LCD (GC9A01 driver).
- Receives and parses UART data from the Nano.
- Displays data using [LVGL](https://lvgl.io/) for a modern, responsive UI.
- Shows emergency states by flashing background red (e.g., overheat or undervoltage).
- Implements UART timeout detection and resets display on disconnection.

---

## Features

- **Real-time Data Display**:
  - RPM gauge (with smooth animation).
  - Temperature and voltage labels.
  
- **Emergency Feedback**:
  - Red blinking background when temperature exceeds 105°C or voltage drops below 12V.

- **Serial Timeout Detection**:
  - If no data is received in 1 second, labels show "ERR" or reset to default.

- **Averaging Filter**:
  - The ESP32 buffers the latest 10 RPM readings (from the last second) and uses the average to ensure stable gauge behavior.

---
