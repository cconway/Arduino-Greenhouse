// LICENSES: [a1cdbd]
// -----------------------------------
// The contents of this file contains the aggregate of contributions
//   covered under one or more licences. The full text of those licenses
//   can be found in the "LICENSES" file at the top level of this project
//   identified by the MD5 fingerprints listed above.

#ifndef BLE_h
#define BLE_h

#include <lib_aci.h>
#include <aci_setup.h>

typedef void (*ACIPostEventHandler)(aci_state_t *aci_state, aci_evt_t *aci_evt);

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

    void ble_setup(void);
    void ble_loop(void); 
 
    byte _aci_cmd_pending;
    byte _data_credit_pending;   
    
  private:
    
    void processACIEvent(aci_state_t *aci_state, aci_evt_t *aci_evt);
    void waitForACIResponse();
    void waitForDataCredit();
    boolean writeBufferToPipe(uint8_t *buffer, uint8_t byteCount, uint8_t pipe);  
};

void setACIPostEventHandler(ACIPostEventHandler handlerFn);

#endif
