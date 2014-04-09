// LICENSES: [a1cdbd]
// -----------------------------------
// The contents of this file contains the aggregate of contributions
//   covered under one or more licences. The full text of those licenses
//   can be found in the "LICENSES" file at the top level of this project
//   identified by the MD5 fingerprints listed above.

#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <EEPROM.h>

#include <lib_aci.h>
#include <aci_setup.h>
#include <SPI.h>
#include <Wire.h>
#include <Servo.h>
#include <Time.h>
#include <PID_v1.h>

#include "constants.h"
#include "services.h"
#include "lib_ble.h"
#include "lib_hih6100.h"
#include "lib_fsm.h"
#include "lib_timeSeries.h"


// USER-CONFIGURABLE VARIABLES
// -------------------------------------------------
UserConfig currentConfig;

// GLOBAL VARIABLES
// -------------------------------------------------

// Bluetooth Low Energy (BLE)
BLE BLE_board(handleACIEvent);

// Finite State Machines
FiniteStateMachine climateStateMachine(5, stateWillChange, stateDidChange);  // Receives # of states to support

// TimeSeries storage
TimeSeries humiditySeries;
TimeSeries humidityChangeSeries;
float maxHumidityChangeThisCycle = UNAVAILABLE_f;
float peakHumidityChange = UNAVAILABLE_f;

// PID control
boolean hasInitialData = false;
double ventingNecessity = UNAVAILABLE_f;
double ventFlapPosition = VENT_DOOR_CLOSED;
double setpoint = 0;
PID ventFlapPID(&ventingNecessity, &ventFlapPosition, &setpoint, 2.5, 0.25, 0.5, DIRECT);

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
//   watchdogWokeUp = true to indicate to run-loop that watchdog timer fired
volatile boolean watchdogWokeUp = false;

void setup (void) {

//  Serial.begin(57600);
//  while(!Serial) {}  //  Wait until the serial port is available (useful only for the leonardo)
  Serial.println(F("Serial logging enabled"));
  
  restoreConfiguration();
  
  // Initiate our measurement trigger
  startWatchdogTimer();
//  wdt_enable(WDTO_4S);

// Configure Bluetooth LE support
//  setACIPostEventHandler(handleACIEvent);
  BLE_board.ble_setup();
  
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
   
//    Serial.println(F("Watchdog barked!"));
    
    watchdogWokeUp = false;  // Reset flag until watchdog fires again
    
    performMeasurements();
    
    analyzeSystemState();
    
    checkIlluminationTimer();
  }

  //Process any ACI commands or events
  BLE_board.ble_loop();
  
}


// STUCK-IN-LOOP DETECTIOn
// ----------------------------------------------------
void detectHungLoop() {
  
  if (BLE_board.loopingSinceLastBark) {
    
    // Assume we're in an inf-loop, force us out to try and recover
    BLE_board._aci_cmd_pending = false;
    BLE_board._data_credit_pending  = false;
    
  } else if (BLE_board._aci_cmd_pending || BLE_board._data_credit_pending) {
    
    // Set a marker so we can see if either loop has exited by the next time the watchdog barks
    BLE_board.loopingSinceLastBark = true;
  
  }  // else do nothing, everything's peachy
}

// CLIMATE STATE MACHINE CALLBACKS
// ----------------------------------------------------
void stateWillChange(uint8_t fromState, uint8_t toState) {
  
  switch (fromState) {  // WILL LEAVE...
    
    case ClimateStateSteady: {
      
      humiditySeries.clearAll();
      humidityChangeSeries.clearAll();
      
      // Copy max last cycle to peak in case it didn't exceed previous peak
      peakHumidityChange = maxHumidityChangeThisCycle;
      maxHumidityChangeThisCycle = 0;
      
      BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_PEAK_HUMIDITY_RISE_SET, peakHumidityChange);
      BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_PEAK_HUMIDITY_RISE_TX, peakHumidityChange);
      
//      Serial.print("Gaining ");
//      Serial.print(peakHumidityChange);
//      Serial.println(" % RH/min");
 
      break;
    }
  }
}

