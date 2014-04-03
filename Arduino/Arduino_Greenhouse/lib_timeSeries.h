// LICENSES: [a1cdbd]
// -----------------------------------
// The contents of this file contains the aggregate of contributions
//   covered under one or more licences. The full text of those licenses
//   can be found in the "LICENSES" file at the top level of this project
//   identified by the MD5 fingerprints listed above.

#ifndef TimeSeries_h
#define TimeSeries_h

#include <Time.h>
#include "constants.h"

#define CIRC_BUFFER_DEPTH 6


// Class Definition
class TimeSeries {
  
  public:
    TimeSeries(void);
    
    void addValue(float newValue);
    void clearAll();
    
    float averageValue();
    float averageSlope();
    
    uint8_t measurementCount;
    
  private:
    float _average(float values[], uint8_t startingIndex, uint8_t length);
  
    float _measurements[CIRC_BUFFER_DEPTH];
    uint16_t _timeDeltas[CIRC_BUFFER_DEPTH];
    uint8_t _bufferIndex;
    unsigned long _lastMeasurementTime;
};

#endif
