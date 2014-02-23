#import "Arduino.h"
#import "lib_hih6100.h"
#include <Wire.h>


HIH6100_Sensor::HIH6100_Sensor(void) {
  
  state = HIH6100StateUndefined;
}

boolean HIH6100_Sensor::performMeasurement(void) {
  
  byte _status;
  unsigned int H_dat, T_dat;
  float RH, T_C;
   
  _status = this->_requestData(&H_dat, &T_dat);

  // Decode the device status
  switch (_status) {
    
    case 0: state = HIH6100StateNormal; break;
    case 1: state = HIH6100StateStale; break;  // Hasn't performed a measurement since we last read
    case 2: state = HIH6100StateCommandMode; break;  // Entered into command mode for changing EEPROM settings
    default: state = HIH6100StateDiagnostic; break;  // An error occurred
  }       
  
  // Convert raw values to appropriate units
  humidity = (float) H_dat * 6.10e-3;
  temperature = (float) T_dat * 1.007e-2 - 40.0;

  return (state == HIH6100StateNormal);
}

byte HIH6100_Sensor::_requestData(unsigned int *p_H_dat, unsigned int *p_T_dat) {
  
  byte address, Hum_H, Hum_L, Temp_H, Temp_L, _status;
  unsigned int H_dat, T_dat;
  address = 0x27;
  Wire.beginTransmission(address); 
  Wire.endTransmission();
  delay(100);
  
  Wire.requestFrom((int)address, (int) 4);
  
  if (Wire.available() == 4) {
    
    Hum_H = Wire.read();
    Hum_L = Wire.read();
    Temp_H = Wire.read();
    Temp_L = Wire.read();
    Wire.endTransmission();
  
  } else Serial.println("Didn't get bytes from HIH");
  
  _status = (Hum_H >> 6) & 0x03;
  Hum_H = Hum_H & 0x3f;
  H_dat = (((unsigned int)Hum_H) << 8) | Hum_L;
  T_dat = (((unsigned int)Temp_H) << 8) | Temp_L;
  T_dat = T_dat / 4;
  *p_H_dat = H_dat;
  *p_T_dat = T_dat;
  return(_status);
}

void HIH6100_Sensor::printStatus(void) {
  
  Serial.print("HIH6100: ");
  Serial.print(humidity);
  Serial.print("  % humidity, ");
  Serial.print(temperature);
  Serial.println(" deg C");
}
