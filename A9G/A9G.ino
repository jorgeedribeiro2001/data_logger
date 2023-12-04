#include <HardwareSerial.h>

HardwareSerial SerialPort(2);  // use UART2

char number = ' ';
int LED = 2;

struct GpsData {
  String time;
  String latitude;
  String N_S;
  String longitude;
  String E_W;
  String fix;
  String satellites;
  String altitude;
  String units;
  String geoidalSeparation;
  String units2;
};

void setup() {
  SerialPort.begin(115200, SERIAL_8N1, 16, 17);
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
}
void loop() {

  //Read from A9G and display in the Serial Monitor
  if (SerialPort.available()) {
    String message_read = "";
    while (SerialPort.available()) {
      char char_read = SerialPort.read();  // Read one character from the serial port
      message_read += char_read;           //Construction of the message
    }

    //Verifies if the message_read is a GPS Status NMEA Setence and Process It for better view in the terminal
    if (message_read.startsWith("+GPSRD:")) {
      //Serial.println(message_read);
      //Serial.println(" ");
      GpsData gpsdata = processNMEA(message_read);
      Serial.println("Satellites Conections: " + gpsdata.satellites);
      Serial.println("Latitude: " + String(convertLatitude(gpsdata.latitude, gpsdata.N_S), 10));
      Serial.println("Longitude: " + String(convertLongitude(gpsdata.longitude, gpsdata.E_W), 10));
      Serial.println("Altitude: " + gpsdata.altitude + " " + gpsdata.units);
      Serial.println(" ");
    }

    //Prints the message in the Serial Monitor
    else { Serial.println(message_read); }
  }


  //Read from Serial Port and send to A9G
  if (Serial.available()) {
    String message_to_send = "";
    while (Serial.available()) {
      char char_to_send = Serial.read();  // Read one character from the serial port
      if (char_to_send == '\n') {
        //Serial.print(message_to_send);
        SerialPort.println(message_to_send);
      } else {
        message_to_send += char_to_send;  // Corrected line
      }
    }
  }

  delay(1000);
}




