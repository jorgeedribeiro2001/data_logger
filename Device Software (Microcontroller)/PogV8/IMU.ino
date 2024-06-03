//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//Function to configure the IAM-20680 Motion Sensor

void imu_config_accelerometer() {

  // Check if the IMU is connected
  Wire.beginTransmission(IMU_ADDRESS);
  if (Wire.endTransmission() != 0) {
    Serial.println("IMU not connected. Please check connection.");
    while (1) { delay(300); }  //Stays in loop
  }

  //Resetting the IMU
  write_I2C(IMU_ADDRESS, 0x6B, 0b10000000);  // PWR_MGMT_1 register
  vTaskDelay(pdMS_TO_TICKS(200));            //Time for iniciating again

  // PWR_MGMT_1 register (REGISTER 107 – POWER MANAGEMENT 1 - Pag42)
  // set clock source
  write_I2C(IMU_ADDRESS, 0x6B, 0b10000001);

  //SMPLRT_DIV register ( REGISTER 25 – SAMPLE RATE DIVIDER - Pag34)
  //SAMPLE_RATE = INTERNAL_SAMPLE_RATE / (1 + SMPLRT_DIV) Where INTERNAL_SAMPLE_RATE = 1 kHz
  //SMPLRT_DIV=7->Output Data Rate = 100Hz
  write_I2C(IMU_ADDRESS, 0x19, 0b0000);  //0b00001001);

  // ACCEL_CONFIG2 register (9.13 REGISTER 29 – ACCELEROMETER CONFIGURATION 2 - Pag36)
  //Averaging filter settings for Low Power Accelerometer mode [5:4]: 0 = Average 4 samples 1 = Average 8 samples 2 = Average 16 samples 3 = Average 32 samples
  //Accelerometer low pass filter setting: ACCEL_FCHOICE_B[3] and A_DLPF_CFG[2:0]- Table 13 Pag36
  write_I2C(IMU_ADDRESS, 0x1D, 0b00100111);

  // GYRO_CONFIG register (9.11 REGISTER 27 – GYROSCOPE CONFIGURATION - Pag35)
  // Gyro Full Scale Select [4:3]: 00 = ±250 dps 01= ±500 dps 10 = ±1000 dps 11 = ±2000 dps
  write_I2C(IMU_ADDRESS, 0x1B, 0b00000000);

  // ACCEL_CONFIG register (9.12 REGISTER 28 – ACCELEROMETER CONFIGURATION - Pag35)
  // Accel Full Scale Select [4:3]: ±2g (00), ±4g (01), ±8g (10), ±16g (11)

  switch (accelerometer_scale) {
    case 2:  // ±2g
      write_I2C(IMU_ADDRESS, 0x1C, 0b00000000);
      break;
    case 4:  // ±4g
      write_I2C(IMU_ADDRESS, 0x1C, 0b00001000);
      break;
    case 8:  // ±8g
      write_I2C(IMU_ADDRESS, 0x1C, 0b00010000);
      break;
    case 16:  // ±16g
      write_I2C(IMU_ADDRESS, 0x1C, 0b00011000);
      break;
    default:
      // Default case, should not happen if accelerometer_scale is correctly set
      break;
  }
}

void imu_config_sleepmode() {

  //Resetting the IMU
  write_I2C(IMU_ADDRESS, 0x6B, 0b10000000);  // PWR_MGMT_1 register
  vTaskDelay(pdMS_TO_TICKS(200));            //Time for inciating again

  //Starting to Configure for sleeping mode
  write_I2C(IMU_ADDRESS, 0x37, 0b00000000);

  // Step 1: Ensure that Accelerometer is running
  write_I2C(IMU_ADDRESS, 0x6B, 0b00000001);  // PWR_MGMT_1 register
  write_I2C(IMU_ADDRESS, 0x6C, 0b00000111);  // PWR_MGMT_2 register

  // Step 2: Accelerometer Configuration
  write_I2C(IMU_ADDRESS, 0x1D, 0b00000001);  // ACCEL_CONFIG2 register

  // Step 3: Enable Motion Interrupt
  write_I2C(IMU_ADDRESS, 0x38, 0b11100000);  // INT_ENABLE register

  // Step 4: Set Motion Threshold
  write_I2C(IMU_ADDRESS, 0x1F, 0b00000010);  // ACCEL_WOM_THR register

  // Step 5: Enable Accelerometer Hardware Intelligence
  write_I2C(IMU_ADDRESS, 0x69, 0b11000000);  // ACCEL_INTEL_CTRL register- [7] ACCEL_INTEL_EN This bit enables the Wake-on-Motion detection logic.

  // Step 6: Set Frequency of Wake-Up
  // Assuming a sample rate of 500 Hz for demonstration
  write_I2C(IMU_ADDRESS, 0x19, 0b00000001);  // SMPLRT_DIV register

  // Step 7: Enable Cycle Mode (Accelerometer Low-Power Mode)
  write_I2C(IMU_ADDRESS, 0x6B, 0b00101001);  // PWR_MGMT_1 register
}



//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//Function to read the accelerometer values of the IAM-20680 Motion Sensor
//REGISTERS 59 TO 64 – ACCELEROMETER MEASUREMENTS (Pag.39 a 41)

//Registers 59 and 60: ACCEL_XOUT_H and ACCEL_XOUT_L
//Registers 61 and 62: ACCEL_YOUT_H and ACCEL_YOUT_L
//Registers 63 and 64: ACCEL_ZOUT_H and ACCEL_ZOUT_L

void imu_reading(float& accelX_g, float& accelY_g, float& accelZ_g) {
  // Read XOUT_H from the IMU
  Wire.beginTransmission(IMU_ADDRESS);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(IMU_ADDRESS, 6, true);  // request a total of 6 registers

  int16_t accelX = Wire.read() << 8 | Wire.read();  // ACCEL_XOUT
  int16_t accelY = Wire.read() << 8 | Wire.read();  // ACCEL_YOUT
  int16_t accelZ = Wire.read() << 8 | Wire.read();  // ACCEL_ZOUT

  Wire.endTransmission(true);

  // Convert the values to Gravitational Units (g)
  //Sensitivity Scale Factor (Pag.10)
  //AFS_SEL=0 (±2g) 16,384 LSB/g3 --- AFS_SEL=1 (±4g) 8,192 LSB/g 3 --- AFS_SEL=2 (±8g) 4,096 LSB/g 3 --- AFS_SEL=3 (±16g) 2,048 LSB/g 3

  switch (accelerometer_scale) {
    case 2:  // ±2g
      accelX_g = accelX / 16384.0;
      accelY_g = accelY / 16384.0;
      accelZ_g = accelZ / 16384.0;
      break;
    case 4:  // ±4g
      accelX_g = accelX / 8192.0;
      accelY_g = accelY / 8192.0;
      accelZ_g = accelZ / 8192.0;
      break;
    case 8:  // ±8g
      accelX_g = accelX / 4096.0;
      accelY_g = accelY / 4096.0;
      accelZ_g = accelZ / 4096.0;
      break;
    case 16:  // ±16g
      accelX_g = accelX / 2048.0;
      accelY_g = accelY / 2048.0;
      accelZ_g = accelZ / 2048.0;
      break;
    default:
      // Default case, should not happen if accelerometer_scale is correctly set
      accelX_g = accelY_g = accelZ_g = 0.0;
      break;
  }

  /*Serial.print(accelX_g);
  Serial.print(",");
  Serial.print(accelY_g);
  Serial.print(",");
  Serial.println(accelZ_g);*/
}