void stateDidChange(uint8_t fromState, uint8_t toState) {
  
  // Send current state to BLE board
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_STATE_CLIMATE_CONTROL_STATE_SET, (uint8_t) toState);
  BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_STATE_CLIMATE_CONTROL_STATE_TX, (uint8_t) toState);
  
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
  
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_CONTROLS_VENT_SERVO_POSITION_SET, (uint8_t) ventDoorServo.read());
  BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_CONTROLS_VENT_SERVO_POSITION_TX, (uint8_t) ventDoorServo.read());
  
  currentConfig.targetVentingNecessity = UNAVAILABLE_f;
  
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_STATE_VENTING_NECESSITY_TARGET_SET, currentConfig.targetVentingNecessity);
  BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_STATE_VENTING_NECESSITY_TARGET_TX, currentConfig.targetVentingNecessity);
}

void didLeaveSteadyState(uint8_t fromState, uint8_t toState) {
  
  // NO-OP
}

void didEnterDecreasingHumidityState(uint8_t fromState, uint8_t toState) {
  
  ventDoorServo.write(VENT_DOOR_OPEN);
  
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_CONTROLS_VENT_SERVO_POSITION_SET, (uint8_t) ventDoorServo.read());
  BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_CONTROLS_VENT_SERVO_POSITION_TX, (uint8_t) ventDoorServo.read());
}

void didLeaveDecreasingHumidityState(uint8_t fromState, uint8_t toState) {
  
  ventDoorServo.write(VENT_DOOR_CLOSED);
  
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_CONTROLS_VENT_SERVO_POSITION_SET, (uint8_t) ventDoorServo.read());
  BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_CONTROLS_VENT_SERVO_POSITION_TX, (uint8_t) ventDoorServo.read());
  
}

// BREAKOUT
// ----------------------------------------------------
void updateBluetoothReadPipes() {
  
  delay(100);  // Need to let BLE board settle before we fill set-pipes otherwise first command will be ignored
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_TEMPERATURE_SETPOINT_SET, 12.34f);  // Throwaway to prime the pump
  
  Serial.println(F("Updating infrequently changed set-pipes"));
  
  // User Adjustments  
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_TEMPERATURE_SETPOINT_SET, currentConfig.temperatureSetpoint);
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_HUMIDITY_SETPOINT_SET, currentConfig.humiditySetpoint);
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_HUMIDITY_NECESSITY_COEFF_SET, currentConfig.humidityNecessityCoeff);
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_TEMPERATURE_NECESSITY_COEFF_SET, currentConfig.temperatureNecessityCoeff);
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_VENTING_NECESSITY_THRESHOLD_SET, currentConfig.ventingNecessityThreshold);
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_VENTING_NECESSITY_OVERSHOOT_SET, currentConfig.ventingNecessityOvershoot);
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_ILLUMINATION_ON_TIME_SET, currentConfig.illuminationOnMinutes);
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_ILLUMINATION_OFF_TIME_SET, currentConfig.illuminationOffMinutes);
  
  // Climate Control State
//  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_STATE_VENTING_NECESSITY_SET, 0);
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_STATE_VENTING_NECESSITY_TARGET_SET, currentConfig.targetVentingNecessity);
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_STATE_CLIMATE_CONTROL_STATE_SET, (uint8_t) climateStateMachine.getCurrentState());
//  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_STATE_SHIFT_REGISTER_STATE_SET, shiftRegisterState);
  
  // Controls
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_CONTROLS_LIGHT_BANK_1_DUTY_CYCLE_SET, (uint8_t) lightBank1DutyCycle);
//  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_CONTROLS_LIGHT_BANK_2_DUTY_CYCLE_SET, (uint8_t) lightBank2DutyCycle);
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_CONTROLS_VENT_SERVO_POSITION_SET, (uint8_t) ventDoorServo.read());
  
  // Measurements
