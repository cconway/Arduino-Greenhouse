#ifndef Constants_h
#define Constants_h

typedef enum ClimateState {
  
  ClimateStateSteady,
  ClimateStateIncreasingHumidity,
  ClimateStateDecreasingHumidity,
  ClimateStateIncreasingTemperature,
  ClimateStateDecreasingTemperature
  
};

#define UNAVAILABLE_f -12345.678f  // Used for indicating a value has become unavailable
#define UNAVAILABLE_u -1234  // Used for indicating a value has become unavailable

#define VENT_DOOR_OPEN 20  // servo angle
#define VENT_DOOR_CLOSED 125  // servo angle

#define TEMPERATURE_SETPOINT_MIN 10.0  // °C
#define TEMPERATURE_SETPOINT_MAX 35.0  // °C

#define HUMIDITY_SETPOINT_MIN 20.0  // % RH
#define HUMIDITY_SETPOINT_MAX 90.0  // % RH

#define NECESSITY_COEFF_MIN 0.0  // unitless
#define NECESSITY_COEFF_MAX 1.0  // unitless

#define NECESSITY_THRESHOLD_MIN 1.0  // unitless
#define NECESSITY_THRESHOLD_MAX 15.0  // unitless

#define VENTING_OVERSHOOT_MIN 0.0  // % of 'threshold'
#define VENTING_OVERSHOOT_MAX 50.0  // % of 'threshold'

#endif
