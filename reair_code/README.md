# REAIR
### ระบบตรวจวัดและฟื้นฟูคุณภาพอากาศอัจฉริยะด้วยเทคโนโลยีการตกตะกอนไฟฟ้าสถิต

## About

REAIR is a prototype smart air quality monitoring and purification
system designed to monitor air quality and automatically adjust
the purification process according to PM2.5 levels.

The system uses an ESP32/KidBright32 as the main controller and
integrates air-quality monitoring, fan control, electrostatic
precipitation, safety monitoring, and energy-saving operation.

---

## System Concept

The system operates using a feedback-control concept:

Measure → Analyze → Purify → Measure Again → Adjust

When PM2.5 increases, the system increases the operating level
of the fan and electrostatic precipitator.

When PM2.5 reaches the target level, the system reduces the
operating level and enters ECO mode to reduce energy consumption.

---

## Operating Modes

### STANDBY

The system continuously displays simulated air-quality values
and basic system information.

### AUTO

When a dust event is detected, PM2.5 increases and the system
automatically increases the fan and electrostatic precipitator
levels.

The PM2.5 value is then simulated to gradually decrease as the
purification process operates.

### ECO

When PM2.5 reaches the target level, the system reduces the
operating level of the fan and electrostatic precipitator
to reduce power consumption.

---

## Monitoring System

The S1 button is used to switch between monitoring pages.

### Page 1 — Air Quality
- PM2.5
- Temperature
- Humidity

### Page 2 — System Operation
- Fan Level
- ESP Level
- Power

### Page 3 — Safety
- O3
- O3 Limit
- Safety Status

### Page 4 — System Status
- Current Mode
- System Status

---

## Button Functions

| Button | Function |
|---|---|
| S1 | Switch monitoring pages |
| S2 | Simulate a dust event and start AUTO mode |

---

## Hardware

- ESP32 / KidBright32 V1.3
- Fan
- Electrostatic Precipitator (ESP)
- Temperature & Humidity Sensor
- LED Matrix Display

---

## Software

- Arduino IDE
- C/C++
- KidBright32 Arduino Library
- Adafruit GFX Library
- Adafruit LED Backpack Library

---

## Current Prototype Status

This project is currently a prototype.

Some sensor values, including O3, are simulated for testing the
control logic and display system.

The O3 value in the current prototype is not measured by a real
O3 sensor.

---

## Development

The system was developed iteratively by designing the control
logic, implementing the prototype software, testing the system,
identifying errors, and refining the program behavior.

AI-assisted development was used as a programming support tool
during the development process.

The project developer designed the system requirements,
operating logic, hardware concept, and testing behavior.