//  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_EXTERIOR_HUMIDITY_SET, shiftRegisterState);
//  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_EXTERIOR_TEMPERATURE_SET, shiftRegisterState);
//  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_INTERIOR_HUMIDITY_SET, shiftRegisterState);
//  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_INTERIOR_TEMPERATURE_SET, shiftRegisterState);
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_PEAK_HUMIDITY_RISE_SET, peakHumidityChange);
}

void setupClimateStateMachine() {
  
  // NOTE: Still need to figure out how to handle cases where handler fn isn't set without crashing!!!
  
  climateStateMachine.setCanTransition(ClimateStateSteady, ClimateStateDecreasingHumidity, true);
  climateStateMachine.setCanTransition(ClimateStateDecreasingHumidity, ClimateStateSteady, true);
    
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
  
  BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_EXTERIOR_HUMIDITY_TX, exteriorHoneywell.humidity);
  BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_EXTERIOR_TEMPERATURE_TX, exteriorHoneywell.temperature);
  
  // Interior
  enableHoneywellSensor(HoneywellSensorInterior);
  delay(50);  // Allow sensor to wakeup
  interiorHoneywell.performMeasurement();
//  interiorHoneywell.printStatus();
  
  BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_INTERIOR_HUMIDITY_TX, interiorHoneywell.humidity);
  BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_INTERIOR_TEMPERATURE_TX, interiorHoneywell.temperature);
  
  // Cleanup
  enableHoneywellSensor(HoneywellSensorNone);
}

