#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_BME680.h>
#include <ESP32Time.h>
#include <HardwareSerial.h>
#include <esp_system.h>
#include <esp_adc_cal.h>
#include <LittleFS.h>
#include <Arduino.h>
#include <arduinoFFT.h>
#include <ArduinoJson.h>


//////////////////////  SPI Configurations  //////////////////////////
//SPI Pinouts
const int32_t SD_SS_PIN = 18;
const int32_t SD_CS_PIN = 15;
const int32_t SD_MO_PIN = 23;
const int32_t SD_MI_PIN = 19;

SPIClass serial_peripheral_interface = SPIClass(VSPI);


//////////////////    I2C Configurations    //////////////////////////
// Define the pins for SDA and SCL for I2C Bus Line
#define SDA_PIN 21
#define SCL_PIN 22

#define IMU_ADDRESS 0x69  // I2C address of the IMU
#define BME_ADDRESS 0x76  //I2C address of the BME

#define LED_PIN 2         // LED connected to GPIO 2
#define BUTTON_PIN 27     // Button connected to GPIO 27
#define DEBOUNCE_TIME 50  // Debounce time in milliseconds

#define FORMAT_LITTLEFS_IF_FAILED true

Adafruit_BME680 bme;  //Uses of the Adrafuit library

//////////////////    UART Configurations    //////////////////////////
HardwareSerial SerialPortA9G(2);  // use UART2
HardwareSerial SerialPortGPS(1);

///////// Definitions of Global Variables of the Program /////////////

String data_logger_id = "device_2";  //UNIC ID OF THE DEVICE

String adress_broker_mqtt = "172.187.90.157";
int port_broker_mqtt = 1883;
unsigned long start_timer = 0;

bool sleep_mode = false;
unsigned long interval_to_sleep_mode = 10 * 60 * 1000;
float movement_detection_threshold = 0.3;

bool bme_mode = true;
unsigned long bme_interval_timer =  30 * 1000;
int interval_bme_recording = 10;  //In Seconds

bool gps_mode = false;
long gps_interval_timer = 60 * 60 * 1000;
int max_time_try_location = 60;  //In Seconds

int accelerometer_scale = 8;  //2,4,8,16g
int sample_rate = 200;        //Hz
bool impact_mode = false;
float impact_threeshold = 5;  //In G - Impact Threeshold

bool vibration_mode = false;
int n_vibration_per_sample = 512;             // This should be a power of 2
float vibration_magnitude_threshold = 0.01;  //G
long vibration_interval_timer = 2 * 60 * 1000;

//Definning functions interval timers
bool mqtt_mode = true;
long mqtt_interval_timer = 15 * 60 * 1000;
bool logs_mode = false;
bool esp_stats_mode = true;
long esp_stats_interval_timer = 10 * 60 * 1000;


//Definition of enums
enum log_message_type {
  INFO,
  WARNING,
  ERROR,
  FATAL_ERROR
};

//Definition of Structs
struct accelerometer_data_struct {
  float accelx;
  float accely;
  float accelz;
};

struct impact_data_struct {
  unsigned long time;
  int millis;
  float accelx;
  float accely;
  float accelz;
};

struct vibration_data_struct {
  unsigned long time;
  float frequency;
  float magnitude;
};

struct bme_data_struct {
  unsigned long epochTime;
  float temperatureMean;
  float humidityMean;
  float pressureMean;
};

struct gps_data_struct {
  unsigned long time;
  String latitude;
  String N_S;
  String longitude;
  String E_W;
  int fix;
  int satellites;
  float HDOP;
  float altitude;
  String units;
  float geoidalSeparation;
  String units2;
};

struct esp_stats_struct {
  unsigned long time;
  float ram_usage;
  float battery_voltage;
  int n_valueswaiting_accelerometer_data_queue;
  int n_valueswaiting_bme_data_queue;
  int n_valueswaiting_gps_data_queue;
};


//Definition of variables of decision
volatile bool interruptFlag_movement = false;
volatile bool order_to_communicate_MQTT = true;    //This variable will controll the comunication
bool connection_configurated_sucessfully = false;  //mqtt to sd
bool request_samples_vibration = false;
volatile bool order_to_turn_off = false;
volatile bool order_to_sleep = false;
volatile bool order_to_read_imu = false;
unsigned long buttonPressTime = 0;


///////////// Definition of Interrupt Functions //////////////////////
// Interrupt Service Routine when button is pressed
void IRAM_ATTR buttonPressed() {
  buttonPressTime = millis();  // Record the time the button was pressed
}

// Interrupt Service Routine when button is released
void IRAM_ATTR buttonReleased() {
  const unsigned long press_time_to_turn_off = 3000;  // Required press time in milliseconds (3 seconds)

  if ((millis() - buttonPressTime) >= press_time_to_turn_off) {  // Check if the button was held long enough
    order_to_turn_off = true;
    xTaskCreate(blinkLEDTask, "Blink LED Task", 1024, NULL, 1, NULL);  // Create the task
  }
}

// Interrupt Service Routine when the imu_reading_timer expires
void IRAM_ATTR imu_reading_timer() {
  order_to_read_imu = true;
}


///////////// Definition of FreeRtos Components //////////////////////
// Definitions of Task Handles
TaskHandle_t esp_manager_task_handle;
TaskHandle_t imu_reading_task_handle;
TaskHandle_t vibration_processing_task_handle;
TaskHandle_t bme_reading_task_handle;
TaskHandle_t gps_manager_task_handle;
TaskHandle_t sd_communication_task_handle;
TaskHandle_t esp_stats_task_handle;
TaskHandle_t mqtt_communication_task_handle;
TaskHandle_t led_blink_task_handle;

//Definitions of Queues
QueueHandle_t impact_data_queue = xQueueCreate(2000, sizeof(impact_data_struct));
QueueHandle_t vibration_data_queue = xQueueCreate(1000, sizeof(vibration_data_struct));
QueueHandle_t vibration_sample_queue = xQueueCreate(n_vibration_per_sample, sizeof(accelerometer_data_struct));

QueueHandle_t bme_data_queue = xQueueCreate(100, sizeof(bme_data_struct));
QueueHandle_t gps_data_queue = xQueueCreate(50, sizeof(gps_data_struct));
QueueHandle_t esp_stats_queue = xQueueCreate(100, sizeof(esp_stats_struct));
QueueHandle_t log_data_queue = xQueueCreate(100, sizeof(char[100]));

