/////////////////////////////////////////////////////////////////////
/////////////////////   I2C FUNCTIONS   /////////////////////////////

//Function to write a value in a register adress of a slave device by using I2C communication.
void write_I2C(int address, int registerAddress, int value) {
  Wire.beginTransmission(address);  // Start the transmission
  Wire.write(registerAddress);      // write the register adress
  Wire.write(value);                // Write the value
  Wire.endTransmission(true);       // End Transmission
}

//Function to read a value in a register adress of a slave device by using I2C communication.
void read_I2C(int address, int registerAddress) {
  // Read XOUT_H from the IMU
  Wire.beginTransmission(address);
  Wire.write(registerAddress);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(address, 1, true);  // request a total of 6 registers
  int16_t value = Wire.read();
  Wire.endTransmission(true);

  //For debugging
  Serial.print("Register Address: ");
  Serial.println(registerAddress, HEX);
  Serial.print("Value: ");
  Serial.println(value, BIN);
}

/////////////////////////////////////////////////////////////////////
///////////   GSM COMMUNICATION FUNCTIONS (A9G)  ////////////////////

//Function to configure the A9G and the gsm network

void COM_Config(String task) {

  String receivedMessage = "";
  int startTime = 0;

  //INITIATES THE A9G
  log(INFO, task, "Initiating the A9G Config doing RST");

  startTime = millis();
  SerialPortA9G.println("AT+RST=1");
  /*Expecting this:
  Init... +CREG: 2 +CTZV:18/03/30,06:59:43,+08 +CREG: 1 +CTZV:18/03/30,06:59:44,+08 OK A9/A9G V02.00.20180327RC Ai_Thinker_Co.LTD READY*/

  while ((millis() - startTime) < 60000) {
    String receivedMessage = COM_Read_A9G();

    if (receivedMessage.indexOf("+CTZV:") != -1) {
      //+CTZV:18/03/30,06:59:44,+08

      //Dividing the string to only the date and hour
      int startIndex = receivedMessage.indexOf("+CTZV:");
      int endIndex = startIndex + 27;  // Length of "+CTZV:18/03/30,06:59:44,+08"
      String receivedMessage_Time = receivedMessage.substring(startIndex, endIndex);

      /*For debugging:
      Serial.println("year: " + receivedMessage_Time.substring(6, 8));
      Serial.println("month: " + receivedMessage_Time.substring(9, 11));
      Serial.println("day: " + receivedMessage_Time.substring(12, 14));

      Serial.println("hour: " + receivedMessage_Time.substring(15, 17));
      Serial.println("minutes: " + receivedMessage_Time.substring(18, 20));
      Serial.println("seconds: " + receivedMessage_Time.substring(21, 23));*/

      //Defining the internal real-time clock
      rtc.setTime(receivedMessage_Time.substring(21, 23).toInt(),
                  receivedMessage_Time.substring(18, 20).toInt(),
                  receivedMessage_Time.substring(15, 17).toInt(),
                  receivedMessage_Time.substring(12, 14).toInt(),
                  receivedMessage_Time.substring(9, 11).toInt(),
                  (receivedMessage_Time.substring(6, 8).toInt() + 2000));

      Serial.println("Rtc is synchronized: " + rtc.getDateTime(true));
    }

    if (receivedMessage.indexOf("READY") != -1) {
      log(INFO, task, "A9G READY for receive AT Commands!");
      break;  // Exit the loop if "OK" is found
    }
    delay(100);
  }
  if ((millis() - startTime) >= 30000) {
    log(ERROR, task, "AT+RST Timeout");
  }

  vTaskDelay(pdMS_TO_TICKS(1000));
}