void analyzeSystemState() {
  
  // Calculate our Venting Necessity
  float humidityDeviation = interiorHoneywell.humidity - currentConfig.humiditySetpoint;  // + when interior is too humid
  float temperatureDeviation = interiorHoneywell.temperature - currentConfig.temperatureSetpoint; // + when interior is too warm

  float humidityDelta = interiorHoneywell.humidity - exteriorHoneywell.humidity;  // + when interior is more humid than exterior
  float temperatureDelta = interiorHoneywell.temperature - exteriorHoneywell.temperature;  // + when interior is warmer than exterior
  
  // Example H(i) = 31%, H(o) = 28.5%, H(s) = 30.0, T(i) = 22.3, T(o) = 21.9, T(s) = 23.0
  //    2.22             = ( 1.0                   * 2.5           * 1.0              ) + ( 1.0                      * 0.4              * -0.7                )
  ventingNecessity = (currentConfig.humidityNecessityCoeff * humidityDelta * humidityDeviation) + (currentConfig.temperatureNecessityCoeff * temperatureDelta * temperatureDeviation);
  
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_STATE_VENTING_NECESSITY_SET, (float)ventingNecessity);
  BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_STATE_VENTING_NECESSITY_TX, (float)ventingNecessity);
  
  
  // Set Vent Flap servo position based on PID controller
  if (hasInitialData) {
    
    ventFlapPID.Compute();
//    Serial.print("Servo position = ");
//    Serial.println(ventFlapPosition);
    
    ventDoorServo.write(ventFlapPosition);
    
  } else {
    
    hasInitialData = true;
    
    // Turn the PID on
    // NOTE: To raise venting necessity you would raise the output value (towards vent flap closure)
    //   so relationship is 'direct', not 'reverse'
    ventFlapPID.SetSampleTime(3000);  // 3sec, less than watchdog cycle
    ventFlapPID.SetOutputLimits(VENT_DOOR_OPEN, VENT_DOOR_CLOSED);  // (min, max)
    ventFlapPID.SetMode(AUTOMATIC);
  }
  
  
  /*
  // Determine if we need to change states
  switch ( climateStateMachine.getCurrentState() ) {
    
    case ClimateStateSteady: {
      
      // Perform timeseries analysis
      humiditySeries.addValue(interiorHoneywell.humidity);
      
      float slope = humiditySeries.averageSlope();
      if (slope != UNAVAILABLE_f) {
        
        float percentPerMinute = slope * 60.0;  // Scale %-per-second to minutes
        humidityChangeSeries.addValue(percentPerMinute);
        
        float rollingAverage = humidityChangeSeries.averageValue();
        if (rollingAverage > maxHumidityChangeThisCycle) {
          
          maxHumidityChangeThisCycle = rollingAverage;
        }
        
        if (rollingAverage > peakHumidityChange) {  // Update immediately if current rolling average > peak
          
          peakHumidityChange = rollingAverage;
          
          BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_PEAK_HUMIDITY_RISE_SET, peakHumidityChange);
          BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_PEAK_HUMIDITY_RISE_TX, peakHumidityChange);
        }
      
      } else {  // No slope info available
        
        BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_PEAK_HUMIDITY_RISE_SET, UNAVAILABLE_f);
        BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_MEASUREMENTS_PEAK_HUMIDITY_RISE_TX, UNAVAILABLE_f);
      }
      
      // Trigger state change when venting necessity exceeds threshold
      if (ventingNecessity >= currentConfig.ventingNecessityThreshold) {  // Trigger right at the threshold
        
        climateStateMachine.transitionToState(ClimateStateDecreasingHumidity);
      }
      
      break;
    }
    
    case ClimateStateDecreasingHumidity: {
      
      currentConfig.targetVentingNecessity = (currentConfig.ventingNecessityThreshold * (1 - (currentConfig.ventingNecessityOvershoot / 100.0)));
      
      BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_STATE_VENTING_NECESSITY_TARGET_SET, currentConfig.targetVentingNecessity);
      BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_STATE_VENTING_NECESSITY_TARGET_TX, currentConfig.targetVentingNecessity);
      
      if (ventingNecessity <= currentConfig.targetVentingNecessity) {  // Trigger only when we reach the lower target
        
        climateStateMachine.transitionToState(ClimateStateSteady);
      }
      
      break;
    }
    
    default: {
      
      climateStateMachine.transitionToState(ClimateStateSteady);
      
      break;
    }
  }*/
}

void checkIlluminationTimer() {
  
  int minutesSinceMidnight = (60 * hour()) + minute();
  
  if (timeStatus() != timeSet || currentConfig.illuminationOnMinutes == UNAVAILABLE_u || currentConfig.illuminationOffMinutes == UNAVAILABLE_u) {
    
    // Default to ON all the time if no times set
    lightBank1DutyCycle = HIGH;
    lightBank2DutyCycle = HIGH;
    
  } else if (currentConfig.illuminationOnMinutes > currentConfig.illuminationOffMinutes) {
    
    if (minutesSinceMidnight > currentConfig.illuminationOnMinutes || minutesSinceMidnight <= currentConfig.illuminationOffMinutes) {
      
      lightBank1DutyCycle = HIGH;
      lightBank2DutyCycle = HIGH;
    
    } else {
      
      lightBank1DutyCycle = LOW;
      lightBank2DutyCycle = LOW;
    }
  
  } else {  // illuminationOnMinutes <= illuminationOffMinutes
    
    if (minutesSinceMidnight > currentConfig.illuminationOnMinutes && minutesSinceMidnight <= currentConfig.illuminationOffMinutes) {
      
      lightBank1DutyCycle = HIGH;
      lightBank2DutyCycle = HIGH;
    
    } else {
      
      lightBank1DutyCycle = LOW;
      lightBank2DutyCycle = LOW;
    }
  }
    
  digitalWrite(5, lightBank1DutyCycle);
  digitalWrite(6, lightBank2DutyCycle);
  
  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_CONTROLS_LIGHT_BANK_1_DUTY_CYCLE_SET, (uint8_t) lightBank1DutyCycle);
  BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_CONTROLS_LIGHT_BANK_1_DUTY_CYCLE_TX, (uint8_t) lightBank1DutyCycle);
  
