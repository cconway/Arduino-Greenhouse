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
float humiditySetpoint = 30.0f;
float humidityNecessityCoeff = 1.0;  // Unit-less, coefficient for balancing humidity needs with temperature needs
//float humidityDriftAbove = 1.0f;
//float humidityDriftBelow = 1.0f;

float temperatureSetpoint = 21.111f;
float temperatureNecessityCoeff = 0.75;  // Unit-less, coefficient for balancing humidity needs with temperature needs
//float temperatureDriftAbove = 1.0f;
//float temperatureDriftBelow = 1.0f;

float ventingNecessityThreshold = 2.0f;  // Unit-less, takes into account balancing humidity and temperature setpoint targets

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


// CLIMATE STATE MACHINE CALLBACKS
// ----------------------------------------------------

void didEnterSteadyState(int fromState, int toState) {
  
  ventDoorServo.write(VENT_DOOR_CLOSED);
}

void didLeaveSteadyState(int fromState, int toState) {
  
  // NO-OP
}

void didEnterDecreasingHumidityState(int fromState, int toState) {
  
  ventDoorServo.write(VENT_DOOR_OPEN);
}

void didLeaveDecreasingHumidityState(int fromState, int toState) {
  
  ventDoorServo.write(VENT_DOOR_CLOSED);
}


// BREAKOUT
// ----------------------------------------------------
void setupClimateStateMachine() {
  
  climateStateMachine.setCanTransition(ClimateStateSteady, ClimateStateDecreasingHumidity, true);
  climateStateMachine.setCanTransition(ClimateStateDecreasingHumidity, ClimateStateSteady, true);
  
  climateStateMachine.setDidEnterHandler(ClimateStateSteady, didEnterSteadyState);
  climateStateMachine.setDidLeaveHandler(ClimateStateSteady, didLeaveSteadyState);
  
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
  
//  boolean canDecreaseHumidity = (interiorHoneywell.humidity > exteriorHoneywell.humidity);  // By opening vent
//  boolean canIncreaseHumidity = (interiorHoneywell.humidity < exteriorHoneywell.humidity);  // By opening vent
//  
//  boolean canDecreaseTemperature = (interiorHoneywell.temperature > exteriorHoneywell.temperature);  // By opening vent
//  boolean canIncreaseTemperature = (interiorHoneywell.temperature < exteriorHoneywell.temperature);  // By opening vent

  float humidityDeviation = interiorHoneywell.humidity - humiditySetpoint;  // + when interior is too humid
  float temperatureDeviation = interiorHoneywell.temperature - temperatureSetpoint; // + when interior is too warm

  float humidityDelta = interiorHoneywell.humidity - exteriorHoneywell.humidity;  // + when interior is more humid than exterior
  float temperatureDelta = interiorHoneywell.temperature - exteriorHoneywell.temperature;  // + when interior is warmer than exterior
  
  // Example H(i) = 31%, H(o) = 28.5%, H(s) = 30.0, T(i) = 22.3, T(o) = 21.9, T(s) = 23.0
  //    2.22             = ( 1.0                   * 2.5           * 1.0              ) + ( 1.0                      * 0.4              * -0.7                )
  float ventingNecessity = (humidityNecessityCoeff * humidityDelta * humidityDeviation) + (temperatureNecessityCoeff * temperatureDelta * temperatureDeviation);
  
  Serial.print("Venting necessity = ");
  Serial.println(ventingNecessity);
  
//  if (interiorHoneywell.humidity > humiditySetpoint && interiorHoneywell.humidity > exteriorHoneywell.humidity) {
  if (ventingNecessity >= ventingNecessityThreshold) {
    
    climateStateMachine.transitionToState(ClimateStateDecreasingHumidity);
    
  } else climateStateMachine.transitionToState(ClimateStateSteady);
  
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
