#include <Wire.h>
#include <Adafruit_INA219.h>

#define BUTTON_PIN 0 // GPIO pin connected to the button

Adafruit_INA219 ina219;

bool buttonState = false; // Current state of the button
bool lastButtonState = false; // Previous state of the button
bool monitoringActive = false; // Whether monitoring is currently active

unsigned long startTime = 0; // Time when the button is pressed

void setup() {
 Serial.begin(115200); // Start the serial communication
 pinMode(BUTTON_PIN, INPUT_PULLUP); // Initialize the button pin as input with pull-up resistor

 // Initialize the INA219 sensor
 if (!ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1);
 }

 // Set the calibration values for 16V 400mA
 ina219.setCalibration_16V_400mA();
}

void loop() {
 // Read the current state of the button
 buttonState = digitalRead(BUTTON_PIN);

 // Check if the button state has changed
 if (buttonState != lastButtonState) {
    // If the button has been pressed, toggle monitoring
    if (buttonState == LOW) { // Assuming the button is active low
      monitoringActive = !monitoringActive;
      // Reset startTime when the button is pressed to start a new recording
      if (monitoringActive) {
        startTime = millis()+53;
      } else {
        startTime = 0; // Reset startTime when stopping monitoring
      }
      delay(50); // Debounce delay
    }
 }

 // Remember the current button state for the next loop iteration
 lastButtonState = buttonState;

 // If monitoring is active, read the sensor values and send them to the serial port
 if (monitoringActive) {
    float shuntvoltage = ina219.getShuntVoltage_mV();
    float busvoltage = ina219.getBusVoltage_V();
    float current_mA = ina219.getCurrent_mA();
    float power_mW = ina219.getPower_mW();

    // Calculate the time relative to when the button was pressed
    unsigned long relativeTime = millis() - startTime;

    // Output the values in CSV format, including the relative time
    Serial.print(relativeTime); Serial.print(",");
    Serial.print(busvoltage,4); Serial.print(",");
    Serial.print(shuntvoltage,4); Serial.print(",");
    Serial.print(current_mA,4); Serial.print(",");
    Serial.println(power_mW,4);

    delay(100-3); // Wait for 100ms before the next reading
 }
}
