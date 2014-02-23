#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include <lib_aci.h>
#include <aci_setup.h>
#include <SPI.h>
#include <Wire.h>
#include <Servo.h>
#include <Time.h>

//#include <PID_v1.h>

//#include "ble_housekeeping.h"
#include "constants.h"
#include "lib_nordic.h"
#include "lib_ble.h"
#include "lib_hih6100.h"
#include "lib_fsm.h"


// USER-CONFIGURABLE VARIABLES
// -------------------------------------------------
float humiditySetpoint = 30.0f;
float humidityNecessityCoeff = 1.0;  // Unit-less, coefficient for balancing humidity needs with temperature needs

float temperatureSetpoint = 21.0f;
float temperatureNecessityCoeff = 0.75;  // Unit-less, coefficient for balancing humidity needs with temperature needs

float ventingNecessityThreshold = 8.0f;  // Unit-less, takes into account balancing humidity and temperature setpoint targets
float ventingNecessityOvershoot = 25.0f;  // Percent of ventingNecessityThreshold to overshoot by to reduce frequency of cycling
float targetVentingNecessity = UNAVAILABLE_f;  // Venting necessity value at which to stop venting

int illuminationOnMinutes = UNAVAILABLE_u;
int illuminationOffMinutes = UNAVAILABLE_u;

// GLOBAL VARIABLES
// -------------------------------------------------

// Bluetooth Low Energy (BLE)
BLE BLE_board(handleACIEvent);

// Finite State Machines
FiniteStateMachine climateStateMachine(5, stateWillChange, stateDidChange);  // Receives # of states to support

// Shift Register
int shiftRegLatchPin = 4;
int shiftRegClockPin = 2;
int shiftRegDataPin = 7;
byte shiftRegisterState;

// Humidity-&-Temp sensors
HIH6100_Sensor interiorHoneywell, exteriorHoneywell;

// Vent door servo
Servo ventDoorServo;
int ventDoorServoPin = 3;

// Light banks
int lightBank1DutyCycle = HIGH;
int lightBank2DutyCycle = HIGH;

// W.D. interrupt handler should be as short as possible, so it sets
//   watchdogWokeUp = 1 to indicate to run-loop that watchdog timer fired
volatile int watchdogWokeUp = 0;


void setup (void) {
  
  Serial.begin(57600);
  while(!Serial) {}  //  Wait until the serial port is available (useful only for the leonardo)
  Serial.println(F("Serial logging enabled"));
  
  // Initiate our measurement trigger
  startWatchdogTimer();
//  wdt_enable(WDTO_4S);

// Configure Bluetooth LE support
  setACIPostEventHandler(handleACIEvent);
  aci_setup();
  
  // Configure State Machine
  setupClimateStateMachine();

  // Configure support for Honeywell sensors
  setupHoneywellSensors();
  
  // Configure vent door servo
  ventDoorServo.attach(ventDoorServoPin);
  
  // Enable illumination control
  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
}

void loop() {
  
  // Check whether the watchdog barked
  if (watchdogWokeUp) {
   
    Serial.println(F("Watchdog barked!"));
    
    watchdogWokeUp = 0;  // Reset flag until watchdog fires again
    
    performMeasurements();
    
    analyzeSystemState();
    
    checkIlluminationTimer();
  }

  //Process any ACI commands or events
  aci_loop();

//  Serial.println(F("Runloop finished"));
}


// CLIMATE STATE MACHINE CALLBACKS
// ----------------------------------------------------
void stateWillChange(uint8_t fromState, uint8_t toState) {
  
  
}

void stateDidChange(uint8_t fromState, uint8_t toState) {
  
  // Send current state to BLE board
//  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_STATE_CLIMATE_CONTROL_STATE_SET, (uint8_t) toState);
//  BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_STATE_CLIMATE_CONTROL_STATE_TX, (uint8_t) toState);
  
  switch (fromState) {  // DID LEAVE...
    
    case ClimateStateSteady: {
      
      didLeaveSteadyState(fromState, toState);
      break;
    }
    
    case ClimateStateDecreasingHumidity: {
      
      didLeaveDecreasingHumidityState(fromState, toState);
      break;
    }
  }  // end switch(fromState)
  
  
  switch (toState) {  // DID ENTER...
    
    case ClimateStateSteady: {
      
      didEnterSteadyState(fromState, toState);
      break;
    }
    
    case ClimateStateDecreasingHumidity: {
      
      didEnterDecreasingHumidityState(fromState, toState);
      break;
    }
  }  // end switch(toState)
  
}

// -----------------------------------------------------------------