//  BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_CONTROLS_LIGHT_BANK_2_DUTY_CYCLE_SET, (uint8_t) lightBank2DutyCycle);
//  BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_CONTROLS_LIGHT_BANK_2_DUTY_CYCLE_TX, (uint8_t) lightBank2DutyCycle);
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
    
    case ACI_EVT_CONNECTED: {
      
        BLE_board.timing_change_done = false;
        break;
    }
  
    case ACI_EVT_DATA_RECEIVED: {  // One of the writeable pipes (for a Characteristic) has received data
      
      uint8_t *bytes = &aci_evt->params.data_received.rx_data.aci_data[0];
      uint8_t byteCount = aci_evt->len - 2;
      uint8_t pipe = aci_evt->params.data_received.rx_data.pipe_number;
      
      receivedDataFromPipe(bytes, byteCount, pipe);
      
      break;
    }
    
    case ACI_EVT_CMD_RSP: {  // Acknowledgement of an ACI command
      
      BLE_board._aci_cmd_pending = false;
      break;
    }
    
    case ACI_EVT_DATA_CREDIT: {  // Flow-control received the ability to make another transmission
      
      BLE_board._data_credit_pending = false;
      break;
    }
    
    case ACI_EVT_PIPE_ERROR: {  // An error associated with a pipe
      
      if (ACI_STATUS_ERROR_PEER_ATT_ERROR != aci_evt->params.pipe_error.error_code) {
          
        BLE_board._data_credit_pending = false;  // NOTE: Added while tracking down hang, don't want waitForDataCredit() to hang even though we got credit
      }
      break;
    }
    
    case ACI_EVT_PIPE_STATUS: {  //
    
        if (lib_aci_is_pipe_available(aci_state, PIPE_GREENHOUSE_MEASUREMENTS_EXTERIOR_HUMIDITY_TX) && (BLE_board.timing_change_done == false)) {
          
          lib_aci_change_timing_GAP_PPCP(); // change the timing on the link as specified in the nRFgo studio -> nRF8001 conf. -> GAP. 
                                            // Used to increase or decrease bandwidth
          BLE_board.timing_change_done = true;
        }
        
        break;
    }
    
    default: {
      
      // Clear these flags in case of unhandled switch case
      //   to avoid staying in while() loop forever
      //   NOTE: Ideally wouldn't use catch-all but not familiar enough with all possible EVT cases
      BLE_board._aci_cmd_pending = false;
      BLE_board._data_credit_pending = false;
      
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
          
          currentConfig.temperatureSetpoint = floatValue;
          persistConfiguration();
          
          BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_TEMPERATURE_SETPOINT_SET, floatValue); 
        }
      }
      break;
    }
    
    case PIPE_GREENHOUSE_USER_ADJUSTMENTS_HUMIDITY_SETPOINT_RX_ACK_AUTO: {
      
      if (byteCount == PIPE_GREENHOUSE_USER_ADJUSTMENTS_HUMIDITY_SETPOINT_RX_ACK_AUTO_MAX_SIZE) {
      
        float floatValue = *((float *)bytes);
        if ( valueWithinLimits(floatValue, HUMIDITY_SETPOINT_MIN, HUMIDITY_SETPOINT_MAX) ) {
          
          currentConfig.humiditySetpoint = floatValue;
          persistConfiguration();
          
          BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_HUMIDITY_SETPOINT_SET, floatValue); 
        }
      }
      break;
    }
    
    case PIPE_GREENHOUSE_USER_ADJUSTMENTS_HUMIDITY_NECESSITY_COEFF_RX_ACK_AUTO: {
      
      if (byteCount == PIPE_GREENHOUSE_USER_ADJUSTMENTS_HUMIDITY_NECESSITY_COEFF_RX_ACK_AUTO_MAX_SIZE) {
      
        float floatValue = *((float *)bytes);
        if ( valueWithinLimits(floatValue, NECESSITY_COEFF_MIN, NECESSITY_COEFF_MAX) ) {
          
          currentConfig.humidityNecessityCoeff = floatValue;
          persistConfiguration();
          
          BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_HUMIDITY_NECESSITY_COEFF_SET, floatValue); 
        }
      }
      break;
    }
    
    case PIPE_GREENHOUSE_USER_ADJUSTMENTS_TEMPERATURE_NECESSITY_COEFF_RX_ACK_AUTO: {
      
      if (byteCount == PIPE_GREENHOUSE_USER_ADJUSTMENTS_TEMPERATURE_NECESSITY_COEFF_RX_ACK_AUTO_MAX_SIZE) {
      
        float floatValue = *((float *)bytes);
        if ( valueWithinLimits(floatValue, NECESSITY_COEFF_MIN, NECESSITY_COEFF_MAX) ) {
          
          currentConfig.temperatureNecessityCoeff = floatValue;
          persistConfiguration();
          
          BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_TEMPERATURE_NECESSITY_COEFF_SET, floatValue); 
        }
      }
      break;
    }
    
    case PIPE_GREENHOUSE_USER_ADJUSTMENTS_VENTING_NECESSITY_THRESHOLD_RX_ACK_AUTO: {
      
      if (byteCount == PIPE_GREENHOUSE_USER_ADJUSTMENTS_VENTING_NECESSITY_THRESHOLD_RX_ACK_AUTO_MAX_SIZE) {
      
        float floatValue = *((float *)bytes);
        if ( valueWithinLimits(floatValue, NECESSITY_THRESHOLD_MIN, NECESSITY_THRESHOLD_MAX) ) {
          
          currentConfig.ventingNecessityThreshold = floatValue;
          persistConfiguration();
          
          BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_VENTING_NECESSITY_THRESHOLD_SET, floatValue); 
        }
      }
      break;
    }
    
    case PIPE_GREENHOUSE_USER_ADJUSTMENTS_VENTING_NECESSITY_OVERSHOOT_RX_ACK_AUTO: {
      
      if (byteCount == PIPE_GREENHOUSE_USER_ADJUSTMENTS_VENTING_NECESSITY_OVERSHOOT_RX_ACK_AUTO_MAX_SIZE) {
      
        float floatValue = *((float *)bytes);
        if ( valueWithinLimits(floatValue, VENTING_OVERSHOOT_MIN, VENTING_OVERSHOOT_MAX) ) {
          
          currentConfig.ventingNecessityOvershoot = floatValue;
          persistConfiguration();
          
          BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_VENTING_NECESSITY_OVERSHOOT_SET, floatValue); 
        }
      }
      break;
    }
    
    case PIPE_GREENHOUSE_USER_ADJUSTMENTS_DATETIME_RX_ACK_AUTO: {
      
      if (byteCount == PIPE_GREENHOUSE_USER_ADJUSTMENTS_DATETIME_RX_ACK_AUTO_MAX_SIZE) {
      
        unsigned long bleHostTime = *((unsigned long *)bytes);
        setTime(bleHostTime);
        adjustTime(3600);  // Shift time forward 1hr (not sure why necessary to be correct)
        
      }
      
      break;
    }
    
    case PIPE_GREENHOUSE_USER_ADJUSTMENTS_ILLUMINATION_ON_TIME_RX_ACK_AUTO: {
      
      if (byteCount == PIPE_GREENHOUSE_USER_ADJUSTMENTS_ILLUMINATION_ON_TIME_RX_ACK_AUTO_MAX_SIZE) {
      
        currentConfig.illuminationOnMinutes = *((int *)bytes);  // NOTE: Expecting that 'int' type is 2 bytes
        persistConfiguration();
        
        BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_ILLUMINATION_ON_TIME_SET, currentConfig.illuminationOnMinutes);
      }
      
      break;
    }
    
    case PIPE_GREENHOUSE_USER_ADJUSTMENTS_ILLUMINATION_OFF_TIME_RX_ACK_AUTO: {
      
      if (byteCount == PIPE_GREENHOUSE_USER_ADJUSTMENTS_ILLUMINATION_OFF_TIME_RX_ACK_AUTO_MAX_SIZE) {
      
        currentConfig.illuminationOffMinutes = *((int *)bytes);  // NOTE: Expecting that 'int' type is 2 bytes
        persistConfiguration();
        
        BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_USER_ADJUSTMENTS_ILLUMINATION_OFF_TIME_SET, currentConfig.illuminationOffMinutes);
      }
      
      break;
    }

  }  // end switch(pipe)
}

