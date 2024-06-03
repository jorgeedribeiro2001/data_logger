//////////////////////    GPS Activation   /////////////////////////////////
void COM_GPS(bool order) {
  //If the order is true, is turning one the GPS. If false, turn off the gps
  long startTime;

  if (order == true) {
    //CONFIGURATION OF THE GPS
    SerialPortA9G.println("AT+GPS=1");
    startTime = millis();
    while ((millis() - startTime) < 5000) {
      String receivedMessage = COM_Read_A9G();
      if (receivedMessage.indexOf("OK") != -1) {
        Serial.println("GPS Turned ON!");
        break;  // Exit the loop if "OK" is found
      }
    }
  } else if (order == false) {
    //CONFIGURATION OF THE GPS
    SerialPortA9G.println("AT+GPS=0");
    startTime = millis();
    while ((millis() - startTime) < 5000) {
      String receivedMessage = COM_Read_A9G();
      if (receivedMessage.indexOf("OK") != -1) {
        Serial.println("GPS Turned OFF!");
        break;  // Exit the loop if "OK" is found
      }
    }
  } /* else if (order == LOW_POWER_MODE) {

      //AT+GPSLP= 0 Normal Mode
      //AT+GPSLP= 1 Low Power Consumption
      //AT+GPSLP= 2 Ultra-low power tracking mode

    }*/
  else {
    Serial.print("Happened a error trying to turn on/off the GPS");
  }
  delay(200);
}

//////////////   GPS Data Processing   ///////////////////////////////////

gps_data_struct Processing_GPS(String message_read) {

  gps_data_struct data;

  int startIndex = message_read.indexOf("$GNGGA");
  if (startIndex != -1) {
    //Serial.println(message_read);

    int firstComma = message_read.indexOf(',');
    int secondComma = message_read.indexOf(',', firstComma + 1);

    data.time = message_read.substring(firstComma + 1, secondComma).toInt();
    // Uses the sycronized rtc time (compare to a date in the past to asure if the rtc is sycronized with the gsm network)
    if (rtc.getEpoch() >= 1700000000) {
      data.time = rtc.getEpoch();
    }
    message_read.remove(0, secondComma + 1);

    secondComma = message_read.indexOf(',', 1);
    data.latitude = message_read.substring(0, secondComma);
    message_read.remove(0, secondComma + 1);

    secondComma = message_read.indexOf(',', 1);
    data.N_S = message_read.substring(0, secondComma);
    message_read.remove(0, secondComma + 1);

    secondComma = message_read.indexOf(',', 1);
    data.longitude = message_read.substring(0, secondComma);
    message_read.remove(0, secondComma + 1);

    secondComma = message_read.indexOf(',', 1);
    data.E_W = message_read.substring(0, secondComma);
    message_read.remove(0, secondComma + 1);

    secondComma = message_read.indexOf(',', 1);
    data.fix = message_read.substring(0, secondComma).toInt();
    message_read.remove(0, secondComma + 1);

    secondComma = message_read.indexOf(',', 1);
    data.satellites = message_read.substring(0, secondComma).toInt();
    message_read.remove(0, secondComma + 1);

    firstComma = message_read.indexOf(',', 0);
    secondComma = message_read.indexOf(',', 1);
    //Caso HDPO seja inexistente, ou seja = ",,"
    if (firstComma == 0) {
      data.HDOP = 0;
      message_read.remove(0, 1);
    } else {
      data.HDOP = message_read.substring(0, secondComma).toFloat();
      message_read.remove(0, secondComma + 1);
    }

    secondComma = message_read.indexOf(',', 1);
    data.altitude = message_read.substring(0, secondComma).toFloat();
    message_read.remove(0, secondComma + 1);

    secondComma = message_read.indexOf(',', 1);
    data.units = message_read.substring(0, secondComma);
    message_read.remove(0, secondComma + 1);

    secondComma = message_read.indexOf(',', 1);
    data.geoidalSeparation = message_read.substring(0, secondComma).toFloat();
    message_read.remove(0, secondComma + 1);

    secondComma = message_read.indexOf(',', 1);
    data.units2 = message_read.substring(0, secondComma);
    message_read.remove(0, secondComma + 1);

    /*/For debugging
          Serial.println("Time: " + data.time);
          Serial.println("Latitude: " + data.latitude);
          Serial.println("N_S: " + data.N_S);
          Serial.println("Longitude: " + data.longitude);
          Serial.println("E_W: " + data.E_W);
          Serial.println("Fix: " + data.fix);
          Serial.println("Satellites: " + data.satellites);
          Serial.println("HDOP: " + data.HDOP);
          Serial.println("Altitude: " + data.altitude);
          Serial.println("Units: " + data.units);
          Serial.println("Geoidal Separation: " + data.geoidalSeparation);
          Serial.println("Units2: " + data.units2);*/
  }
  return data;
}

// Function to check if the GPS is on or off
bool check_gps_status() {
  // Send command to check GPS status
  SerialPortA9G.println("AT+GPS?");
  unsigned long startTime = millis();

  while (millis() - startTime <= 6000) {
    String receivedMessage = COM_Read_A9G();

    if (receivedMessage.indexOf("+GPS: 1") != -1) {
      Serial.println("GPS Already Turned On!");
      return true;  // GPS is on

    } else if (receivedMessage.indexOf("+GPS: 0") != -1) {
      Serial.println("GPS is Turned OFF!");
      COM_GPS(true);  // Turning ON the GPS
      return false;   // GPS was off, but now it's on
    }
  }

  // If we reach here, it means the module did not respond within the timeout
  Serial.println("Error A9G: Not Responding to Commands! Trying to Restart the Module!");
  COM_Config("gps_manager_task");
  vTaskDelay(pdMS_TO_TICKS(1000));
  COM_GPS(true);

  return false;  // Return false indicating the GPS status is unknown or not successfully checked
}

//////////////////////////////////////////////////////////////////////////////
////////////////CONVERT FUNCTION GPS LATITUDE AND LONGITUDE///////////////////
//4036.6045,N,00844.9832,W -> Latitude: 40.6100769043 Longitude: -8.7497196198

double convertLatitude(String lat, String N_S) {
  // Parse the degrees and minutes from the input string
  int degrees = lat.substring(0, 2).toInt();
  float minutes = lat.substring(2).toFloat();

  // Convert the minutes to the decimal part of the degrees
  double decimalDegrees = degrees + (minutes / 60.0);

  // Adjust for the hemisphere (North or South)
  if (strcmp(N_S.c_str(), "S") == 0) {
    decimalDegrees = -decimalDegrees;
  }
  return decimalDegrees;
}

double convertLongitude(String lon, String E_W) {
  // Parse the degrees and minutes from the input string
  int degrees = lon.substring(0, 3).toInt();
  float minutes = lon.substring(3).toFloat();

  // Convert the minutes to the decimal part of the degrees
  double decimalDegrees = degrees + (minutes / 60.0);

  // Adjust for the hemisphere (East or West)
  if (E_W.equals("W")) {
    decimalDegrees = -decimalDegrees;
  }

  return decimalDegrees;
}