//Definition of Semaphores
SemaphoreHandle_t i2cSemaphore = xSemaphoreCreateMutex();
SemaphoreHandle_t A9G_usageSemaphore = xSemaphoreCreateMutex();
SemaphoreHandle_t sd_Semaphore = xSemaphoreCreateMutex();
SemaphoreHandle_t internal_memory_Semaphore = xSemaphoreCreateMutex();

//////////////////    RTC Configurations    //////////////////////////
ESP32Time rtc(0);  // offset in seconds GMT


/////////////////////////////////////////////////////////////////////
/////////////////////  SETUP FUNCTION   /////////////////////////////
void setup() {

  Serial.begin(115200);
  SerialPortA9G.begin(115200, SERIAL_8N1);

  pinMode(SD_CS_PIN, OUTPUT);
  pinMode(25, INPUT);

  //Interrupt Pin - IMU: If detects movement turns HIGH
  pinMode(26, INPUT);

  pinMode(LED_PIN, OUTPUT);     // Set the LED pin as output
  digitalWrite(LED_PIN, HIGH);  //Turn on for configuration

  pinMode(BUTTON_PIN, INPUT_PULLUP);                                           // Set the button pin as input with pull-up resistor
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonPressed, FALLING);  // Attach interrupt for button press
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonReleased, RISING);  // Attach interrupt for button release

  serial_peripheral_interface.begin(SD_SS_PIN, SD_MI_PIN, SD_MO_PIN, SD_CS_PIN);

  Wire.begin(SDA_PIN, SCL_PIN);  // Start the I2C bus

  Serial.println("Starting with a delay of 2000ms");
  delay(2000);

  //Making the remote configuration
  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
    Serial.println("LittleFS Mount Failed");
  }
  testing_sd();
  LittleFS.end();

  //Configuration of variables of the ESP
  config_esp();

  //Configuration of the timer of the imu
  hw_timer_t* timer = NULL;         // Pointer to the hardware timer instance
  timer = timerBegin(0, 80, true);  // Timer 0, prescaler of 80 (80 MHz / 80 = 1 MHz), count up
  timerAttachInterrupt(timer, &imu_reading_timer, true);
  uint32_t timer_interval_ticks = 1000000 / sample_rate;  // Convert sample rate to timer ticks
  timerAlarmWrite(timer, timer_interval_ticks, true);
  timerAlarmEnable(timer);

  //Definition and Start of The ESP Manager Task
  xTaskCreatePinnedToCore(esp_manager_task, "Task MSG", 8000, NULL, 5, &esp_manager_task_handle, 1);
}

/////////////////////////////////////////////////////////////////////
////////////////////  ESP MANAGER TASk  /////////////////////////////

