void config_esp() {

  // Verifies if there is a config file in internal memory
  if (check_file(LittleFS, "/config_1.txt") && check_file(LittleFS, "/config_2.txt")) {

    if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
      Serial.println("LittleFS Mount Failed");
    }

    // Open the file
    File configFile = LittleFS.open("/config_1.txt", FILE_READ);
    if (!configFile) {
      Serial.println("Failed to open config file");
    }

    // Calculate the size of the file
    size_t size = configFile.size();
    if (size == 0) {
      Serial.println("Config file is empty");
      configFile.close();
      return;
    }

    // Create a buffer to hold the file contents
    std::unique_ptr<char[]> buf(new char[size + 1]);  // +1 to allow for null terminator
    configFile.readBytes(buf.get(), size);
    buf[size] = '\0';  // Null-terminate the string

    // Parse the JSON from the buffer
    DynamicJsonDocument doc(1024);  // Ensure this size is enough for your JSON document
    DeserializationError error = deserializeJson(doc, buf.get());
    if (error) {
      Serial.print("Failed to parse config file: ");
      Serial.println(error.c_str());
    } else {
      // Use the JSON document as needed
      Serial.println("JSON parsed successfully!");
      serializeJsonPretty(doc, Serial);  // Output the parsed JSON to Serial
    }

    configFile.close();  // Close the file

    //Assign the variables
    if (doc.containsKey("mqtt_address") && doc["mqtt_address"].is<const char*>()) {
      const char* mqtt_address = doc["mqtt_address"];  // Explicitly getting the value as a const char*
      adress_broker_mqtt = String(mqtt_address);       // Now explicitly creating a String from a const char*
    }
    if (doc.containsKey("mqtt_port") && doc["mqtt_port"].is<int>()) {
      port_broker_mqtt = doc["mqtt_port"];
    }
    if (doc.containsKey("sleep_mode") && doc["sleep_mode"].is<bool>()) {
      sleep_mode = doc["sleep_mode"];
    }
    if (doc.containsKey("sleep_interval") && doc["sleep_interval"].is<long>()) {
      interval_to_sleep_mode = doc["sleep_interval"];
      interval_to_sleep_mode = interval_to_sleep_mode * 60 * 1000;
    }
    if (doc.containsKey("movement_detection_threshold") && doc["movement_detection_threshold"].is<float>()) {
      movement_detection_threshold = doc["movement_detection_threshold"];
    }
    if (doc.containsKey("bme_mode") && doc["bme_mode"].is<bool>()) {
      bme_mode = doc["bme_mode"];
    }
    if (doc.containsKey("bme_sampling_interval") && doc["bme_sampling_interval"].is<long>()) {
      bme_interval_timer = doc["bme_sampling_interval"];
      bme_interval_timer = bme_interval_timer * 60 * 1000;
    }
    if (doc.containsKey("bme_sampling_duration") && doc["bme_sampling_duration"].is<int>()) {
      interval_bme_recording = doc["bme_sampling_duration"];
    }
    if (doc.containsKey("gps_mode") && doc["gps_mode"].is<bool>()) {
      gps_mode = doc["gps_mode"];
    }
    if (doc.containsKey("gps_interval_timer") && doc["gps_interval_timer"].is<int>()) {
      gps_interval_timer = doc["gps_interval_timer"];
      gps_interval_timer = gps_interval_timer * 60 * 1000;
    }


    // Open the file
    configFile = LittleFS.open("/config_2.txt", FILE_READ);
    if (!configFile) {
      Serial.println("Failed to open config file");
    }

    // Calculate the size of the file
    size = configFile.size();
    if (size == 0) {
      Serial.println("Config file is empty");
      configFile.close();
      return;
    }

    // Create a buffer to hold the file contents
    std::unique_ptr<char[]> buf2(new char[size + 1]);  // +1 to allow for null terminator
    configFile.readBytes(buf2.get(), size);
    buf2[size] = '\0';  // Null-terminate the string

    // Parse the JSON from the buffer
    error = deserializeJson(doc, buf2.get());
    if (error) {
      Serial.print("Failed to parse config file: ");
      Serial.println(error.c_str());
    } else {
      // Use the JSON document as needed
      Serial.println("JSON parsed successfully!");
      serializeJsonPretty(doc, Serial);  // Output the parsed JSON to Serial
    }

    configFile.close();  // Close the file


    if (doc.containsKey("max_time_try_location") && doc["max_time_try_location"].is<int>()) {
      max_time_try_location = doc["max_time_try_location"];
    }
    if (doc.containsKey("accelerometer_scale") && doc["accelerometer_scale"].is<int>()) {
      accelerometer_scale = doc["accelerometer_scale"];
    }
    if (doc.containsKey("sample_rate") && doc["sample_rate"].is<int>()) {
      sample_rate = doc["sample_rate"];
    }
    if (doc.containsKey("impact_mode") && doc["impact_mode"].is<bool>()) {
      impact_mode = doc["impact_mode"];
    }
    if (doc.containsKey("impact_threshold") && doc["impact_threshold"].is<float>()) {
      impact_threeshold = doc["impact_threshold"];
    }
    if (doc.containsKey("vibration_mode") && doc["vibration_mode"].is<bool>()) {
      vibration_mode = doc["vibration_mode"];
    }
    if (doc.containsKey("n_vibration_per_sample") && doc["n_vibration_per_sample"].is<int>()) {
      n_vibration_per_sample = doc["n_vibration_per_sample"];
    }
    if (doc.containsKey("vibration_threshold") && doc["vibration_threshold"].is<float>()) {
      vibration_magnitude_threshold = doc["vibration_threshold"];
    }
    if (doc.containsKey("vibration_sampling_interval") && doc["vibration_sampling_interval"].is<long>()) {
      vibration_interval_timer = doc["vibration_sampling_interval"];
      vibration_interval_timer = vibration_interval_timer * 1000;  // Convert minutes to milliseconds
    }
    if (doc.containsKey("mqtt_mode") && doc["mqtt_mode"].is<bool>()) {
      mqtt_mode = doc["mqtt_mode"];
    }
    if (doc.containsKey("mqtt_interval") && doc["mqtt_interval"].is<long>()) {
      long mqtt_interval_timer = doc["mqtt_interval"];
      mqtt_interval_timer = mqtt_interval_timer * 60 * 1000;
    }
    if (doc.containsKey("logs_mode") && doc["logs_mode"].is<bool>()) {
      logs_mode = doc["logs_mode"];
    }
    if (doc.containsKey("esp_stats_mode") && doc["esp_stats_mode"].is<bool>()) {
      esp_stats_mode = doc["esp_stats_mode"];
    }
    if (doc.containsKey("esp_stats_interval_timer") && doc["esp_stats_interval_timer"].is<long>()) {
      esp_stats_interval_timer = doc["esp_stats_interval_timer"];
      esp_stats_interval_timer = esp_stats_interval_timer * 60 * 1000;  // Convert minutes to milliseconds
    }

    //Turn off the LED PIN
    digitalWrite(LED_PIN, LOW);
    LittleFS.end();
    return;
  }

  //If there is no file in internal memory, it will try to obtain on MQTT Server
  if (connection_configurated_sucessfully == false) {
    for (int i = 0; i < 5; i++) {

      //Waking up the A9G
      COM_Config("esp-manager-task");
      vTaskDelay(pdMS_TO_TICKS(1000));

      //Configurating the Network
      if (COM_Config_Network()) {
        connection_configurated_sucessfully = true;
        break;
      } else if (i == 3) {
        log(FATAL_ERROR, "esp-manager-task", "A9G cannot configure to the server after 5 times trying");
        connection_configurated_sucessfully == false;
      }
      vTaskDelay(pdMS_TO_TICKS(2000));
    }
  }

  vTaskDelay(pdMS_TO_TICKS(1000));

  if (connection_configurated_sucessfully == true) {
    //Subscribes the topic "config" and waits for config files
    SerialPortA9G.println("AT+MQTTSUB=\"config1\",1,0");
    if (!check_response_A9G(20000, "OK")) {
      Serial.println("Failed to send MQTT setence");
      return;
    }
    Serial.println("Subscribed to config1 topic!");
    xTaskCreate(blinkLEDTask, "Blink LED Task", 1024, NULL, 1, &led_blink_task_handle);  // Create the task
  }

  int startTime = millis();
  while ((millis() - startTime) < 120000) {

    String message_read = "";
    if (SerialPortA9G.available()) {
      while (SerialPortA9G.available()) {
        char char_read = SerialPortA9G.read();  // Read one character from the serial port
        message_read += char_read;              // Construction of the message
      }
      Serial.println(message_read);
    }
    delay(1000);

    if (message_read.indexOf("+MQTTPUBLISH:") != -1) {

      int jsonStartIndex = message_read.indexOf('{');
      int jsonEndIndex = message_read.lastIndexOf('}');

      if (jsonStartIndex != -1 && jsonEndIndex != -1) {
        String jsonStr = message_read.substring(jsonStartIndex, jsonEndIndex + 1);
        Serial.println(jsonStr);

        StaticJsonDocument<512> doc;  // Adjust size based on expected JSON size
        DeserializationError error = deserializeJson(doc, jsonStr.c_str());

        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.c_str());
        }

        if (!doc.isNull()) {
          if (doc.containsKey("device_id") && doc["device_id"].is<const char*>()) {
            const char* device_id = doc["device_id"];
            if (strcmp(device_id, data_logger_id.c_str()) == 0) {
              Serial.println("Configs Received correctly!!");
              if (doc.containsKey("mqtt_address") && doc["mqtt_address"].is<const char*>()) {
                const char* mqtt_address = doc["mqtt_address"];  // Explicitly getting the value as a const char*
                adress_broker_mqtt = String(mqtt_address);       // Now explicitly creating a String from a const char*
              }
              if (doc.containsKey("mqtt_port") && doc["mqtt_port"].is<int>()) {
                port_broker_mqtt = doc["mqtt_port"];
              }
              if (doc.containsKey("sleep_mode") && doc["sleep_mode"].is<bool>()) {
                sleep_mode = doc["sleep_mode"];
              }/*
              if (doc.containsKey("start_time") && doc["start_time"].is<bool>()) {
                start_time = doc["start_time"];
              }*/
              if (doc.containsKey("sleep_interval") && doc["sleep_interval"].is<long>()) {
                interval_to_sleep_mode = doc["sleep_interval"];
                interval_to_sleep_mode = interval_to_sleep_mode * 60 * 1000;
              }
              if (doc.containsKey("movement_detection_threshold") && doc["movement_detection_threshold"].is<float>()) {
                movement_detection_threshold = doc["movement_detection_threshold"];
              }
              if (doc.containsKey("bme_mode") && doc["bme_mode"].is<bool>()) {
                bme_mode = doc["bme_mode"];
              }
              if (doc.containsKey("bme_sampling_interval") && doc["bme_sampling_interval"].is<long>()) {
                bme_interval_timer = doc["bme_sampling_interval"];
                bme_interval_timer = bme_interval_timer * 60 * 1000;
              }
              if (doc.containsKey("bme_sampling_duration") && doc["bme_sampling_duration"].is<int>()) {
                interval_bme_recording = doc["bme_sampling_duration"];
              }
              if (doc.containsKey("gps_mode") && doc["gps_mode"].is<bool>()) {
                gps_mode = doc["gps_mode"];
              }
              if (doc.containsKey("gps_interval_timer") && doc["gps_interval_timer"].is<int>()) {
                gps_interval_timer = doc["gps_interval_timer"];
                gps_interval_timer = gps_interval_timer * 60 * 1000;
              }

              //Open the file
              int counter = 0;
              if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
                Serial.println("LittleFS Mount Failed");
              } else {
                Serial.println("LittleFS Started");
              }

              while (1) {
                counter++;
                File file = LittleFS.open("/config_1.txt", FILE_WRITE);
                if (!file) {
                  log(ERROR, "config-esp", "Failed to open file for reading");
                  file.close();
                } else {
                  // Serialize JSON to file
                  if (serializeJson(doc, file) == 0) {
                    Serial.println("Failed to write to config file");
                  } else {
                    Serial.println("Configuration saved successfully");
                    break;
                  }
                  // Close the file
                  file.close();
                }
                if (counter == 30) {
                  log(FATAL_ERROR, "config-esp", "Failed to open file for reading 30x");
                  return;
                }
                vTaskDelay(pdMS_TO_TICKS(1000));
              }
            }
            break;

            //Start Date Verification
            

          } else {
            Serial.println("Device ID does not match!");
          }
        } else {
          Serial.println("Device ID missing or incorrect type!");
        }
      } else {
        Serial.println("Received null JSON object!");
      }
    }
  }

  if ((millis() - startTime) >= 120000) {
    Serial.println("Receiving Information Timeout");
  }

  //Subscribes the topic "config" and waits for config files
  SerialPortA9G.println("AT+MQTTSUB=\"config1\",0,0");
  if (!check_response_A9G(20000, "OK")) {
    Serial.println("Failed to send MQTT setence");
    return;
  }
  Serial.println("Unsubscribed to config1 topic!");

  delay(1000);

  SerialPortA9G.println("AT+MQTTSUB=\"config2\",1,0");
  if (!check_response_A9G(20000, "OK")) {
    Serial.println("Failed to send MQTT setence");
    return;
  }
  Serial.println("Subscribed to config2 topic!");


  startTime = millis();
  while ((millis() - startTime) < 60000) {

    String message_read = "";
    if (SerialPortA9G.available()) {
      while (SerialPortA9G.available()) {
        char char_read = SerialPortA9G.read();  // Read one character from the serial port
        message_read += char_read;              // Construction of the message
      }
      Serial.println(message_read);
    }
    delay(1000);

    if (message_read.indexOf("+MQTTPUBLISH:") != -1) {

      int jsonStartIndex = message_read.indexOf('{');
      int jsonEndIndex = message_read.lastIndexOf('}');

      if (jsonStartIndex != -1 && jsonEndIndex != -1) {
        String jsonStr = message_read.substring(jsonStartIndex, jsonEndIndex + 1);
        Serial.println(jsonStr);

        StaticJsonDocument<512> doc;  // Adjust size based on expected JSON size
        DeserializationError error = deserializeJson(doc, jsonStr.c_str());

        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.c_str());
        }

        if (!doc.isNull()) {
          if (doc.containsKey("device_id") && doc["device_id"].is<const char*>()) {
            const char* device_id = doc["device_id"];
            if (strcmp(device_id, data_logger_id.c_str()) == 0) {
              Serial.println("Configs Received correctly!!");

              // Processing new settings
              if (doc.containsKey("max_time_try_location") && doc["max_time_try_location"].is<int>()) {
                max_time_try_location = doc["max_time_try_location"];
              }
              if (doc.containsKey("accelerometer_scale") && doc["accelerometer_scale"].is<int>()) {
                accelerometer_scale = doc["accelerometer_scale"];
                Serial.println(accelerometer_scale);
              }
              if (doc.containsKey("sample_rate") && doc["sample_rate"].is<int>()) {
                sample_rate = doc["sample_rate"];
                Serial.println(sample_rate);
              }
              if (doc.containsKey("impact_mode") && doc["impact_mode"].is<bool>()) {
                impact_mode = doc["impact_mode"];
              }
              if (doc.containsKey("impact_threshold") && doc["impact_threshold"].is<float>()) {
                impact_threeshold = doc["impact_threshold"];
              }
              if (doc.containsKey("vibration_mode") && doc["vibration_mode"].is<bool>()) {
                vibration_mode = doc["vibration_mode"];
              }
              if (doc.containsKey("n_vibration_per_sample") && doc["n_vibration_per_sample"].is<int>()) {
                n_vibration_per_sample = doc["n_vibration_per_sample"];
              }
              if (doc.containsKey("vibration_threshold") && doc["vibration_threshold"].is<float>()) {
                vibration_magnitude_threshold = doc["vibration_threshold"];
              }
              if (doc.containsKey("vibration_sampling_interval") && doc["vibration_sampling_interval"].is<long>()) {
                vibration_interval_timer = doc["vibration_sampling_interval"];
                vibration_interval_timer = vibration_interval_timer * 1000;  // Convert minutes to milliseconds
              }
              if (doc.containsKey("mqtt_mode") && doc["mqtt_mode"].is<bool>()) {
                mqtt_mode = doc["mqtt_mode"];
              }
              if (doc.containsKey("mqtt_interval") && doc["mqtt_interval"].is<long>()) {
                long mqtt_interval_timer = doc["mqtt_interval"];
                mqtt_interval_timer = mqtt_interval_timer * 60 * 1000;
              }
              if (doc.containsKey("logs_mode") && doc["logs_mode"].is<bool>()) {
                logs_mode = doc["logs_mode"];
              }
              if (doc.containsKey("esp_stats_mode") && doc["esp_stats_mode"].is<bool>()) {
                esp_stats_mode = doc["esp_stats_mode"];
              }
              if (doc.containsKey("esp_stats_interval_timer") && doc["esp_stats_interval_timer"].is<long>()) {
                esp_stats_interval_timer = doc["esp_stats_interval_timer"];
                esp_stats_interval_timer = esp_stats_interval_timer * 60 * 1000;  // Convert minutes to milliseconds
              }

              // Open file for writing
              File configFile = LittleFS.open("/config_2.txt", FILE_WRITE);
              if (!configFile) {
                Serial.println("Failed to open config file for writing");
              }

              // Serialize JSON to file
              if (serializeJson(doc, configFile) == 0) {
                Serial.println("Failed to write to config file");
              } else {
                Serial.println("Configuration saved successfully");
              }
              // Close the file
              configFile.close();

              break;
            } else {
              Serial.println("Device ID does not match!");
            }
          } else {
            Serial.println("Device ID missing or incorrect type!");
          }
        } else {
          Serial.println("Received null JSON object!");
        }
      }
    }
  }

  if ((millis() - startTime) >= 60000) {
    Serial.println("Receiving Information Timeout");
  }
  connection_configurated_sucessfully = false;

  LittleFS.end();

  vTaskDelete(led_blink_task_handle);
  digitalWrite(LED_PIN, HIGH);
  delay(3000);
  digitalWrite(LED_PIN, LOW);
}