void didEnterSteadyState(uint8_t fromState, uint8_t toState) {
  
  ventDoorServo.write(VENT_DOOR_CLOSED);
  
//  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_CONTROLS_VENT_SERVO_POSITION_SET, (uint8_t) ventDoorServo.read());
//  BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_CONTROLS_VENT_SERVO_POSITION_TX, (uint8_t) ventDoorServo.read());
  
  targetVentingNecessity = UNAVAILABLE_f;
  
//  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_STATE_VENTING_NECESSITY_TARGET_SET, targetVentingNecessity);
//  BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_STATE_VENTING_NECESSITY_TARGET_TX, targetVentingNecessity);
}

void didLeaveSteadyState(uint8_t fromState, uint8_t toState) {
  
  // NO-OP
}

void didEnterDecreasingHumidityState(uint8_t fromState, uint8_t toState) {
  
  ventDoorServo.write(VENT_DOOR_OPEN);
  
//  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_CONTROLS_VENT_SERVO_POSITION_SET, (uint8_t) ventDoorServo.read());
//  BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_CONTROLS_VENT_SERVO_POSITION_TX, (uint8_t) ventDoorServo.read());
}

void didLeaveDecreasingHumidityState(uint8_t fromState, uint8_t toState) {
  
  ventDoorServo.write(VENT_DOOR_CLOSED);
  
//  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_CONTROLS_VENT_SERVO_POSITION_SET, (uint8_t) ventDoorServo.read());
//  BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_CONTROLS_VENT_SERVO_POSITION_TX, (uint8_t) ventDoorServo.read());
  
}

// BREAKOUT
// ----------------------------------------------------
void updateBluetoothReadPipes() {
  
  delay(100);  // Need to let BLE board settle before we fill set-pipes otherwise first command will be ignored
  //BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_TEMPERATURE_SETPOINT_SET, 12.34f);  // Throwaway to prime the pump
  
  Serial.println(F("Updating infrequently changed set-pipes"));
  
  // User Adjustments  
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_TEMPERATURE_SETPOINT_SET, temperatureSetpoint);
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_HUMIDITY_SETPOINT_SET, humiditySetpoint);
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_HUMIDITY_NECESSITY_COEFF_SET, humidityNecessityCoeff);
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_TEMPERATURE_NECESSITY_COEFF_SET, temperatureNecessityCoeff);
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_VENTING_NECESSITY_THRESHOLD_SET, ventingNecessityThreshold);
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_VENTING_NECESSITY_OVERSHOOT_SET, ventingNecessityOvershoot);
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_ILLUMINATION_ON_TIME_SET, illuminationOnMinutes);
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_ILLUMINATION_OFF_TIME_SET, illuminationOffMinutes);
  
  // Climate Control State
//  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_STATE_VENTING_NECESSITY_SET, 0);
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_STATE_VENTING_NECESSITY_TARGET_SET, targetVentingNecessity);
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_STATE_CLIMATE_CONTROL_STATE_SET, (uint8_t) climateStateMachine.getCurrentState());
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_STATE_SHIFT_REGISTER_STATE_SET, shiftRegisterState);
  
  // Controls
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_CONTROLS_LIGHT_BANK_1_DUTY_CYCLE_SET, (uint8_t) lightBank1DutyCycle);
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_CONTROLS_LIGHT_BANK_2_DUTY_CYCLE_SET, (uint8_t) lightBank2DutyCycle);
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_CONTROLS_VENT_SERVO_POSITION_SET, (uint8_t) ventDoorServo.read());
  
  // Measurements
//  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_EXTERIOR_HUMIDITY_SET, shiftRegisterState);
//  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_EXTERIOR_TEMPERATURE_SET, shiftRegisterState);
//  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_INTERIOR_HUMIDITY_SET, shiftRegisterState);
//  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_INTERIOR_TEMPERATURE_SET, shiftRegisterState);
}

void setupClimateStateMachine() {
  
  // NOTE: Still need to figure out how to handle cases where handler fn isn't set without crashing!!!
  
  climateStateMachine.setCanTransition(ClimateStateSteady, ClimateStateDecreasingHumidity, true);
  climateStateMachine.setCanTransition(ClimateStateDecreasingHumidity, ClimateStateSteady, true);
  
//  climateStateMachine.setDidEnterHandler(ClimateStateSteady, didEnterSteadyState);
//  climateStateMachine.setDidLeaveHandler(ClimateStateSteady, didLeaveSteadyState);
//  
//  climateStateMachine.setDidEnterHandler(ClimateStateDecreasingHumidity, didEnterDecreasingHumidityState);
//  climateStateMachine.setDidLeaveHandler(ClimateStateDecreasingHumidity, didLeaveDecreasingHumidityState);
    
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
  
  // Exterior
  enableHoneywellSensor(HoneywellSensorExterior);
  delay(50);  // Allow sensor to wakeup
  exteriorHoneywell.performMeasurement();
//  exteriorHoneywell.printStatus();
  
//  //BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_EXTERIOR_HUMIDITY_SET, exteriorHoneywell.humidity);
  BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_EXTERIOR_HUMIDITY_TX, exteriorHoneywell.humidity);
  
