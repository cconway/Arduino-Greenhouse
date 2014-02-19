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
//
// copyright, Peter H Anderson, Baltimore, MD, Nov, '11
// You may use it, but please give credit.  

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

//byte fetch_humidity_temperature(unsigned int *p_Humidity, unsigned int *p_Temperature);
//void print_float(float f, int num_digits);

//#define TRUE 1
//#define FALSE 0

//void setup(void)
//{
//   Serial.begin(9600);
//   Wire.begin();
//   pinMode(4, OUTPUT);
//   digitalWrite(4, HIGH); // this turns on the HIH3610
//   delay(5000);
//   Serial.println(">>>>>>>>>>>>>>>>>>>>>>>>");  // just to be sure things are working
//}
    
//void loop(void)
//{
//   byte _status;
//   unsigned int H_dat, T_dat;
//   float RH, T_C;
//   
//   while(1)
//   {
//      _status = fetch_humidity_temperature(&H_dat, &T_dat);
//      
//      switch(_status)
//      {
//          case 0:  Serial.println("Normal.");
//                   break;
//          case 1:  Serial.println("Stale Data.");
//                   break;
//          case 2:  Serial.println("In command mode.");
//                   break;
//          default: Serial.println("Diagnostic."); 
//                   break; 
//      }       
//    
//      RH = (float) H_dat * 6.10e-3;
//      T_C = (float) T_dat * 1.007e-2 - 40.0;
//
//      print_float(RH, 1);
//      Serial.print("  ");
//      print_float(T_C, 2);
//      Serial.println();
//      delay(1000);
//   }
//}

#endif