void waitForTaskNotSuspended(TaskHandle_t taskHandle) {

  if (taskHandle == NULL) {
    if (mqtt_communication_task_handle == NULL) {
      xTaskCreatePinnedToCore(mqtt_communication_task, "Send the values from the microSD to MQTT", 12000, NULL, 10, &mqtt_communication_task_handle, 0);
      taskHandle = mqtt_communication_task_handle;
    } else if (sd_communication_task_handle == NULL) {
      xTaskCreatePinnedToCore(sd_communication_task, "Saving Values on the SD Card", 8000, NULL, 10, &sd_communication_task_handle, 0);
      taskHandle = sd_communication_task_handle;
    }
  }

  if (taskHandle != NULL) {
    vTaskResume(taskHandle);
    while (eTaskGetState(taskHandle) != eSuspended) {
      vTaskDelay(pdMS_TO_TICKS(5000));
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void deleteTask(TaskHandle_t* taskHandle) {
  if (*taskHandle != NULL) {
    vTaskDelete(*taskHandle);
    *taskHandle = NULL;
  }
}

void turn_off_procedure() {
  // Delete tasks safely
  deleteTask(&imu_reading_task_handle);
  deleteTask(&vibration_processing_task_handle);
  deleteTask(&bme_reading_task_handle);
  deleteTask(&gps_manager_task_handle);
  deleteTask(&esp_stats_task_handle);

  // Wait for sd_communication_task to be not suspended
  waitForTaskNotSuspended(sd_communication_task_handle);

  // Wait for mqtt_communication_task to be not suspended
  //waitForTaskNotSuspended(mqtt_communication_task_handle);

  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  //Delete the config files
  delete_file(LittleFS, "/config_1.txt");
  delete_file(LittleFS, "/config_2.txt");

  //Delete the temp files
  delete_file(LittleFS, "/bme_data_temp.txt");
  delete_file(LittleFS, "/impact_data_temp.txt");
  //delete_file(LittleFS, "/vibration_data_temp.txt");
  delete_file(LittleFS, "/esp_stats_temp.txt");

  LittleFS.end();
}