//  //BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_EXTERIOR_TEMPERATURE_SET, exteriorHoneywell.temperature);
  BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_EXTERIOR_TEMPERATURE_TX, exteriorHoneywell.temperature);
  
  // Interior
  enableHoneywellSensor(HoneywellSensorInterior);
  delay(50);  // Allow sensor to wakeup
  interiorHoneywell.performMeasurement();
//  interiorHoneywell.printStatus();
  
//  //BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_INTERIOR_HUMIDITY_SET, interiorHoneywell.humidity);
  BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_INTERIOR_HUMIDITY_TX, interiorHoneywell.humidity);
  
//  //BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_INTERIOR_TEMPERATURE_SET, interiorHoneywell.temperature);
  BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_INTERIOR_TEMPERATURE_TX, interiorHoneywell.temperature);
  
  // Cleanup
  enableHoneywellSensor(HoneywellSensorNone);
}

void analyzeSystemState() {

  float humidityDeviation = interiorHoneywell.humidity - humiditySetpoint;  // + when interior is too humid
  float temperatureDeviation = interiorHoneywell.temperature - temperatureSetpoint; // + when interior is too warm

  float humidityDelta = interiorHoneywell.humidity - exteriorHoneywell.humidity;  // + when interior is more humid than exterior
  float temperatureDelta = interiorHoneywell.temperature - exteriorHoneywell.temperature;  // + when interior is warmer than exterior
  
  // Example H(i) = 31%, H(o) = 28.5%, H(s) = 30.0, T(i) = 22.3, T(o) = 21.9, T(s) = 23.0
  //    2.22             = ( 1.0                   * 2.5           * 1.0              ) + ( 1.0                      * 0.4              * -0.7                )
  float ventingNecessity = (humidityNecessityCoeff * humidityDelta * humidityDeviation) + (temperatureNecessityCoeff * temperatureDelta * temperatureDeviation);
  
  
  //BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_STATE_VENTING_NECESSITY_SET, ventingNecessity);
  //BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_STATE_VENTING_NECESSITY_TX, ventingNecessity);
  
  
  switch ( climateStateMachine.getCurrentState() ) {
    
    case ClimateStateSteady: {
      
      if (ventingNecessity >= ventingNecessityThreshold) {  // Trigger right at the threshold
        
        climateStateMachine.transitionToState(ClimateStateDecreasingHumidity);
      }
      
      break;
    }
    
    case ClimateStateDecreasingHumidity: {
      
      targetVentingNecessity = (ventingNecessityThreshold * (1 - (ventingNecessityOvershoot / 100.0)));
      
      //BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_STATE_VENTING_NECESSITY_TARGET_SET, targetVentingNecessity);
      //BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_STATE_VENTING_NECESSITY_TARGET_TX, targetVentingNecessity);
      
      if (ventingNecessity <= targetVentingNecessity) {  // Trigger right at the threshold
        
        climateStateMachine.transitionToState(ClimateStateSteady);
      }
      
      break;
    }
    
    default: {
      
      climateStateMachine.transitionToState(ClimateStateSteady);
      
      break;
    }
  }
}

void checkIlluminationTimer() {
  
  int minutesSinceMidnight = (60 * hour()) + minute();
  
  if (timeStatus() != timeSet || illuminationOnMinutes == UNAVAILABLE_u || illuminationOffMinutes == UNAVAILABLE_u) {
    
    // Default to ON all the time if no times set
    lightBank1DutyCycle = HIGH;
    lightBank2DutyCycle = HIGH;
    
  } else if (illuminationOnMinutes > illuminationOffMinutes) {
    
    if (minutesSinceMidnight > illuminationOnMinutes || minutesSinceMidnight <= illuminationOffMinutes) {
      
      lightBank1DutyCycle = HIGH;
      lightBank2DutyCycle = HIGH;
    
    } else {
      
      lightBank1DutyCycle = LOW;
      lightBank2DutyCycle = LOW;
    }
  
  } else {  // illuminationOnMinutes <= illuminationOffMinutes
    
    if (minutesSinceMidnight > illuminationOnMinutes && minutesSinceMidnight <= illuminationOffMinutes) {
      
      lightBank1DutyCycle = HIGH;
      lightBank2DutyCycle = HIGH;
    
    } else {
      
      lightBank1DutyCycle = LOW;
      lightBank2DutyCycle = LOW;
    }
  }
    
  digitalWrite(5, lightBank1DutyCycle);
  digitalWrite(6, lightBank2DutyCycle);
  
  //BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_CONTROLS_LIGHT_BANK_1_DUTY_CYCLE_SET, (uint8_t) lightBank1DutyCycle);
  //BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_CONTROLS_LIGHT_BANK_1_DUTY_CYCLE_TX, (uint8_t) lightBank1DutyCycle);
  
  //BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_CONTROLS_LIGHT_BANK_2_DUTY_CYCLE_SET, (uint8_t) lightBank2DutyCycle);
  //BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_CONTROLS_LIGHT_BANK_2_DUTY_CYCLE_TX, (uint8_t) lightBank2DutyCycle);
}



