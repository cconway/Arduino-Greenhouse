#ifndef BLE_h
#define BLE_h

#include "Arduino.h"
//#include <SPI.h>
//#include <lib_aci.h>
//#include <aci_setup.h>
#include "lib_nordic.h"

// Class Definition
class BLE {
  
  public:
  
    BLE(ACIPostEventHandler handlerFn);
    
    // Set LOCAL pipe value
    void setValueForCharacteristic(uint8_t pipe, float value);
    void setValueForCharacteristic(uint8_t pipe, uint8_t value);
    void setValueForCharacteristic(uint8_t pipe, int value);
    
    // Transmit value to BLE master
    boolean notifyClientOfValueForCharacteristic(uint8_t pipe, float value);
    boolean notifyClientOfValueForCharacteristic(uint8_t pipe, uint8_t value);
    boolean notifyClientOfValueForCharacteristic(uint8_t pipe, int value);    
    
  private:
    
    void waitForACIResponse();
    void waitForDataCredit();
    boolean writeBufferToPipe(uint8_t *buffer, uint8_t byteCount, uint8_t pipe);
    
    volatile byte _aci_cmd_pending;
    volatile byte _data_credit_pending;
    static ACIPostEventHandler _postEventHandlerFn;
};

#endif
