
# Data Logger Project

## Overview
This repository contains the implementation of a data logger project designed for real-time and remote data acquisition during the shipment of Bosch Home Comfort products. The project includes scripts for data processing, device software, and a web interface for data visualization and control.

## Repository Structure

- **Back-End Scripts**: Scripts for data processing and storage.
- **Device Software (Microcontroller)**: Code for the microcontroller used in the data logging device.
- **Django Web-Framework**: Web application framework for interfacing with the data logger.
- **Tests and Results**: Testing scripts and result logs.

## Languages Used
- HTML: 50.8%
- Jupyter Notebook: 43.2%
- Python: 3.0%
- C++: 3.0%

## Getting Started

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/jorgeedribeiro2001/data_logger_project.git
   cd data_logger_project
   ```

2. **Install Dependencies**:
   - For Python components, use the requirements file:
     ```bash
     pip install -r requirements.txt
     ```

3. **Run the Django Application**:
   - Navigate to the Django project directory:
     ```bash
     cd django_project
     ```
   - Apply migrations:
     ```bash
     python manage.py migrate
     ```
   - Create a superuser to access the admin interface:
     ```bash
     python manage.py createsuperuser
     ```
   - Start the development server:
     ```bash
     python manage.py runserver
     ```
   - Access the application at `http://127.0.0.1:8000/`.

## Microcontroller Program

The microcontroller program is developed using the ESP32 microcontroller and various sensors to monitor parameters such as temperature, humidity, pressure, impacts, vibration, and location. The code is structured into tasks that handle sensor readings, data storage, and communication with the web platform via MQTT.

### Key Components:
- **ESP32 Microcontroller**: Handles sensor integration and data processing.
- **Sensors**: Include BME680 for environmental data, IMU for motion data, and GPS for location tracking.
- **SD Card**: For local data storage.
- **MQTT Protocol**: For real-time data transmission to the web platform.

### Running the Microcontroller Program:
1. **Set Up the Development Environment**:
   - Install the necessary tools and libraries for ESP32 development.
   - Use the Arduino IDE or PlatformIO for code uploading.

2. **Upload the Code**:
   - Connect the ESP32 to your computer.
   - Compile and upload the code from the `microcontroller` directory.

3. **Configure MQTT**:
   - Ensure the MQTT broker details in the code match your setup.
   - The device will start collecting data and transmitting it to the web platform.

## Contribution

1. Fork the repository.
2. Create a new branch (`git checkout -b feature-branch`).
3. Commit your changes (`git commit -m 'Add some feature'`).
4. Push to the branch (`git push origin feature-branch`).
5. Open a pull request.

## License
This project is licensed under the MIT License.

## Contact
For any queries, contact the repository owner.

---