// BLUETOOTH LE
// ----------------------------------------------------
void handleACIEvent(aci_state_t *aci_state, aci_evt_t *aci_evt) {
  
  switch (aci_evt->evt_opcode) {  // Switch based on Event Op-Code
  
    case ACI_EVT_DEVICE_STARTED: {  // As soon as you reset the nRF8001 you will get an ACI Device Started Event
    
      if (aci_evt->params.device_started.device_mode == ACI_DEVICE_STANDBY) {
        
        // Copy intial values into BLE board
        updateBluetoothReadPipes();
      }
    
      break;
    }
  
    case ACI_EVT_DATA_RECEIVED: {  // One of the writeable pipes (for a Characteristic) has received data
      
      uint8_t *bytes = &aci_evt->params.data_received.rx_data.aci_data[0];
      uint8_t byteCount = aci_evt->len - 2;
      uint8_t pipe = aci_evt->params.data_received.rx_data.pipe_number;
      
      receivedDataFromPipe(bytes, byteCount, pipe);
      
      break;
    }
    
  }  // end switch(aci_evt->evt_opcode);
}


// USER-CONFIGURATION
// ----------------------------------------------------
boolean valueWithinLimits(float value, float minLimit, float maxLimit) {
  
  return (value >= minLimit && value <= maxLimit);
}

