// Function to process the NMEA sentence and extract GPS data
GpsData processNMEA(String gpsString) {
  GpsData data;
  int startIndex = gpsString.indexOf(':') + 1;
  gpsString = gpsString.substring(startIndex);
  
  data.time = getValue(gpsString, ',', 1);
  data.latitude = getValue(gpsString, ',', 2);
  data.N_S = getValue(gpsString, ',', 3);
  data.longitude = getValue(gpsString, ',', 4);
  data.E_W = getValue(gpsString, ',', 5);
  data.fix = getValue(gpsString, ',', 6);
  data.satellites = getValue(gpsString, ',', 7);
  data.altitude = getValue(gpsString, ',', 9);
  data.units = getValue(gpsString, ',', 10);
  data.geoidalSeparation = getValue(gpsString, ',', 11);
  data.units2 = getValue(gpsString, ',', 12);
  
  return data;
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  // Loop through each character in the string
  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  // Return the value at the specified index
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}


//////////////////////////////////////////////////////////////////////////////
///////////////////CONVERT FUNCTION GPS LATITUDE AND LONGITUDE////////////////
//4036.6045,N,00844.9832,W -> Latitude: 40.6100769043 Longitude: -8.7497196198

float convertLatitude(String lat, String N_S) {
  // Parse the degrees and minutes from the input string
  int degrees = lat.substring(0, 2).toInt();
  float minutes = lat.substring(2).toFloat();

  // Convert the minutes to the decimal part of the degrees
  float decimalDegrees = degrees + (minutes / 60.0);

  // Adjust for the hemisphere (North or South)
  if (strcmp(N_S.c_str(), "S") == 0) {
    decimalDegrees = -decimalDegrees;
  }
  return decimalDegrees;
}

float convertLongitude(String lon, String E_W) {
  // Parse the degrees and minutes from the input string
  int degrees = lon.substring(0, 3).toInt();
  float minutes = lon.substring(3).toFloat();

  // Convert the minutes to the decimal part of the degrees
  float decimalDegrees = degrees + (minutes / 60.0);

  // Adjust for the hemisphere (East or West)
  if (E_W.equals("W")) {
    decimalDegrees = -decimalDegrees;
  }

  return decimalDegrees;
}