// MEASUREMENT
// ----------------------------------------------------
void enableHoneywellSensor(HoneywellSensor sensorID) {
  
   // The transistors switching the SDA (data) line between our two
   //  HIH6100 I2C devices are controlled by the outputs from an
   //  8-bit shift register.  Here we set the shift register state
   //  to enable the +5V output enabling the SDA line for one sensor
   //  or the other.
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
   
//   BLE_board.setValueForCharacteristic(PIPE_GREENHOUSE_STATE_SHIFT_REGISTER_STATE_SET, shiftRegisterState);
//   BLE_board.notifyClientOfValueForCharacteristic(PIPE_GREENHOUSE_STATE_SHIFT_REGISTER_STATE_TX, shiftRegisterState);
}


// PERSISTENT CONFIGURATION
// ----------------------------------------------------
void persistConfiguration(void) {
  
  const byte* p = (const byte*)(const void*)&currentConfig;
  unsigned int i;
  unsigned int ee = 0;
  for (i = 0; i < sizeof(UserConfig); i++) {  // Write each byte of the struct to EEPROM in series
  
    EEPROM.write(ee++, *p++);
  }
}

void restoreConfiguration(void) {
  
  byte* p = (byte*)(void*)&currentConfig;
  unsigned int i;
  unsigned int ee = 0;
  for (i = 0; i < sizeof(UserConfig); i++) {    // Write each byte of the struct to EEPROM in series
    
    byte readByte = EEPROM.read(ee++);
    
    if (i == 0 && readByte != 17) {  // Magic number doesn't match what we're expecting
      
      Serial.println(F("No stored config found.  Setting defaults"));
      
      currentConfig.magicNumber = 17;
      
      currentConfig.humiditySetpoint = 30.0f;
      currentConfig.humidityNecessityCoeff = 1.0;  // Unit-less, coefficient for balancing humidity needs with temperature needs
      
      currentConfig.temperatureSetpoint = 21.0f;
      currentConfig.temperatureNecessityCoeff = 0.75;  // Unit-less, coefficient for balancing humidity needs with temperature needs
      
      currentConfig.ventingNecessityThreshold = 8.0f;  // Unit-less, takes into account balancing humidity and temperature setpoint targets
      currentConfig.ventingNecessityOvershoot = 25.0f;  // Percent of ventingNecessityThreshold to overshoot by to reduce frequency of cycling
      currentConfig.targetVentingNecessity = UNAVAILABLE_f;  // Venting necessity value at which to stop venting
      
      currentConfig.illuminationOnMinutes = UNAVAILABLE_u;  // Stored as minutes since 12:00AM
      currentConfig.illuminationOffMinutes = UNAVAILABLE_u;  // Stored as minutes since 12:00AM
      
      break;
    }
    
    *p++ = readByte;
  }
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
    
    watchdogWokeUp = true;
    wdt_reset();  // Pet the Watchdog, stop it from forcing hardware reset
}
