void bme_config() {
  Serial.println("Configuring the BME Sensor");
  Wire.endTransmission(true);

  if (!bme.begin(0x76)) {
    Serial.println("Check wiring!");
    return;
  } else {
    Serial.println("BME680 connected succesfully!");
  }

  //Oversampling e filter initialization
  bme.setTemperatureOversampling(BME680_OS_16X);
  bme.setHumidityOversampling(BME680_OS_16X);
  bme.setPressureOversampling(BME680_OS_16X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);

  Wire.endTransmission(true);
}

void BME_Reading(float &temperature, float &humidity, float &pressure) {

  if (bme.performReading()) {
    temperature = bme.temperature;
    humidity = bme.humidity;
    pressure = bme.pressure;

  } else {
    log(ERROR, "bme_reading_task", "Failed obtaining data from BME");
  }
  Wire.endTransmission();
}