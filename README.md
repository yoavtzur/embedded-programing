# Motor Control and GUI Interface Project

## Overview
This project implements a control system for motor-based machines, including manual control, joystick-based control, and a GUI interface for interacting with the system. It also features a painting application using joystick inputs and a script mode for executing custom commands.

## Features
- **Manual Motor Control**: Rotate the motor in various directions using manual commands.
- **Joystick-based PC Painter**: Use a joystick to paint on a canvas in real-time.
- **Stepper Motor Calibration**: Calibrate and track motor positions for accurate control.
- **Script Mode**: Execute custom scripts for controlling the motor and other functionalities.
- **GUI Interface**: A PySimpleGUI-based graphical interface for controlling motor functions and interacting with scripts.

## Components
1. **Motor Control**: The project provides manual, joystick, and automated control of a stepper motor.
2. **GUI**: A Python-based GUI built using PySimpleGUI for controlling the motor, executing scripts, and providing feedback to the user.
3. **Joystick-based Painter**: A feature that allows a joystick to be used as an input device to control the mouse pointer for painting.

## Files
- **GUI.py**: Main Python script that provides the graphical interface and logic for motor control and joystick painting.
- **api.c**: API layer for controlling the motor and handling joystick inputs.
- **bsp.c**: Board support package for configuring the MSP430 microcontroller and managing interrupts.
- **flash.c**: Manages data storage in flash memory, including script execution and memory writes.
- **main.c**: Main application file that coordinates the system states and calls the appropriate functions for motor control, calibration, and script execution.

## Usage
1. Run the GUI by executing `GUI.py`.
2. Use the GUI buttons to control the motor manually or enter script mode.
3. In joystick-based painting mode, move the joystick to control the mouse pointer and draw on the canvas.
4. Use the script mode to upload and execute scripts that define custom motor movements and LCD operations.

## Setup
- **Requirements**:
  - Python 3.x
  - PySimpleGUI
  - MSP430G2553 microcontroller
  - Serial communication enabled (adjust COM port as needed in `GUI.py`)

## How to Run
1. Ensure the MSP430 is connected to the PC via a serial connection.
2. Run the `GUI.py` script.
3. Interact with the GUI for motor control, joystick painting, or script execution.
4. For joystick-based control, connect a compatible joystick to the system.

## License
This project is licensed under the MIT License.
