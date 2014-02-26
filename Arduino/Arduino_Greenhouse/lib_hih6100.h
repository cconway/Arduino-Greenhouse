// LICENSES: [c3912b, a1cdbd]
// -----------------------------------
// The contents of this file contains the aggregate of contributions
//   covered under one or more licences. The full text of those licenses
//   can be found in the "LICENSES" file at the top level of this project
//   identified by the MD5 fingerprints listed above.

// HIH_6130_1  - Arduino
// 
// Arduino                HIH-6130
// SCL (Analog 5) ------- SCL (term 3)
// SDA (Analog 4) ------- SDA (term 4)
//
// Note 2.2K pullups to 5 VDC on both SDA and SCL
//
// Pin4 ----------------- Vdd (term 8) 
//
// Illustrates how to measure relative humidity and temperature.

#ifndef HIH6100_Sensor_h
#define HIH6100_Sensor_h
    
// TYPES
// -------------------------------------------------
typedef enum HoneywellSensor {
  
  HoneywellSensorNone,
  HoneywellSensorExterior,
  HoneywellSensorInterior
  
};

typedef enum HIH6100State {
  
  HIH6100StateUndefined,
  HIH6100StateNormal,
  HIH6100StateStale,
  HIH6100StateCommandMode,
  HIH6100StateDiagnostic
};



// Class Definition
// -------------------------------------------------
class HIH6100_Sensor {
  
  public:
    HIH6100_Sensor(void);
    boolean performMeasurement(void);
    void printStatus(void);
    
    HIH6100State state;  // Status of the sensor device
    float temperature;  // °C
    float humidity;  // % relative humidity
    
  private:
    byte _requestData(unsigned int *p_H_dat, unsigned int *p_T_dat);
    void _print_float(float f, int num_digits);
};

#endif
