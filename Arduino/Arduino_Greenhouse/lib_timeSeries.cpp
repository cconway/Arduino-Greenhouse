// LICENSES: [a1cdbd]
// -----------------------------------
// The contents of this file contains the aggregate of contributions
//   covered under one or more licences. The full text of those licenses
//   can be found in the "LICENSES" file at the top level of this project
//   identified by the MD5 fingerprints listed above.

#include "Arduino.h"
#include "lib_timeSeries.h"

TimeSeries::TimeSeries() {
  
  clearAll();
}

void TimeSeries::clearAll() {
  
  _bufferIndex = 0;
  measurementCount = 0;
}

void TimeSeries::addValue(float newValue) {
  
  // Record the measurement
  _measurements[_bufferIndex] = newValue;
  
  if (measurementCount == 0) {
    
    _timeDeltas[_bufferIndex] = 0;
    
  } else {
    
    // TODO: Handle case where millis wraps around
    unsigned long currentTime = millis();
    _timeDeltas[_bufferIndex] = currentTime - _lastMeasurementTime;
    _lastMeasurementTime = currentTime;
  }
  
  // Move the index forward, circularly
  _bufferIndex = ++_bufferIndex % CIRC_BUFFER_DEPTH;
  
  if (measurementCount < CIRC_BUFFER_DEPTH) measurementCount++;  // Only increment up to CIRC_BUFFER_DEPTH
}

float TimeSeries::averageValue() {
  
  if (measurementCount == 0) return UNAVAILABLE_f;
  else return _average(_measurements, _bufferIndex, measurementCount);
}

float TimeSeries::averageSlope() {
  
  if (measurementCount < 2) return UNAVAILABLE_f;
  else {
    
    float dVdTs[CIRC_BUFFER_DEPTH - 1];
    
    // Calculate the slope between each timepoints
    for (uint8_t i=0; i < (measurementCount - 1); i++) {
      
      // Calculate indices that go backwards through the circular buffer
      uint8_t startingIndex = (_bufferIndex + (CIRC_BUFFER_DEPTH - measurementCount)) % CIRC_BUFFER_DEPTH;
      uint8_t indexA = (startingIndex + i) % CIRC_BUFFER_DEPTH;
      uint8_t indexB = (indexA + 1) % CIRC_BUFFER_DEPTH;
      
      dVdTs[i] = (_measurements[indexB] - _measurements[indexA]) / ( (float)_timeDeltas[indexB] / 1e3f);  // Note: Converting ms to s
    }
    
    return _average(dVdTs, 0, measurementCount - 1);
  }
}

float TimeSeries::_average(float values[], uint8_t startingIndex, uint8_t length) {
  
  float total = 0;
  for (uint8_t i = 0; i < length; i++) {
    
    uint8_t index = (i + startingIndex) % length;
    
    total += values[index];
  }
  
  return total / length;
}