///////////////////////    A9G Network Config   /////////////////////////////////////
// Main function to configure the network
bool COM_Config_Network() {

  SerialPortA9G.println("AT+CGATT=1");
  if (!check_response_A9G(5000, "OK")) {
    log(ERROR, "mqtt-communication-task", "Internet Permissions Denied!");
    return false;
  }

  log(INFO, "mqtt-communication-task", "Internet Permissions OK!");
  vTaskDelay(pdMS_TO_TICKS(1000));

  SerialPortA9G.println("AT+CGDCONT=1,\"IP\",\"" + apn + "\"");
  if (!check_response_A9G(5000, "OK")) {
    log(ERROR, "mqtt-communication-task", "Configuration of the APN Server not succeeded!");
    return false;
  }

  /*SerialPortA9G.println("AT+CGDCONT=1,\"IP\",\"net2.vodafone.pt\"");
  if (!check_response_A9G(5000, "OK")) {
    log(ERROR, "mqtt_communication_task", "Configuration of the APN Server not successed!");
    return false;
  }*/

  log(INFO, "mqtt-communication-task", "Configuration of the APN Server successed!");
  vTaskDelay(pdMS_TO_TICKS(1000));

  SerialPortA9G.println("AT+CGACT=1,1");
  if (!check_response_A9G(5000, "OK")) {
    log(ERROR, "mqtt-communication-task", "Connection to the APN Server not successed!");
    return false;
  }

  log(INFO, "mqtt-communication-task", "Connection to the APN Server successed!");
  vTaskDelay(pdMS_TO_TICKS(1000));

  // Configuration of the MQTT Server
  SerialPortA9G.println("AT+MQTTCONN=\"" + adress_broker_mqtt + "\"," + port_broker_mqtt + ",\"" + data_logger_id + "\",120,0");
  if (!check_response_A9G(10000, "OK")) {
    log(ERROR, "mqtt-communication-task", "Connection not sucessfully with the MQTT Server!");
    return false;
  }

  log(INFO, "mqtt-communication-task", "MQTT Server Connected!");
  vTaskDelay(pdMS_TO_TICKS(1000));

  return true;
}


/////////////////// Function to send a MQTT Setence ///////////////////////////

bool COM_send_MQTT(String data_source, String data) {
  String MQTT_setence = "%" + data_logger_id + "%";
  MQTT_setence += data_source + ",";
  MQTT_setence += data;

  Serial.println(MQTT_setence);

  SerialPortA9G.println("AT+MQTTPUB=\"" + data_source + "\",\"" + MQTT_setence + "\",0,0,0");
  if (!check_response_A9G(20000, "OK")) {
    Serial.println("Failed to send MQTT setence");
    return false;
  }

  vTaskDelay(pdMS_TO_TICKS(500));

  /*/ Turning off the MQTT Server
  SerialPortA9G.println("AT+MQTTDISCONN");
  if (!check_response_A9G(10000,"OK")) {
    Serial.println("Failed to disconnect from MQTT Server");
    return false;
  }*/

  return true;
}

///////////////////////////////////////////////////////////////////////////////

//Function to read the Serial Port from A9G
String COM_Read_A9G() {
  if (SerialPortA9G.available()) {
    String message_read = "";
    while (SerialPortA9G.available()) {
      char char_read = SerialPortA9G.read();  // Read one character from the serial port
      message_read += char_read;              //Construction of the message
    }
    return message_read;
  }
}

// Function to check for a specific response
bool check_response_A9G(int timeout, String expectedResponse) {
  int startTime = millis();
  while ((millis() - startTime) < timeout) {
    String receivedMessage = COM_Read_A9G();
    if (receivedMessage.indexOf(expectedResponse) != -1) {
      return true;
    }
  }
  return false;
}

///////////////// SD TO MQTT Function //////////////////////////////