void esp_manager_task(void* parameter) {

  //Configuration of the Sensors
  log(INFO, "esp_manager_task", "Starting to Configurating the Sensors");
  imu_config_accelerometer();
  bme_config();

  //Configuration of the A9G Module
  if (xSemaphoreTake(A9G_usageSemaphore, portMAX_DELAY)) {
    COM_Config("esp_manager_task");
    xSemaphoreGive(A9G_usageSemaphore);
  }

  digitalWrite(LED_PIN, LOW);  //Turn on for configuration

  Serial.println("ESP Manager started to managing tasks");

  unsigned long sleepmode_timer = millis() + interval_to_sleep_mode;

  unsigned long vibration_timer = millis() + 20 * 1000;
  unsigned long bme_timer = millis() + 0.5 * 60000;     //seconds
  unsigned long gps_timer = millis() + 0.5 * 60000;     //seconds
  unsigned long mqtt_timer = millis() + 2 * 60 * 1000;  //seconds
  unsigned long esp_stats_timer = millis() + 1 * 60 * 1000;

  //Normal State of the ESP
  while (1) {

    //Handling the Sleep Mode
    if (interruptFlag_movement == false && sleep_mode == true) {

      // If pin 26 has been low for more than 10 minutes, signal the sleep task
      if (millis() >= sleepmode_timer) {
        Serial.println("Entering in sleeping state mode");
        order_to_sleep = true;

        // Wait for crucial tasks to be suspended
        while (1) {
          bool allTasksSuspended = true;

          // Check if sd_communication_task_handle is not suspended
          if (sd_communication_task_handle == NULL) {
          } else if (eTaskGetState(sd_communication_task_handle) != eSuspended) {
            allTasksSuspended = false;
          }
          // Check if gps_manager_task_handle is not suspended
          if (gps_manager_task_handle == NULL) {
          } else if (eTaskGetState(gps_manager_task_handle) != eSuspended) {
            allTasksSuspended = false;
          }
          // Check if mqtt_communication_task_handle is not suspended
          if (mqtt_communication_task_handle == NULL) {
          } else if (eTaskGetState(mqtt_communication_task_handle) != eSuspended) {
            allTasksSuspended = false;
          }
          // Check if imu_reading_task_handle is not suspended
          if (imu_reading_task_handle == NULL) {
          } else if (eTaskGetState(imu_reading_task_handle) != eSuspended) {
            allTasksSuspended = false;
          }
          // Check if bme_reading_task_handle is not suspended
          if (bme_reading_task_handle == NULL) {
          } else if (eTaskGetState(bme_reading_task_handle) != eSuspended) {
            allTasksSuspended = false;
          }
          // Check if esp_stats_task_handle is not suspended
          if (esp_stats_task_handle == NULL) {
          } else if (eTaskGetState(esp_stats_task_handle) != eSuspended) {
            allTasksSuspended = false;
          }

          if (allTasksSuspended) {
            break;
          }

          Serial.println("Waiting for tasks to be finished");
          vTaskDelay(pdMS_TO_TICKS(2000));
        }

        //Verifies how much time remaining to start
        long remaining_time_vibration = millis() - vibration_timer;
        long remaining_time_bme = millis() - bme_timer;
        long remaining_time_gps = millis() - gps_timer;
        long remaining_time_mqtt = millis() - mqtt_timer;
        long remaining_time_espstats = millis() - esp_stats_timer;


        //Configuration of the Wake-on-motion interrupt
        //vTaskDelay because in the first moments the pin become HIGH several Times
        Serial.println("Sleeping now");
        imu_config_sleepmode();
        vTaskDelay(pdMS_TO_TICKS(5000));

        // Configure the wake-up source
        esp_sleep_enable_ext0_wakeup(GPIO_NUM_26, 1);      // Wake up on HIGH signal
        gpio_set_direction(GPIO_NUM_26, GPIO_MODE_INPUT);  // Configuring the GPIO as input
        gpio_pullup_en(GPIO_NUM_26);                       //enabling internal pull-up

        // Enter deep sleep
        esp_light_sleep_start();

        Serial.println("Waking Up!");
        interruptFlag_movement = true;  //Avoid entering immediately again
        order_to_sleep = false;

        vTaskDelay(pdMS_TO_TICKS(100));
        imu_config_accelerometer();

        vTaskDelay(pdMS_TO_TICKS(2000));

        vibration_timer = millis() + remaining_time_vibration;
        bme_timer = millis() + remaining_time_bme;
        gps_timer = millis() + remaining_time_gps;
        mqtt_timer = millis() + remaining_time_mqtt;
        esp_stats_timer = millis() + remaining_time_espstats;
      }

    } else {
      sleepmode_timer = millis() + interval_to_sleep_mode;  // Reset the start time if the imu_reading_task indicates movement
      Serial.println("Resetting the timer to sleep mode");
      interruptFlag_movement = false;  //Resetting the Flag
    }



    vTaskDelay(pdMS_TO_TICKS(200));

    if (imu_reading_task_handle == NULL) {
      xTaskCreatePinnedToCore(imu_reading_task, "Read IMU Values", 5000, NULL, 10, &imu_reading_task_handle, 0);
    } else if (eTaskGetState(imu_reading_task_handle) == eSuspended) {
      vTaskResume(imu_reading_task_handle);
    } else if (eTaskGetState(imu_reading_task_handle) == eDeleted) {
      xTaskCreatePinnedToCore(imu_reading_task, "Read IMU Values", 5000, NULL, 10, &imu_reading_task_handle, 0);
    }

    vTaskDelay(pdMS_TO_TICKS(200));

    if (millis() >= vibration_timer && vibration_mode == true) {
      if (vibration_processing_task_handle == NULL) {
        xTaskCreatePinnedToCore(vibration_processing_task, "Processing Vibration Values", 30000, NULL, 4, &vibration_processing_task_handle, 1);
      } else if (eTaskGetState(vibration_processing_task_handle) == eSuspended) {
        vTaskResume(vibration_processing_task_handle);
      }
      vibration_timer = millis() + vibration_interval_timer;
    }


    if (millis() >= bme_timer && bme_mode == true) {
      if (bme_reading_task_handle == NULL) {
        xTaskCreatePinnedToCore(bme_reading_task, "Read BME Values", 5000, NULL, 6, &bme_reading_task_handle, 0);
      } else if (eTaskGetState(bme_reading_task_handle) == eSuspended) {
        vTaskResume(bme_reading_task_handle);
      } else if (eTaskGetState(bme_reading_task_handle) == eDeleted) {
        xTaskCreatePinnedToCore(bme_reading_task, "Read IMU Values", 5000, NULL, 10, &bme_reading_task_handle, 0);
      }
      bme_timer = millis() + bme_interval_timer;
    }

    vTaskDelay(pdMS_TO_TICKS(200));

    if (sd_communication_task_handle == NULL) {
      xTaskCreatePinnedToCore(sd_communication_task, "Saving Values on the SD Card", 8000, NULL, 10, &sd_communication_task_handle, 1);
    } else if (eTaskGetState(sd_communication_task_handle) == eSuspended && ((uxQueueMessagesWaiting(bme_data_queue) >= 3) || (uxQueueMessagesWaiting(impact_data_queue) >= 50) || (uxQueueMessagesWaiting(vibration_data_queue) >= 200))) {
      vTaskResume(sd_communication_task_handle);
    }

    vTaskDelay(pdMS_TO_TICKS(200));

    if (millis() >= mqtt_timer && mqtt_mode == true) {
      if (mqtt_communication_task_handle == NULL) {
        xTaskCreatePinnedToCore(mqtt_communication_task, "Send the values from the microSD to MQTT", 12000, NULL, 10, &mqtt_communication_task_handle, 1);
      } else if ((eTaskGetState(mqtt_communication_task_handle) == eSuspended)) {
        vTaskResume(mqtt_communication_task_handle);
      }
      mqtt_timer = millis() + mqtt_interval_timer;
    }

    vTaskDelay(pdMS_TO_TICKS(200));

    if (millis() >= gps_timer && gps_mode == true) {
      if (gps_manager_task_handle == NULL) {
        xTaskCreatePinnedToCore(gps_manager_task, "GPS Manager", 4000, NULL, 4, &gps_manager_task_handle, 1);
      } else if ((eTaskGetState(gps_manager_task_handle) == eSuspended)) {
        vTaskResume(gps_manager_task_handle);
      }
      gps_timer = millis() + gps_interval_timer;
    }

    vTaskDelay(pdMS_TO_TICKS(200));

    if (millis() >= esp_stats_timer && esp_stats_mode == true) {
      if (esp_stats_task_handle == NULL) {
        xTaskCreatePinnedToCore(esp_stats_task, "ESP Stats Recorder", 3000, NULL, 1, &esp_stats_task_handle, 1);
      } else if ((eTaskGetState(esp_stats_task_handle) == eSuspended)) {
        vTaskResume(esp_stats_task_handle);
      } else if ((eTaskGetState(esp_stats_task_handle) == eDeleted)) {
        xTaskCreatePinnedToCore(esp_stats_task, "ESP Stats Recorder", 3000, NULL, 1, &esp_stats_task_handle, 1);
      }
      esp_stats_timer = millis() + esp_stats_interval_timer;
    }

    //Debugging Timers
    Serial.println("-----------------------------------------------------------");
    if (vibration_mode == true) {
      Serial.print("Remaining Time to start vibrations monitoring: ");
      Serial.println((vibration_timer - millis()) / 1000);
    } else {
      Serial.println("vibration_mode deactivated.");
    }

    if (bme_mode == true) {
      Serial.print("Remaining Time to start bme: ");
      Serial.println((bme_timer - millis()) / 1000);
    } else {
      Serial.println("bme_mode deactivated.");
    }
    if (gps_mode == true) {
      Serial.print("Remaining Time to start gps: ");
      Serial.println((gps_timer - millis()) / 1000);
    } else {
      Serial.println("gps_mode deactivated.");
    }
    if (mqtt_mode == true) {
      Serial.print("Remaining Time to start mqtt: ");
      Serial.println((mqtt_timer - millis()) / 1000);
    } else {
      Serial.println("mqtt_mode deactivated.");
    }
    if (esp_stats_mode == true) {
      Serial.print("Remaining Time to start esp_stats: ");
      Serial.println((esp_stats_timer - millis()) / 1000);
    } else {
      Serial.println("esp_stats deactivated.");
    }
    Serial.println("-----------------------------------------------------------");


    //If the pin27 button pressed more than 3 seconds, the microcontroller turns off
    if (order_to_turn_off == true) {
      log(INFO, "esp-manager-task", "Turning off the Device Procedure");

      turn_off_procedure();

      Serial.println("Turning OFF!");
      esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, LOW);
      esp_deep_sleep_start();
    }

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% CORE 0 %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//Function for Core0 Task: Read IMU Values

/*
This task monitors accelerometer data in an infinite loop. 
  It uses a semaphore, the i2cSemaphore to ensure that it is the only task using the I2C bus. 
  The data is stored in a structure and sent to a queue, the accelerometer_data_queue,
which follows a FIFO (First In, First Out) policy architecture, for later processing in task0Core1.
*/

void imu_reading_task(void* parameter) {
  log(INFO, "imu_reading_task", "Task Started");

  float accelX_g = 0;
  float accelY_g = 0;
  float accelZ_g = 0;

  float gyroX = 0;
  float gyroY = 0;
  float gyroZ = 0;

  accelerometer_data_struct vibration_sample;
  impact_data_struct impact_data;

  unsigned long timestamp = 0;

  float previous_accelX_g = 0;
  float previous_accelY_g = 0;
  float previous_accelZ_g = 0;

  while (1) {

    if (order_to_read_imu == true) {
      order_to_read_imu = false;

      previous_accelX_g = accelX_g;
      previous_accelY_g = accelY_g;
      previous_accelZ_g = accelZ_g;

      if (xSemaphoreTake(i2cSemaphore, portMAX_DELAY)) {
        imu_reading(accelX_g, accelY_g, accelZ_g);
        xSemaphoreGive(i2cSemaphore);
      }

      /*Serial.print(accelX_g);
    Serial.print(",");
    Serial.print(accelY_g);
    Serial.print(",");
    Serial.println(accelZ_g);*/


      // Calculate the magnitude of the acceleration vector
      double magnitude = sqrt(accelX_g * accelX_g + accelY_g * accelY_g + accelZ_g * accelZ_g);

      // Check if the magnitude is greater than a certain threshold
      if (magnitude > impact_threeshold && impact_mode == true) {

        timestamp = (float)rtc.getEpoch();
        log(INFO, "imu_reading_task", "Impact Detected!");

        //send the previous data
        impact_data.time = timestamp;
        impact_data.millis = 0;
        impact_data.accelx = previous_accelX_g;
        impact_data.accely = previous_accelY_g;
        impact_data.accelz = previous_accelZ_g;

        if (xQueueSend(impact_data_queue, &impact_data, 100) != pdPASS) {
          Serial.println("Failed to send data to the accelerometer_data_processed_queue!");
        }

        //send the actual dectected above threeshold data
        impact_data.time = timestamp;
        impact_data.millis = (1000 / sample_rate);
        impact_data.accelx = accelX_g;
        impact_data.accely = accelY_g;
        impact_data.accelz = accelZ_g;

        if (xQueueSend(impact_data_queue, &impact_data, 100) != pdPASS) {
          Serial.println("Failed to send data to the accelerometer_data_processed_queue!");
        }

        int i = 0;
        int finish_pos = 1;

        while (i < 15) {

          if (order_to_read_imu == true) {
            order_to_read_imu = false;
            finish_pos += 1;

            // Get new acceleration values
            if (xSemaphoreTake(i2cSemaphore, portMAX_DELAY)) {
              imu_reading(accelX_g, accelY_g, accelZ_g);
              xSemaphoreGive(i2cSemaphore);
            }

            //send the actual data
            impact_data.time = timestamp;
            impact_data.millis = (1000 / sample_rate) * finish_pos;
            impact_data.accelx = accelX_g;
            impact_data.accely = accelY_g;
            impact_data.accelz = accelZ_g;

            if (xQueueSend(impact_data_queue, &impact_data, 100) != pdPASS) {
              Serial.println("Failed to send data to the accelerometer_data_processed_queue!");
            }

            magnitude = sqrt(accelX_g * accelX_g + accelY_g * accelY_g + accelZ_g * accelZ_g);

            if (magnitude < 1.1) {
              i += 1;
            } else if (finish_pos == 300) {
              Serial.println("Excedeu 300 finish_pos!");
              break;
            } else {
              i = 0;
              //vTaskDelay(pdMS_TO_TICKS(1000 / sample_rate - 1));
            }
          }
          vTaskDelay(1);
        }

        int impact_duration = (finish_pos - 1) * (1000 / sample_rate);
        log(INFO, "imu_reading_task", "Impact above " + String(impact_threeshold) + "G");
        log(INFO, "imu_reading_task", "Number of Records: " + String(finish_pos));
        log(INFO, "imu_reading_task", "Impact Duration in ms: " + String(impact_duration) + " ms");
        xQueueReset(vibration_sample_queue);
      }

      //Verifies if it is on movement or not
      double previous_magnitude = sqrt(previous_accelX_g * previous_accelX_g + previous_accelY_g * previous_accelY_g + previous_accelZ_g * previous_accelZ_g);
      // Determine if the current magnitude is significantly different from the previous magnitude
      if (abs(magnitude - previous_magnitude) >= movement_detection_threshold) {
        interruptFlag_movement = true;
      }

      // Check if vibration sampling is requested
      if (request_samples_vibration) {

        vibration_sample.accelx = accelX_g;
        vibration_sample.accely = accelY_g;
        vibration_sample.accelz = accelZ_g - 1;

        // Attempt to send the vibration sample to the queue
        if (xQueueSend(vibration_sample_queue, &vibration_sample, 10) != pdPASS) {
          // If the queue is full, disable vibration sampling
          Serial.println("Queue full! Disabling vibration sampling.");
          request_samples_vibration = false;  // Disable vibration sampling
        }
      }
    }
    //vTaskDelay(pdMS_TO_TICKS(1000 / sample_rate - 1));

    if (order_to_sleep == true) {
      vTaskSuspend(imu_reading_task_handle);
    }
  }
  log(INFO, "imu_reading_task", "Task Ended");
  vTaskSuspend(imu_reading_task_handle);
}



/////////////////////////////////////////////////////////////////////////////////////77

void vibration_processing_task(void* parameter) {

  accelerometer_data_struct vibration_sample;
  vibration_data_struct vibration_data;

  double vReal_accelz[n_vibration_per_sample];
  double vImag[n_vibration_per_sample] = { 0 };  // Initialize all to zero

  // Create FFT object
  ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal_accelz, vImag, n_vibration_per_sample, sample_rate);

  while (1) {
    log(INFO, "vibration-processing-task", "Task Started");

    //The Task will catch the semaphore for not over-running the CPU while is communicating with the A9G
    if (xSemaphoreTake(A9G_usageSemaphore, portMAX_DELAY)) {

      request_samples_vibration = true;

      while (request_samples_vibration) {
        Serial.println("Waiting for Vibration Data");
        vTaskDelay(pdMS_TO_TICKS(500));
      }

      int samples_received = 0;
      while (samples_received < n_vibration_per_sample) {
        if (xQueueReceive(vibration_sample_queue, &vibration_sample, portMAX_DELAY) == pdPASS) {
          vReal_accelz[samples_received] = vibration_sample.accelz;
          vImag[samples_received] = 0;
          samples_received++;
        }
      }

      // FFT Processing
      FFT.windowing(vReal_accelz, n_vibration_per_sample, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
      FFT.compute(vReal_accelz, vImag, n_vibration_per_sample, FFT_FORWARD);
      FFT.complexToMagnitude(vReal_accelz, vImag, n_vibration_per_sample);

      // Normalize the magnitude of the FFT
      double normalization_factor = n_vibration_per_sample / 2.0;
      for (int i = 0; i < n_vibration_per_sample; i++) {
        vReal_accelz[i] /= normalization_factor;
      }

      vibration_data.time = rtc.getEpoch();

      // Process and send vibration data if above threshold
      for (int i = 1; i <= sample_rate / 2; i++) {
        int index = i * n_vibration_per_sample / sample_rate;
        if (index < n_vibration_per_sample / 2) {
          vibration_data.frequency = i;
          vibration_data.magnitude = vReal_accelz[index];

          if (vibration_data.magnitude > vibration_magnitude_threshold) {
            Serial.print("Frequency: ");
            Serial.print(i);
            Serial.print(" Hz, Magnitude: ");
            Serial.println(vibration_data.magnitude, 5);

            if (xQueueSend(vibration_data_queue, &vibration_data, 10) != pdPASS) {
              log(ERROR, "vibration-processing-task", "Queue Send Failed");
            }
          }
        }
      }
      xSemaphoreGive(A9G_usageSemaphore);
    }

    log(INFO, "vibration-processing-task", "Task Ended");
    vTaskSuspend(vibration_processing_task_handle);
  }
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//Function for Core0 Task 1: Read BME Values

/*
This task monitors bme data in an infinite loop. 
  It uses a semaphore, the i2cSemaphore to ensure that it is the only task using the I2C bus. 
  The data is stored in a structure and sent to a queue, the bme_data_queue,
which follows a FIFO (First In, First Out) policy architecture, for later processing in task0Core1.
*/
void bme_reading_task(void* parameter) {

  while (1) {
    log(INFO, "bme_reading_task", "Task Started");

    float temperature = 0;
    float humidity = 0;
    float pressure = 0;
    float temp_mean = 0;
    float hum_mean = 0;
    float pressure_mean = 0;
    int contador_values = 0;

    bme_data_struct sentData_bme;


    //Wake up the sensor and configure again sampling for temperature and pressure

    //Register Ctrl_meas (0x74)
    //Pag26: mode<1:0> 00 SleepMode 01 ForcedMode
    //pag27: osrs_p<4:2> 100 oversampling x8 - pressure
    //Pag27: osrs_t<8:5> 100 oversampling x8 - temperature

    //Register Ctrl_hum (0x72)
    //Pag27: osrs_t<2:0> 100 oversampling x8 - humidity

    //if (xSemaphoreTake(i2cSemaphore, portMAX_DELAY)) {
      write_I2C(BME_ADDRESS, 0x74, 0b10100100);
      write_I2C(BME_ADDRESS, 0x72, 0b00000100);
      //xSemaphoreGive(i2cSemaphore);
    //}

    //Waiting to wake up
    vTaskDelay(pdMS_TO_TICKS(100));

    unsigned long initial_timer = millis();

    while ((millis() - initial_timer) <= (interval_bme_recording * 1000)) {

      //if (xSemaphoreTake(i2cSemaphore, portMAX_DELAY)) {
        BME_Reading(temperature, humidity, pressure);
        Serial.println(humidity);
        //xSemaphoreGive(i2cSemaphore);
      //}

      temp_mean += temperature;
      hum_mean += humidity;
      pressure_mean += pressure;
      contador_values += 1;
    }

    sentData_bme.epochTime = rtc.getEpoch();
    sentData_bme.temperatureMean = temp_mean / contador_values;
    sentData_bme.humidityMean = hum_mean / contador_values;
    sentData_bme.pressureMean = pressure_mean / contador_values;

    if (xQueueSend(bme_data_queue, &sentData_bme, portMAX_DELAY) != pdPASS) {
      log(ERROR, "bme_reading_task", "Failed to send data to the queue!");
    }

    //The sensor will sleep
    //if (xSemaphoreTake(i2cSemaphore, portMAX_DELAY)) {
    write_I2C(BME_ADDRESS, 0x74, 0b0);
      //xSemaphoreGive(i2cSemaphore);
    //}

    log(INFO, "bme_reading_task", "Task Ended");
    vTaskSuspend(bme_reading_task_handle);
  }
  vTaskDelete(NULL);  //Never should get here
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//Function for Core1 Task 2: GPS Manager

void gps_manager_task(void* parameter) {

  while (1) {
    log(INFO, "gps_manager_task", "Task Started");

    //Local Variables
    gps_data_struct data;
    data.satellites == 0;
    String message_read = "";
    bool gps_status = false;

    //Verifies if the GPS is already on, if not, it will turn on.
    while (gps_status == false) {
      //Verifies if the gps is already turn on or off
      if (xSemaphoreTake(A9G_usageSemaphore, portMAX_DELAY)) {
        gps_status = check_gps_status();
        xSemaphoreGive(A9G_usageSemaphore);
      }
      vTaskDelay(pdMS_TO_TICKS(2000));
    }

    //Configuration of the GPS and SerialPort
    SerialPortGPS.begin(9600, SERIAL_8N1, 14, 13);

    long startTime = millis();
    log(INFO, "gps_manager_task", "Starting to obtain GPS Signal!");

    while (1) {
      // Check for available data on the GPS serial port
      if (SerialPortGPS.available() > 0) {
        // Read and process the GPS data
        String message_read = SerialPortGPS.readStringUntil('\r');
        SerialPortGPS.flush();
        gps_data_struct data = Processing_GPS(message_read);

        // Check if the GPS fix is valid
        if (data.fix == 1 && data.satellites >= 4 && data.latitude != 0 && data.longitude != 0 && data.HDOP >= 2) {
          log(INFO, "gps_manager_task", "Location Successed!");
          // Attempt to send the GPS data to the queue
          if (xQueueSend(gps_data_queue, &data, portMAX_DELAY) != pdPASS) {
            log(INFO, "gps_manager_task", "Failed to send GPS data to the queue!");
          }

          // Safely turn off the GPS
          if (xSemaphoreTake(A9G_usageSemaphore, portMAX_DELAY)) {
            COM_GPS(false);
            xSemaphoreGive(A9G_usageSemaphore);
          }
          break;  // Exit the loop if the GPS fix is valid
        }
      }

      // Check if the timeout has been reached
      if (millis() - startTime >= (max_time_try_location * 1000)) {
        log(INFO, "gps_manager_task", "Location Timeout");
        break;  // Exit the loop if the timeout is reached
      }
    }

    SerialPortGPS.end();
    log(INFO, "gps_manager_task", "Task Ended");
    vTaskSuspend(gps_manager_task_handle);
  }
  vTaskDelete(NULL);  //Should never get here
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//Task Function sd_communication_task: SD communication Task

void sd_communication_task(void* parameters) {

  //Using of a loop for suspending the task
  while (1) {

    log(INFO, "sd-communication-task", "Task Started");
    bool ver1 = false;
    bool ver2 = false;

    while (1) {
      bool decision = checkMicroSDCard();
      if (decision == true) {
        break;
      } else {
        log(INFO, "sd-communication-task", "No SD-Card available on the slot!");
        vTaskDelay(pdMS_TO_TICKS(10000));
        vTaskSuspend(NULL);  // Suspend the current task
      }
      vTaskDelay(pdMS_TO_TICKS(20));
    }

    if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
      Serial.println("LittleFS Mount Failed");
    }


    if (xSemaphoreTake(sd_Semaphore, portMAX_DELAY) && xSemaphoreTake(internal_memory_Semaphore, portMAX_DELAY)) {

      //Waits 3 seconds after getting the sempahores, because the sd is very sensible
      vTaskDelay(pdMS_TO_TICKS(3000));

      char log_data[100];  // Allocate memory for 99 characters plus the null terminator

      while (uxQueueMessagesWaiting(log_data_queue) >= 2) {
        if (xQueueReceive(log_data_queue, log_data, 0) == pdTRUE) {
          strcat(log_data, "\n");  //Add a paragraph
          if (!SD.begin(SD_CS_PIN, serial_peripheral_interface, 1000000)) {
            log(ERROR, "sd_communication_task", "SD card mount failed saving log data.");
            break;
          } else {
            ver1 = append_file_testing(SD, "/log_data.txt", log_data);
            ver2 = append_file_testing(LittleFS, "/log_data_temp.txt", log_data);
            if (ver1 || ver2) {
            } else {
              log(WARNING, "sd_communication_task", "log data line failed to write.");
            }
            SD.end();
          }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
      }

      // Saving bme data
      bme_data_struct bme_data;
      String data = "";

      while (uxQueueMessagesWaiting(bme_data_queue) >= 2) {
        if (xQueueReceive(bme_data_queue, &bme_data, 0) == pdTRUE) {

          // Format the data and append to the buffer
          data = String(bme_data.epochTime) + ",";
          data += String(bme_data.temperatureMean) + ",";
          data += String(bme_data.humidityMean) + ",";
          data += String(bme_data.pressureMean) + "\n";

          if (!SD.begin(SD_CS_PIN, serial_peripheral_interface, 1000000)) {
            log(ERROR, "sd_communication_task", "SD card mount failed saving bme data.");
            break;
          } else {
            ver1 = append_file(SD, "/bme_data.txt", data);
            ver2 = append_file(LittleFS, "/bme_data_temp.txt", data);

            if (ver1 || ver2) {
              data = "";
            } else {
              log(WARNING, "sd_communication_task", "Bme data line failed to write.");
            }

            SD.end();
          }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
      }

      //Saving impacts data
      impact_data_struct impact_data;
      data = "";

      while (uxQueueMessagesWaiting(impact_data_queue) >= 1) {
        if (xQueueReceive(impact_data_queue, &impact_data, 0) == pdTRUE) {
          data += String(impact_data.time) + ",";
          data += String(impact_data.millis) + ",";
          data += String(impact_data.accelx, 4) + ",";
          data += String(impact_data.accely, 4) + ",";
          data += String(impact_data.accelz, 4) + "\n";
        }

        if (!SD.begin(SD_CS_PIN, serial_peripheral_interface, 1000000)) {
          log(ERROR, "sd_communication_task", "SD card mount failed saving imu data.");
          break;
        } else {
          ver1 = append_file(SD, "/impact_data.txt", data);
          ver2 = append_file(LittleFS, "/impact_data_temp.txt", data);

          if (ver1 || ver2) {
            data = "";
          } else {
            log(WARNING, "sd_communication_task", "Imu data line failed to write.");
          }

          SD.end();
        }

        vTaskDelay(pdMS_TO_TICKS(20));
      }


      // Saving vibration data
      vibration_data_struct vibration_data;
      data = "";

      while (uxQueueMessagesWaiting(vibration_data_queue) >= 1) {
        if (xQueueReceive(vibration_data_queue, &vibration_data, 0) == pdTRUE) {
          data += String(vibration_data.time) + ",";
          data += String(vibration_data.frequency) + ",";
          data += String(vibration_data.magnitude, 3) + "\n";
        }

        if (!SD.begin(SD_CS_PIN, serial_peripheral_interface, 1000000)) {
          log(ERROR, "sd_communication_task", "SD card mount failed saving vibration data.");
          break;
        } else {
          ver1 = append_file(SD, "/vibration_data.txt", data);
          ver2 = append_file(LittleFS, "/vibration_data_temp.txt", data);

          if (ver1 || ver2) {
            data = "";
          } else {
            log(WARNING, "sd_communication_task", "Imu data line failed to write.");
          }

          SD.end();
        }

        vTaskDelay(pdMS_TO_TICKS(20));
      }

      // Saving gps data
      gps_data_struct gps_data;
      data = "";

      while (uxQueueMessagesWaiting(gps_data_queue) >= 1) {
        if (xQueueReceive(gps_data_queue, &gps_data, 0) == pdTRUE) {

          // Format the data and append to the buffer
          data = String(gps_data.time) + ",";
          data += String(convertLatitude(gps_data.latitude, gps_data.N_S), 8) + ",";
          data += String(convertLongitude(gps_data.longitude, gps_data.E_W), 8) + ",";
          data += String(gps_data.fix) + ",";
          data += String(gps_data.satellites) + ",";
          data += String(gps_data.HDOP) + ",";
          data += String(gps_data.altitude) + ",";
          data += String(gps_data.geoidalSeparation) + "\n";


          if (!SD.begin(SD_CS_PIN, serial_peripheral_interface, 1000000)) {
            log(ERROR, "sd_communication_task", "SD card mount failed saving gps data.");
            break;
          } else {
            ver1 = append_file(SD, "/gps_data.txt", data);
            ver2 = append_file(LittleFS, "/gps_data_temp.txt", data);

            if (ver1 || ver2) {
              data = "";
            } else {
              log(WARNING, "sd_communication_task", "GPS data line failed to write.");
            }

            SD.end();
          }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
      }

      // Saving esp statistics data
      esp_stats_struct esp_stats;
      data = "";

      while (uxQueueMessagesWaiting(esp_stats_queue) >= 2) {
        if (xQueueReceive(esp_stats_queue, &esp_stats, 0) == pdTRUE) {

          // Format the data and append to the buffer
          data = String(esp_stats.time) + ",";
          data += String(esp_stats.ram_usage) + ",";
          data += String(esp_stats.battery_voltage) + ",";
          data += String(esp_stats.n_valueswaiting_accelerometer_data_queue) + ",";
          data += String(esp_stats.n_valueswaiting_bme_data_queue) + ",";
          data += String(esp_stats.n_valueswaiting_gps_data_queue) + "\n";

          if (!SD.begin(SD_CS_PIN, serial_peripheral_interface, 1000000)) {
            log(ERROR, "sd_communication_task", "SD card mount failed saving esp_stats data.");
            break;
          } else {
            ver1 = append_file(SD, "/esp_stats.txt", data);
            ver2 = append_file(LittleFS, "/esp_stats_temp.txt", data);

            if (ver1 || ver2) {
              data = "";
            } else {
              log(WARNING, "sd_communication_task", "esp_stats data line failed to write.");
            }
            SD.end();
          }
        }
        vTaskDelay(pdMS_TO_TICKS(20));
      }

      data = "";
      xSemaphoreGive(sd_Semaphore);
      xSemaphoreGive(internal_memory_Semaphore);
    }
    log(INFO, "sd_communication_task", "Task Ended");
    vTaskSuspend(sd_communication_task_handle);
  }
  vTaskDelete(NULL);  //Should never get here
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//Task Function mqtt_communication_task: Sending data from the MicroSD Card to MQTT

void mqtt_communication_task(void* parameters) {

  //The program will never exit this loop
  while (1) {

    log(INFO, "mqtt_communication_task", "Task Started");

    if (xSemaphoreTake(A9G_usageSemaphore, portMAX_DELAY) && xSemaphoreTake(internal_memory_Semaphore, portMAX_DELAY)) {


      if (connection_configurated_sucessfully == false) {
        for (int i = 0; i < 3; i++) {

          //Waking up the A9G
          COM_Config("mqtt_communication_task");
          vTaskDelay(pdMS_TO_TICKS(1000));

          //Configurating the Network
          if (COM_Config_Network()) {
            connection_configurated_sucessfully = true;
            break;
          } else if (i == 3) {
            log(FATAL_ERROR, "mqtt_communication_task", "A9G cannot configure to the server after 3 times trying");
            connection_configurated_sucessfully == false;
          }
          vTaskDelay(pdMS_TO_TICKS(2000));
        }
      }

      vTaskDelay(pdMS_TO_TICKS(1000));

      if (connection_configurated_sucessfully == true) {

        //Reading log_data_temp.txt and publish in the "log" topic in mqtt /////////////////////////////////
        if (check_file(LittleFS, "/log_data_temp.txt") == true) {
          if (sd_to_mqtt(LittleFS, "/log_data_temp.txt", "log", 5) == true) {
            log(INFO, "mqtt_communication_task", "Data sent sucessfully (log_data_temp.txt)");
            delete_file(LittleFS, "/log_data_temp.txt");
          }
          vTaskDelay(pdMS_TO_TICKS(1000));
        }

        //Reading bme_data_temp.txt and publish in the "bme" topic in mqtt /////////////////////////////////
        if (check_file(LittleFS, "/bme_data_temp.txt") == true) {
          if (sd_to_mqtt(LittleFS, "/bme_data_temp.txt", "bme", 10) == true) {
            log(INFO, "mqtt_communication_task", "Data sent sucessfully (bme_data_temp.txt)");
            delete_file(LittleFS, "/bme_data_temp.txt");
          }
          vTaskDelay(pdMS_TO_TICKS(1000));
        }

        //Reading impact_data_temp.txt and publish in the "imu" topic in mqtt //////////////////////////////
        if (check_file(LittleFS, "/impact_data_temp.txt") == true) {
          if (sd_to_mqtt(LittleFS, "/impact_data_temp.txt", "impacts", 20) == true) {
            log(INFO, "mqtt_communication_task", "Data sent sucessfully (impact_data_temp.txt)");
            delete_file(LittleFS, "/impact_data_temp.txt");
          }
          vTaskDelay(pdMS_TO_TICKS(1000));
        }

        //Reading vibration_data_temp.txt and publish in the "vibration" topic in mqtt /////////////////////////////////
        if (check_file(LittleFS, "/vibration_data_temp.txt") == true) {
          if (sd_to_mqtt(LittleFS, "/vibration_data_temp.txt", "vibration", 20) == true) {
            log(INFO, "mqtt_communication_task", "Data sent sucessfully (vibration_data_temp.txt)");
            delete_file(LittleFS, "/vibration_data_temp.txt");
          }
          vTaskDelay(pdMS_TO_TICKS(1000));
        }

        //Reading gps_data_temp.txt and publish in the "gps" topic in mqtt /////////////////////////////////
        if (check_file(LittleFS, "/gps_data_temp.txt") == true) {
          if (sd_to_mqtt(LittleFS, "/gps_data_temp.txt", "gps", 10) == true) {
            log(INFO, "mqtt_communication_task", "Data sent sucessfully (gps_data_temp.txt)");
            delete_file(LittleFS, "/gps_data_temp.txt");
          }
          vTaskDelay(pdMS_TO_TICKS(1000));
        }

        //Readingesp_stats_temp.txt and publish in the "esp_stats" topic in mqtt /////////////////////////////////
        if (check_file(LittleFS, "/esp_stats_temp.txt") == true) {
          if (sd_to_mqtt(LittleFS, "/esp_stats_temp.txt", "esp_stats", 10) == true) {
            log(INFO, "mqtt_communication_task", "Data sent sucessfully (esp_stats_temp.txt)");
            delete_file(LittleFS, "/esp_stats_temp.txt");
          }
          vTaskDelay(pdMS_TO_TICKS(1000));
        }

        //Disconnects from the MQTT Server
        SerialPortA9G.println("AT+MQTTDISCONN");
        if (!check_response_A9G(10000, "OK")) {
          log(ERROR, "mqtt_communication_task", "Failed to disconnect from MQTT Server");
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
        log(INFO, "mqtt_communication_task", "Ending Transmitting Data");
      }
    } else {
      log(FATAL_ERROR, "mqtt_communication_task", "Connection with A9G failed");
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
    xSemaphoreGive(internal_memory_Semaphore);
    xSemaphoreGive(A9G_usageSemaphore);

    connection_configurated_sucessfully = false;
    log(INFO, "mqtt_communication_task", "Task Ended");
    vTaskSuspend(mqtt_communication_task_handle);
  }
  vTaskDelete(NULL);  //Should never get here
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//Function for Core1 Task 3: Debugging and RAM Usage Monitoring

void esp_stats_task(void* parameter) {

  while (1) {

    esp_stats_struct data;

    log(INFO, "esp_stats_task", "Task Started");

    data.time = rtc.getEpoch();

    ///////////////////// RAM USAGE    ////////////////////////////////////
    // Get free heap size and total heap size
    long freeHeap = ESP.getFreeHeap();
    long totalHeap = ESP.getHeapSize();
    long CPU_freq = ESP.getCpuFreqMHz();

    // Calculate RAM usage percentage
    data.ram_usage = static_cast<float>(freeHeap) / totalHeap * 100.0;

    ///////////////////   Batery Status ///////////////////////////////////
    uint32_t value = 0;
    int rounds = 11;
    esp_adc_cal_characteristics_t adc_chars;
    String battery_voltage = "";

    //battery voltage divided by 2 can be measured at GPIO34, which equals ADC1_CHANNEL6
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);

    //to avoid noise, sample the pin several times and average the result
    for (int i = 1; i <= rounds; i++) {
      value += adc1_get_raw(ADC1_CHANNEL_6);
    }
    value /= (uint32_t)rounds;

    //due to the voltage divider (1M+1M) values must be multiplied by 2
    //and convert mV to V
    data.battery_voltage = esp_adc_cal_raw_to_voltage(value, &adc_chars) * 2.0 / 1000.0;
    Serial.print("Battery Voltage:");
    Serial.println(data.battery_voltage);

    /////////////////////////// Queues Status /////////////////////////////

    data.n_valueswaiting_accelerometer_data_queue = uxQueueMessagesWaiting(impact_data_queue);
    data.n_valueswaiting_bme_data_queue = uxQueueMessagesWaiting(bme_data_queue);
    data.n_valueswaiting_gps_data_queue = uxQueueMessagesWaiting(gps_data_queue);


    ////////////////// MONITORING DATA SAVING /////////////////////////////

    if (xQueueSend(esp_stats_queue, &data, portMAX_DELAY) != pdPASS) {
      log(ERROR, "esp_stats_task", "Failed to send espstats data to the queue!");
    }

    // Delay for 10 second
    vTaskDelay(pdMS_TO_TICKS(10000));

    log(INFO, "esp_stats_task", "Task Ended");
    vTaskSuspend(esp_stats_task_handle);
  }
  vTaskDelete(NULL);  //Should never get here
}

// Secondary task to blink the led
void blinkLEDTask(void* parameter) {
  while (1) {
    digitalWrite(LED_PIN, HIGH);     // Turn LED on
    vTaskDelay(pdMS_TO_TICKS(500));  // Delay for 500 ms
    digitalWrite(LED_PIN, LOW);      // Turn LED off
    vTaskDelay(pdMS_TO_TICKS(500));  // Delay for 500 ms
  }
  vTaskDelete(NULL);
}

void loop() {}