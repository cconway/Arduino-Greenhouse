#include "lib_ble.h"
#include "lib_nordic.h"

BLE::BLE(ACIPostEventHandler handlerFn) {
  
  _aci_cmd_pending = 0;
 _data_credit_pending = 0;
 
 postEventHandlerFn = handlerFn;  // postEventHandlerFn defined in lib_nordic.h
 
 aci_setup();
}

void BLE::waitForACIResponse() {
  
  _aci_cmd_pending = true;
  while (_aci_cmd_pending) aci_loop();
}

void BLE::waitForDataCredit() {
  
  _data_credit_pending = true;
  while (_data_credit_pending) aci_loop();
}

boolean BLE::writeBufferToPipe(uint8_t *buffer, uint8_t byteCount, uint8_t pipe) {
  
  boolean success = false;
  
  if (lib_aci_is_pipe_available(&aci_state, pipe) && (aci_state.data_credit_available >= 1)) {
    
//    Serial.print(byteCount);
//    Serial.println(F(" bytes sent to pipe"));
    
    success = lib_aci_send_data(pipe, buffer, byteCount);
    
    if (success) {
      
      aci_state.data_credit_available--;
      
      waitForDataCredit();
    
    } else Serial.println(F("lib_aci_send_data() failed"));
  
  } //else Serial.println(F("Pipe not available or no remaining data credits"));
  	
  return success;
}


// ------------------------------



void BLE::setValueForCharacteristic(uint8_t pipe, float value) {
  
  lib_aci_set_local_data(&aci_state, pipe, (uint8_t*) &value, sizeof(float));
  
  waitForACIResponse();
}

void BLE::setValueForCharacteristic(uint8_t pipe, uint8_t value) {
  
  lib_aci_set_local_data(&aci_state, pipe, (uint8_t*) &value, sizeof(uint8_t));
  
  waitForACIResponse();
}

void BLE::setValueForCharacteristic(uint8_t pipe, int value) {
  
  lib_aci_set_local_data(&aci_state, pipe, (uint8_t*) &value, sizeof(int));
  
  waitForACIResponse();
}


boolean BLE::notifyClientOfValueForCharacteristic(uint8_t pipe, float value) {
 
  return writeBufferToPipe((uint8_t *) &value, sizeof(float), pipe);
}

boolean BLE::notifyClientOfValueForCharacteristic(uint8_t pipe, uint8_t value) {
 
  return writeBufferToPipe((uint8_t *) &value, sizeof(uint8_t), pipe);
}

boolean BLE::notifyClientOfValueForCharacteristic(uint8_t pipe, int value) {
  
  return writeBufferToPipe((uint8_t *) &value, sizeof(int), pipe);
}