bool sd_to_mqtt(fs::FS& fs, const char* path, String topic, int n_paragraphs_to_send) {

  String message = "";
  int n_paragraphs = 0;
  int max_characters_per_paragraph = 10000;
  int n_characters_per_paragraph = 0;
  int n_failling_MQTT = 0;
  unsigned long last_pos_paragraph = 0;
  unsigned long size_file = 0;
  int char_counter = 0;
  bool segment_processed_successfully = false;


  //this cycle will divide the file into segments and send the segments individually
  while (true) {

    message = "";
    int counter = 0;

    //Start the connection to the internal memory
    while (1) {
      counter++;
      if (!LittleFS.begin()) {
        log(ERROR, "mqtt-communication-task", "SD card mount failed");
      } else {
        break;
      }
      vTaskDelay(pdMS_TO_TICKS(1000));
      if (counter == 30) {
        log(FATAL_ERROR, "mqtt-communication-task", "LittleFS mount Failed 30x");
        return false;
      }
    }

    File file;
    counter = 0;

    //Open the file
    while (1) {
      counter++;
      file = fs.open(path, FILE_READ);
      if (!file) {
        log(ERROR, "mqtt-communication-task", "Failed to open file for reading");
        file.close();
      } else {
        break;
      }
      if (counter == 30) {
        log(FATAL_ERROR, "mqtt-communication-task", "Failed to open file for reading 30x");
        return false;
      }
      vTaskDelay(pdMS_TO_TICKS(1000));
    }


    size_file = file.size();
    file.seek(last_pos_paragraph);

    // Starts to record char by char until the number defined of paragraphs, an reading error or when the file ends
    while (file.available()) {
      char character = file.read();  // Read each character from the file

      if (character == '\n') {
        n_paragraphs++;
        n_characters_per_paragraph = 0;
        message += "_";  // Replace newline characters with underscores
        last_pos_paragraph = file.position();
        char_counter = 0;

      } else if (character == ' ') {
        // Do nothing for spaces
      } else {
        message += character;
        n_characters_per_paragraph++;
      }

      if (n_characters_per_paragraph >= max_characters_per_paragraph) {
        log(ERROR, "mqtt-communication-task", "File Reading Fail, too much characters on the same paragraph");
        segment_processed_successfully = false;
        message = "";
        break;
      }

      if (n_paragraphs == n_paragraphs_to_send) {
        segment_processed_successfully = true;
        break;
      }
    }

    file.close();
    LittleFS.end();

    //Verifies if exist any last data that didnt fill the segment
    if (message.length() > 0) {
      segment_processed_successfully = true;
    }

    //Send the segment by MQTT, if is correctly written or last data
    if (segment_processed_successfully == true) {
      counter = 0;

      while (1) {
        counter++;

        String MQTT_sentence = "%" + data_logger_id + "%";
        MQTT_sentence += topic + ",";
        MQTT_sentence += message;

        Serial.println("AT+MQTTPUB=\"" + topic + "\",\"" + MQTT_sentence + "\",0,0,0");
        SerialPortA9G.println("AT+MQTTPUB=\"" + topic + "\",\"" + MQTT_sentence + "\",0,0,0");

        if (!check_response_A9G(20000, "OK")) {
          log(ERROR, "mqtt-communication-task", "Failed to send MQTT sentence");
        } else if (counter == 3) {
          for (int i = 0; i < 3; i++) {

            //Waking up the A9G
            COM_Config("mqtt_communication_task");
            vTaskDelay(pdMS_TO_TICKS(1000));

            //Configurating the Network
            if (COM_Config_Network()) {
              break;
            } else if (i == 3) {
              log(FATAL_ERROR, "mqtt-communication-task", "A9G cannot configure to the server after 3 times trying");
              return false;
            }
            vTaskDelay(pdMS_TO_TICKS(2000));
          }
        } else if (counter == 5) {
          log(FATAL_ERROR, "mqtt-communication-task", "Cannot send 5 times the MQTT Setence. Returning false!");
          return false;
        }

        else {
          message = "";
          n_paragraphs = 0;  // Reset paragraph count
          break;
        }
      }
    }

    //Verifies if it was the last char to exit from the loop
    if (last_pos_paragraph >= size_file) {
      break;
    }
  }

  return true;
}



/////////////////////////////////////////////////////////////////////
/////////////////////MICRO-SD FUNCTIONS /////////////////////////////

// Function to check for microSD card
bool checkMicroSDCard() {
  //Verifies if it has a physical card on the microsd module
  if (digitalRead(25) == 1) {
    Serial.println("Please insert a microSD card in the slot.");
    double initial_time = millis();
    while (1) {
      double actual_time = millis();
      if (digitalRead(25) == 0) {
        Serial.println("MicroSD card detected.");
        return true;
      } else if (actual_time - initial_time >= 60000) {
        Serial.println("Critical Error: MicroSD not available !!!");
        return false;
      } else {
        delay(500);
      }
    }
  }
  return true;
}

////////////////// testing sd ////////////////////////////77

void testing_sd() {

  //Verifies if it has a physical card on the microsd module
  if (digitalRead(25) == 1) {
    Serial.println("Please insert a microSD card in the slot.");
    double initial_time = millis();
    while (1) {
      double actual_time = millis();
      if (digitalRead(25) == 0) {
        Serial.println("MicroSD card detected.");
        break;
      } else {
        delay(500);
      }
    }
  }

  if (!SD.begin(SD_CS_PIN, serial_peripheral_interface, 1000000)) {  // Change the number depending on the CS pin you're using
    Serial.println("Card Mount Failed");
    return;
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }
  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SD");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  SD.end();
}


