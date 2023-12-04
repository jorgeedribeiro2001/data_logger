# A9G GPS Module Arduino Project

This Arduino project utilizes the A9G module to read and process GPS data. 
The module communicates via UART and provides GPS information in NMEA format. 
The processed data is then displayed in a user-friendly manner on the Arduino Serial Monitor.

## Features

- Reads GPS data from A9G module.
- Processes NMEA sentences for better readability.
- Displays essential GPS information: Satellites Connections, Latitude, Longitude, and Altitude.
- Allows communication with the A9G module through the Serial Monitor.

## Getting Started

### Prerequisites

- Arduino IDE
- A9G module
- Arduino board with UART capabilities (I utilized the ESP32 WROOM-32, equipped with three UART ports: one for programming, another for the Serial Monitor, and a third for communication with the A9G module.)

### Installation

1. Connect the A9G module to the Arduino board.
2. Open the Arduino IDE.
3. Upload the provided code (`A9G_Project.ino`) to your Arduino board.
4. Open the Serial Monitor to view the processed GPS data.

## Usage

1. Ensure the A9G module is properly connected.
2. Upload the Arduino code to your board.
3. Open the Arduino Serial Monitor to view GPS data.

## Configuration

- Adjust serial baud rates in the code if necessary.
- Customize pins and other settings as per your hardware configuration.

## Author

Jorge Ribeiro


