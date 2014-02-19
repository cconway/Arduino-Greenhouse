#include <SPI.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include <lib_aci.h>
#include <aci_setup.h>
#include <Wire.h>
#include <Servo.h>
//#include <PID_v1.h>

#include "ble_housekeeping.h"
#include "lib_hih6100.h"
#include "lib_fsm.h"
#include "constants.h"

// USER-CONFIGURABLE VARIABLES
// -------------------------------------------------
float targetRelativeHumidity = 30.0f;

// GLOBAL VARIABLES
// -------------------------------------------------

// Finite State Machines
FiniteStateMachine climateStateMachine(5);  // Receives # of states to support

// Shift Register
int shiftRegLatchPin = 4;
int shiftRegClockPin = 2;
int shiftRegDataPin = 7;

// Humidity-&-Temp sensors
HIH6100_Sensor interiorHoneywell, exteriorHoneywell;

// Vent door servo
Servo ventDoorServo;
int ventDoorServoPin = 3;

// W.D. interrupt handler should be as short as possible, so it sets
//   watchdogWokeUp = 1 to indicate to run-loop that watchdog timer fired
volatile int watchdogWokeUp = 0;


void setup (void) {
  
  // Initiate our measurement trigger
  startWatchdogTimer();
  
  Serial.begin(115200);
  while(!Serial) {}  //  Wait until the serial port is available (useful only for the leonardo)
  Serial.println(F("Serial logging enabled"));
  
  // Configure Bluetooth LE support
  setDataReceivedCallbackFn(&processReceivedData);  // Pass our callback fn to the ble_housekeeping library
  ble_setup();
  
  // Configure State Machine
  setupClimateStateMachine();

  // Configure support for Honeywell sensors
  setupHoneywellSensors();
  
  // Configure vent door servo
  ventDoorServo.attach(ventDoorServoPin);
  
  // Turn on the lights
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  
  digitalWrite(5, HIGH);
  digitalWrite(6, HIGH);
}

void loop() {
  
  // Check whether the watchdog barked
  if (watchdogWokeUp) {
   
    watchdogWokeUp = 0;  // Reset flag until watchdog fires again
    
    performMeasurements();
    
    analyzeSystemState();
    
  }

  //Process any ACI commands or events
  aci_loop();
}


// BREAKOUT
// ----------------------------------------------------
void setupClimateStateMachine() {
  
  climateStateMachine.setCanTransition(ClimateStateSteady, ClimateStateDecreasingHumidity, true);
  climateStateMachine.setCanTransition(ClimateStateDecreasingHumidity, ClimateStateSteady, true);
  
  climateStateMachine.setDidEnterHandler(ClimateStateSteady, didEnterSteadyState);
  
  climateStateMachine.setDidEnterHandler(ClimateStateDecreasingHumidity, didEnterDecreasingHumidityState);
  climateStateMachine.setDidLeaveHandler(ClimateStateDecreasingHumidity, didLeaveDecreasingHumidityState);
  
  climateStateMachine.initialize();
}

void setupHoneywellSensors() {
  
  pinMode(shiftRegLatchPin, OUTPUT);
  pinMode(shiftRegDataPin, OUTPUT);
  pinMode(shiftRegClockPin, OUTPUT);
  
  enableHoneywellSensor(HoneywellSensorNone);
  Wire.begin();
}

void performMeasurements() {
  
  // Check Temp & Humidity inside and out
  // ---------------------------------------
  enableHoneywellSensor(HoneywellSensorExterior);
  delay(50);  // Allow sensor to wakeup
  exteriorHoneywell.performMeasurement();
  exteriorHoneywell.printStatus();
  
  enableHoneywellSensor(HoneywellSensorInterior);
  delay(50);  // Allow sensor to wakeup
  interiorHoneywell.performMeasurement();
  interiorHoneywell.printStatus();
  
  enableHoneywellSensor(HoneywellSensorNone);
}

void analyzeSystemState() {
  
  if (interiorHoneywell.humidity > targetRelativeHumidity && interiorHoneywell.humidity > exteriorHoneywell.humidity) {
    
    climateStateMachine.transitionToState(ClimateStateDecreasingHumidity);
    
  } else climateStateMachine.transitionToState(ClimateStateSteady);
  
}



// CLIMATE STATE MACHINE CALLBACKS
// ----------------------------------------------------

void didEnterSteadyState(int fromState, int toState) {
  
  ventDoorServo.write(VENT_DOOR_CLOSED);
}

void didEnterDecreasingHumidityState(int fromState, int toState) {
  
  ventDoorServo.write(VENT_DOOR_OPEN);
}

void didLeaveDecreasingHumidityState(int fromState, int toState) {
  
  ventDoorServo.write(VENT_DOOR_CLOSED);
}


// BLUETOOTH LE
// ----------------------------------------------------
void processReceivedData(uint8_t *bytes, uint8_t byteCount, uint8_t pipe) {
  
  switch (pipe) {
    
//    case PIPE_CUSTOM_THERMOMETER_ALARM_THRESHOLD_RX_ACK_AUTO: {  // Set alarm threshold value [float]
//      
//      if (byteCount != PIPE_CUSTOM_THERMOMETER_ALARM_THRESHOLD_RX_ACK_AUTO_MAX_SIZE) {
//
////        Serial.println(F("Write to Temp. Threshold value incorrect size"));
//        
//      } else {
//        
//        alarmArmed = false;
//        
//        thresholdTemperature = *((float *)bytes);
//        
////        Serial.print(F("Updating alarm threshold temp = "));
////        Serial.println(thresholdTemperature, 2);
//        
//        setValueForCharacteristic(PIPE_CUSTOM_THERMOMETER_ALARM_THRESHOLD_SET, thresholdTemperature);       
//      }
//      
//      break; 
//    }
  }
}


// MEASUREMENT
// ----------------------------------------------------
void enableHoneywellSensor(HoneywellSensor sensorID) {
  
   byte enabledOutputs;
   
   switch (sensorID) {
     
     case HoneywellSensorNone: enabledOutputs = B00000000; break;
     case HoneywellSensorExterior: enabledOutputs = B10000000; break;
     case HoneywellSensorInterior: enabledOutputs = B01000000; break;
     default: enabledOutputs = B00000000; break;
   }
   
   digitalWrite(shiftRegLatchPin, LOW);
   shiftOut(shiftRegDataPin, shiftRegClockPin, LSBFIRST, enabledOutputs);
   digitalWrite(shiftRegLatchPin, HIGH);
}


// WATCHDOG
// ----------------------------------------------------

// Enable the Watchdog timer and configure timer duration
void startWatchdogTimer() {
  
  // Clear the reset flag, the WDRF bit (bit 3) of MCUSR.
  MCUSR = MCUSR & B11110111;
    
  // Set the WDCE bit (bit 4) and the WDE bit (bit 3) 
  // of WDTCSR. The WDCE bit must be set in order to 
  // change WDE or the watchdog prescalers. Setting the 
  // WDCE bit will allow updtaes to the prescalers and 
  // WDE for 4 clock cycles then it will be reset by 
  // hardware.
  WDTCSR = WDTCSR | B00011000; 
  
  // Set the watchdog timeout prescaler value to 1024 K 
  // which will yeild a time-out interval of about 8.0 s.
  WDTCSR = B00100001;
  
  // Enable the watchdog timer interupt.
  WDTCSR = WDTCSR | B01000000;
  MCUSR = MCUSR & B11110111;

}

// Register function for Watchdog interrupt
ISR(WDT_vect) {
    
    watchdogWokeUp = 1;
    wdt_reset();  // Pet the Watchdog, stop it from forcing hardware reset
}