//////////////////// Apending Function ///////////////////////////////

bool append_file(fs::FS& fs, const char* path, const String& message) {
  //Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);

  if (!file) {
    Serial.println("Failed to open file for writing.");
    file.close();

    return false;
  }

  if (file.print(message)) {
    //Serial.println("File written.");
    file.close();
    return true;

  } else {
    Serial.println("Write failed.");
    file.close();
    return false;
  }
}

//////////////////// File Verification Function //////////////////////
//Verifies if exist a file
bool check_file(fs::FS& fs, const char* path) {
  int counter = 0;
  while (1) {
    counter++;
    if (!LittleFS.begin()) {
      log(ERROR, "mqtt-communication-task", "SD card mount failed");
    } else {
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    if (counter == 30) {
      log(FATAL_ERROR, "mqtt-communication-task", "LittleFS mount Failed 30x");
      return false;
    }
  }

  if (LittleFS.exists(path) == true) {
    Serial.println("File Exists");
    LittleFS.end();
    return true;
  } else {
    Serial.println("File doesnt Exists");
  }
  LittleFS.end();
  return false;
}


//////////////////// Apending Function ///////////////////////////////

bool append_file_testing(fs::FS& fs, const char* path, const char* message) {
  //Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);

  if (!file) {
    Serial.println("Failed to open file for writing.");
    file.close();

    return false;
  }

  if (file.print(message)) {
    //Serial.println("File written.");
    file.close();
    return true;

  } else {
    Serial.println("Write failed.");
    file.close();
    return false;
  }
}


//////////////////// Deleting Function ///////////////////////////////

bool delete_file(fs::FS& fs, const char* path) {

  int counter = 0;

  while (1) {
    counter++;
    if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
      log(ERROR, "mqtt-communication-task", "SD card mount failed");
    } else {
      break;
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
    if (counter == 30) {
      log(FATAL_ERROR, "mqtt-communication-task", "SD Card mount Failed 30x");
      return false;
    }
  }

  if (fs.remove(path)) {
    log(INFO, "mqtt-communication-task", "File Deleted");
    return true;
  } else {
    log(ERROR, "mqtt-communication-task", "File Not Deleted");
    return false;
  }

  LittleFS.end();
}




void printTaskState(TaskHandle_t taskHandle) {
  if (taskHandle == NULL) {
    return;
  } else {
    eTaskState taskState = eTaskGetState(taskHandle);
    switch (taskState) {
      case eReady:
        Serial.println("eReady");
        break;
      case eBlocked:
        Serial.println("eBlocked");
        break;
      case eSuspended:
        Serial.println("eSuspended");
        break;
      case eDeleted:
        Serial.println("eDeleted");
        break;
      case eInvalid:
        Serial.println("eInvalid");
        break;
      default:
        Serial.println("Unknown state");
        break;
    }
  }
}



void log(log_message_type type, String task, String message) {
  String data;

  data = String(rtc.getEpoch()) + ",";
  data += logMessageTypeToString(type) + ",";
  data += task + ",";
  data += message;

  //For view in the Serial Port
  Serial.println(data);


  if (logs_mode == true) {
    // Convert the String to a char array
    char char_to_send[data.length() + 1];  // +1 for the null terminator
    data.toCharArray(char_to_send, sizeof(char_to_send));
    // Attempt to add the log data to the queue
    if (xQueueSend(log_data_queue, &char_to_send, portMAX_DELAY) != pdTRUE) {
      Serial.println("Failed to add log data to queue.");
    }
  }
}

void deleteAllFiles(const char* dirname) {
  File dir = LittleFS.open(dirname);
  if (!dir.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = dir.openNextFile();
  while (file) {
    delete_file(LittleFS, file.name());
    file = dir.openNextFile();
  }
  dir.close();
}


String logMessageTypeToString(log_message_type type) {
  switch (type) {
    case INFO:
      return "INFO";
    case WARNING:
      return "WARNING";
    case ERROR:
      return "ERROR";
    case FATAL_ERROR:
      return "FATAL_ERROR";
    default:
      return "UNKNOWN";
  }
}