// LICENSES: [a1cdbd]
// -----------------------------------
// The contents of this file contains the aggregate of contributions
//   covered under one or more licences. The full text of those licenses
//   can be found in the "LICENSES" file at the top level of this project
//   identified by the MD5 fingerprints listed above.

#ifndef Constants_h
#define Constants_h

typedef enum ClimateState {
  
  ClimateStateSteady,
  ClimateStateIncreasingHumidity,
  ClimateStateDecreasingHumidity,
  ClimateStateIncreasingTemperature,
  ClimateStateDecreasingTemperature
  
};

typedef struct {
 
  byte magicNumber;
  
  float humiditySetpoint;
  float humidityNecessityCoeff;  // Unit-less, coefficient for balancing humidity needs with temperature needs

  float temperatureSetpoint;
  float temperatureNecessityCoeff;  // Unit-less, coefficient for balancing humidity needs with temperature needs
  
  float ventingNecessityThreshold;  // Unit-less, takes into account balancing humidity and temperature setpoint targets
  float ventingNecessityOvershoot;  // Percent of ventingNecessityThreshold to overshoot by to reduce frequency of cycling
  float targetVentingNecessity;  // Venting necessity value at which to stop venting
  
  int illuminationOnMinutes;
  int illuminationOffMinutes;
 
} UserConfig;

#define UNAVAILABLE_f -12345.678f  // Used for indicating a value has become unavailable
#define UNAVAILABLE_u -1234  // Used for indicating a value has become unavailable

#define SAMPLING_INTERVAL 4.0  // seconds (NOTE: Must match watchdog timer!)



#define VENT_DOOR_OPEN 20  // servo angle
#define VENT_DOOR_CLOSED 135  // servo angle

#define TEMPERATURE_SETPOINT_MIN 10.0  // °C
#define TEMPERATURE_SETPOINT_MAX 35.0  // °C

#define HUMIDITY_SETPOINT_MIN 20.0  // % RH
#define HUMIDITY_SETPOINT_MAX 90.0  // % RH

#define NECESSITY_COEFF_MIN 0.0  // unitless
#define NECESSITY_COEFF_MAX 1.0  // unitless

#define NECESSITY_THRESHOLD_MIN 10.0  // unitless
#define NECESSITY_THRESHOLD_MAX 100.0  // unitless

#define VENTING_OVERSHOOT_MIN 0.0  // % of 'threshold'
#define VENTING_OVERSHOOT_MAX 50.0  // % of 'threshold'

#endif