// Route incoming data to the correct state variables
boolean receivedDataFromPipe(uint8_t *bytes, uint8_t byteCount, uint8_t pipe) {
  
  switch (pipe) {

    case PIPE_GREENHOUSE_USER_ADJUSTMENTS_TEMPERATURE_SETPOINT_RX_ACK_AUTO: {
      
      if (byteCount == PIPE_GREENHOUSE_USER_ADJUSTMENTS_TEMPERATURE_SETPOINT_RX_ACK_AUTO_MAX_SIZE) {
      
        float floatValue = *((float *)bytes);
        if ( valueWithinLimits(floatValue, TEMPERATURE_SETPOINT_MIN, TEMPERATURE_SETPOINT_MAX) ) {
          
          temperatureSetpoint = floatValue;
          //BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_TEMPERATURE_SETPOINT_SET, floatValue); 
        }
      }
      break;
    }
    
    case PIPE_GREENHOUSE_USER_ADJUSTMENTS_HUMIDITY_SETPOINT_RX_ACK_AUTO: {
      
      if (byteCount == PIPE_GREENHOUSE_USER_ADJUSTMENTS_HUMIDITY_SETPOINT_RX_ACK_AUTO_MAX_SIZE) {
      
        float floatValue = *((float *)bytes);
        if ( valueWithinLimits(floatValue, HUMIDITY_SETPOINT_MIN, HUMIDITY_SETPOINT_MAX) ) {
          
          humiditySetpoint = floatValue;
          //BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_HUMIDITY_SETPOINT_SET, floatValue); 
        }
      }
      break;
    }
    
    case PIPE_GREENHOUSE_USER_ADJUSTMENTS_HUMIDITY_NECESSITY_COEFF_RX_ACK_AUTO: {
      
      if (byteCount == PIPE_GREENHOUSE_USER_ADJUSTMENTS_HUMIDITY_NECESSITY_COEFF_RX_ACK_AUTO_MAX_SIZE) {
      
        float floatValue = *((float *)bytes);
        if ( valueWithinLimits(floatValue, NECESSITY_COEFF_MIN, NECESSITY_COEFF_MAX) ) {
          
          humidityNecessityCoeff = floatValue;
          //BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_HUMIDITY_NECESSITY_COEFF_SET, floatValue); 
        }
      }
      break;
    }
    
    case PIPE_GREENHOUSE_USER_ADJUSTMENTS_TEMPERATURE_NECESSITY_COEFF_RX_ACK_AUTO: {
      
      if (byteCount == PIPE_GREENHOUSE_USER_ADJUSTMENTS_TEMPERATURE_NECESSITY_COEFF_RX_ACK_AUTO_MAX_SIZE) {
      
        float floatValue = *((float *)bytes);
        if ( valueWithinLimits(floatValue, NECESSITY_COEFF_MIN, NECESSITY_COEFF_MAX) ) {
          
          temperatureNecessityCoeff = floatValue;
          //BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_TEMPERATURE_NECESSITY_COEFF_SET, floatValue); 
        }
      }
      break;
    }
    
    case PIPE_GREENHOUSE_USER_ADJUSTMENTS_VENTING_NECESSITY_THRESHOLD_RX_ACK_AUTO: {
      
      if (byteCount == PIPE_GREENHOUSE_USER_ADJUSTMENTS_VENTING_NECESSITY_THRESHOLD_RX_ACK_AUTO_MAX_SIZE) {
      
        float floatValue = *((float *)bytes);
        if ( valueWithinLimits(floatValue, NECESSITY_THRESHOLD_MIN, NECESSITY_THRESHOLD_MAX) ) {
          
          ventingNecessityThreshold = floatValue;
          //BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_VENTING_NECESSITY_THRESHOLD_SET, floatValue); 
        }
      }
      break;
    }
    
    case PIPE_GREENHOUSE_USER_ADJUSTMENTS_VENTING_NECESSITY_OVERSHOOT_RX_ACK_AUTO: {
      
      if (byteCount == PIPE_GREENHOUSE_USER_ADJUSTMENTS_VENTING_NECESSITY_OVERSHOOT_RX_ACK_AUTO_MAX_SIZE) {
      
        float floatValue = *((float *)bytes);
        if ( valueWithinLimits(floatValue, VENTING_OVERSHOOT_MIN, VENTING_OVERSHOOT_MAX) ) {
          
          ventingNecessityOvershoot = floatValue;
          //BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_VENTING_NECESSITY_OVERSHOOT_SET, floatValue); 
        }
      }
      break;
    }
    
    case PIPE_GREENHOUSE_USER_ADJUSTMENTS_DATETIME_RX_ACK_AUTO: {
      
      if (byteCount == PIPE_GREENHOUSE_USER_ADJUSTMENTS_DATETIME_RX_ACK_AUTO_MAX_SIZE) {
      
        unsigned long bleHostTime = *((unsigned long *)bytes);
        setTime(bleHostTime);
        
      }
      
      break;
    }
    
    case PIPE_GREENHOUSE_USER_ADJUSTMENTS_ILLUMINATION_ON_TIME_RX_ACK_AUTO: {
      
      if (byteCount == PIPE_GREENHOUSE_USER_ADJUSTMENTS_ILLUMINATION_ON_TIME_RX_ACK_AUTO_MAX_SIZE) {
      
        illuminationOnMinutes = *((int *)bytes);  // NOTE: Expecting that 'int' type is 2 bytes
        
        //BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_ILLUMINATION_ON_TIME_SET, illuminationOnMinutes);
      }
      
      break;
    }
    
    case PIPE_GREENHOUSE_USER_ADJUSTMENTS_ILLUMINATION_OFF_TIME_RX_ACK_AUTO: {
      
      if (byteCount == PIPE_GREENHOUSE_USER_ADJUSTMENTS_ILLUMINATION_OFF_TIME_RX_ACK_AUTO_MAX_SIZE) {
      
        illuminationOffMinutes = *((int *)bytes);  // NOTE: Expecting that 'int' type is 2 bytes
        
        //BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_ILLUMINATION_OFF_TIME_SET, illuminationOffMinutes);
      }
      
      break;
    }

  }  // end switch(pipe)
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
   
   shiftRegisterState = enabledOutputs;
   
   //BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_STATE_SHIFT_REGISTER_STATE_SET, shiftRegisterState);
   //BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_STATE_SHIFT_REGISTER_STATE_TX, shiftRegisterState);
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
//  WDTCSR = B00100001;  // ~8s
  WDTCSR = B00100000;  // ~4s
  
  // Enable the watchdog timer interupt.
  WDTCSR = WDTCSR | B01000000;
  MCUSR = MCUSR & B11110111;

}

// Register function for Watchdog interrupt
ISR(WDT_vect) {
    
    watchdogWokeUp = 1;
    wdt_reset();  // Pet the Watchdog, stop it from forcing hardware reset
}
