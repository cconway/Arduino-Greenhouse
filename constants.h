#ifndef Constants_h
#define Constants_h

typedef enum ClimateState {
  
  ClimateStateSteady,
  ClimateStateIncreasingHumidity,
  ClimateStateDecreasingHumidity,
  ClimateStateIncreasingTemperature,
  ClimateStateDecreasingTemperature
  
};

#define VENT_DOOR_OPEN 20
#define VENT_DOOR_CLOSED 125

#endif
