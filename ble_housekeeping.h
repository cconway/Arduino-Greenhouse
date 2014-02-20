/*Copyright (c) 2013, Nordic Semiconductor ASA
 *All rights reserved.
 *
 *Redistribution and use in source and binary forms, with or without modification,
 *are permitted provided that the following conditions are met:
 *
 *  Redistributions of source code must retain the above copyright notice, this
 *  list of conditions and the following disclaimer.
 *
 *  Redistributions in binary form must reproduce the above copyright notice, this
 *  list of conditions and the following disclaimer in the documentation and/or
 *  other materials provided with the distribution.
 *
 *  Neither the name of Nordic Semiconductor ASA nor the names of its
 *  contributors may be used to endorse or promote products derived from
 *  this software without specific prior written permission.
 *
 *THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
Description:
 
 In this template we are using the BTLE as a UART and can send and receive packets.
 The maximum size of a packet is 20 bytes.
 When a command it received a response(s) are transmitted back.
 Since the response is done using a Notification the peer must have opened it(subscribed to it) before any packet is transmitted.
 The pipe for the UART_TX becomes available once the peer opens it.
 See section 20.4.1 -> Opening a Transmit pipe 
 In the master control panel, clicking Enable Services will open all the pipes on the nRF8001.
 
 The ACI Evt Data Credit provides the radio level ack of a transmitted packet.
 */

/** @defgroup ble_uart_project_template ble_uart_project_template
 * @{
 * @ingroup projects
 * @brief Empty project that can be used as a template for new projects.
 * 
 * @details
 * This project is a firmware template for new projects. 
 * The project will run correctly in its current state.
 * It can send data on the UART TX characteristic
 * It can receive data on the UART RX characterisitc.
 * With this project you have a starting point for adding your own application functionality.
 * 
 * The following instructions describe the steps to be made on the Windows PC:
 * 
 * -# Install the Master Control Panel on your computer. Connect the Master Emulator 
 * (nRF2739) and make sure the hardware drivers are installed.
 * 
 * -# You can use the nRF UART app in the Apple iOS app store and Google Play for Android 4.3 for Samsung Galaxy S4
 * with this UART template app
 * 
 * -# You can send data from the Arduino serial monitor, maximum length of a string is 19 bytes
 * Set the line ending to "Newline" in the Serial monitor (The newline is also sent over the air
 * 
 *
 * Click on the "Serial Monitor" button on the Arduino IDE to reset the Arduino and start the application.
 * The setup() function is called first and is called only once for each reset of the Arduino.
 * The loop() function as the name implies is called in a loop.
 *
 * The setup() and loop() function are called in this way.
 * main() 
 *  {
 *   setup(); 
 *   while(1)
 *   {
 *     loop();
 *   }
 * }
 *    
 */

/**
 * Put the nRF8001 setup in the RAM of the nRF8001.
 */
#include "services.h"
/**
 * Include the services_lock.h to put the setup in the OTP memory of the nRF8001.
 * This would mean that the setup cannot be changed once put in.
 * However this removes the need to do the setup of the nRF8001 on every reset.
 */


#ifdef SERVICES_PIPE_TYPE_MAPPING_CONTENT
static services_pipe_type_mapping_t
services_pipe_type_mapping[NUMBER_OF_PIPES] = SERVICES_PIPE_TYPE_MAPPING_CONTENT;
#else
#define NUMBER_OF_PIPES 0
static services_pipe_type_mapping_t * services_pipe_type_mapping = NULL;
#endif

/* Store the setup for the nRF8001 in the flash of the AVR to save on RAM */
static hal_aci_data_t setup_msgs[NB_SETUP_MESSAGES] PROGMEM = SETUP_MESSAGES_CONTENT;

// aci_struct that will contain 
// total initial credits
// current credit
// current state of the aci (setup/standby/active/sleep)
// open remote pipe pending
// close remote pipe pending
// Current pipe available bitmap
// Current pipe closed bitmap
// Current connection interval, slave latency and link supervision timeout
// Current State of the the GATT client (Service Discovery)
// Status of the bond (R) Peer address
static struct aci_state_t aci_state;

/*
Temporary buffers for sending ACI commands
 */
static hal_aci_evt_t  aci_data;
//static hal_aci_data_t aci_cmd;

/*
Timing change state variable
 */
static bool timing_change_done          = false;

volatile byte aci_cmd_pending = 0;
volatile byte data_credit_pending = 0;

/*
Used to test the UART TX characteristic notification
 */
//static uart_over_ble_t uart_over_ble;
//static uint8_t         uart_buffer[20];
//static uint8_t         uart_buffer_len = 0;

/*
Initialize the radio_ack. This is the ack received for every transmitted packet.
 */
//static bool radio_ack_pending = false;

void ble_setup() {

  /**
   * Point ACI data structures to the the setup data that the nRFgo studio generated for the nRF8001
   */
  if (NULL != services_pipe_type_mapping)
  {
    aci_state.aci_setup_info.services_pipe_type_mapping = &services_pipe_type_mapping[0];
  }
  else
  {
    aci_state.aci_setup_info.services_pipe_type_mapping = NULL;
  }
  aci_state.aci_setup_info.number_of_pipes    = NUMBER_OF_PIPES;
  aci_state.aci_setup_info.setup_msgs         = setup_msgs;
  aci_state.aci_setup_info.num_setup_msgs     = NB_SETUP_MESSAGES;

  /*
    Tell the ACI library, the MCU to nRF8001 pin connections.
   The Active pin is optional and can be marked UNUSED
   */
  aci_state.aci_pins.board_name = REDBEARLAB_SHIELD_V1_1; //See boards.h for details
  aci_state.aci_pins.reqn_pin   = 9;
  aci_state.aci_pins.rdyn_pin   = 8;
  aci_state.aci_pins.mosi_pin   = MOSI;
  aci_state.aci_pins.miso_pin   = MISO;
  aci_state.aci_pins.sck_pin    = SCK;

  aci_state.aci_pins.spi_clock_divider          = SPI_CLOCK_DIV8;

  aci_state.aci_pins.reset_pin                  = UNUSED;
  aci_state.aci_pins.active_pin                 = UNUSED;
  aci_state.aci_pins.optional_chip_sel_pin      = UNUSED;

  aci_state.aci_pins.interface_is_interrupt	  = false;
  aci_state.aci_pins.interrupt_number		  = 1;

  //Turn debug printing on for the ACI Commands and Events to be printed on the Serial
  lib_aci_debug_print(true);

  //We reset the nRF8001 here by toggling the RESET line connected to the nRF8001
  //If the RESET line is not available we call the ACI Radio Reset to soft reset the nRF8001
  //then we initialize the data structures required to setup the nRF8001
  lib_aci_init(&aci_state);
  delay(100);
  lib_aci_radio_reset(); 

  Serial.println("Completed lib_aci init");
}

// Store a callback fn set from another library for handling received data
void (*dataReceivedCallback)(uint8_t *bytes, uint8_t byteCount, uint8_t pipe);

void setDataReceivedCallbackFn( void (*callbackFn)(uint8_t *bytes, uint8_t byteCount, uint8_t pipe) ) {
  
   dataReceivedCallback = callbackFn; 
}

void aci_loop()
{
  // We enter the if statement only when there is a ACI event available to be processed
  if (lib_aci_event_get(&aci_state, &aci_data))
  {
    aci_evt_t * aci_evt;

    aci_evt = &aci_data.evt;    
    switch(aci_evt->evt_opcode)
    {
      /**
       * As soon as you reset the nRF8001 you will get an ACI Device Started Event
       */
    case ACI_EVT_DEVICE_STARTED:
      {          
        aci_state.data_credit_total = aci_evt->params.device_started.credit_available;
        switch(aci_evt->params.device_started.device_mode) {
          
         case ACI_DEVICE_SETUP:
          /**
           * When the device is in the setup mode
           */
          Serial.println(F("BLE device in setup mode"));
          if (do_aci_setup(&aci_state) != ACI_STATUS_TRANSACTION_COMPLETE) {
            Serial.println(F("Error in ACI Setup"));
          }
          
          break;

        case ACI_DEVICE_STANDBY:
          Serial.println(F("BLE device in standby mode"));
          //Looking for an iPhone by sending radio advertisements
          //When an iPhone connects to us we will get an ACI_EVT_CONNECTED event from the nRF8001
          lib_aci_connect(180/* in seconds */, 0x0050 /* advertising interval 50ms*/);
          Serial.println(F("Beginning BLE advertisement"));
          break;
        }
      }
      break; //ACI Device Started Event

    case ACI_EVT_CMD_RSP:
      
      aci_cmd_pending = false;
      
      //If an ACI command response event comes with an error -> stop
      if (ACI_STATUS_SUCCESS != aci_evt->params.cmd_rsp.cmd_status)  // UNSUCCESSFUL
      {
        //ACI ReadDynamicData and ACI WriteDynamicData will have status codes of
        //TRANSACTION_CONTINUE and TRANSACTION_COMPLETE
        //all other ACI commands will have status code of ACI_STATUS_SCUCCESS for a successful command
        Serial.print(F("ACI command = "));
        Serial.print(aci_evt->params.cmd_rsp.cmd_opcode, HEX);
        Serial.print(F(" failed with status = "));
        Serial.println(aci_evt->params.cmd_rsp.cmd_status, HEX);
      
      } else {  // SUCCESSFUL

//        if (aci_evt->params.cmd_rsp.cmd_opcode == ACI_CMD_SET_LOCAL_DATA) {
//
//          Serial.println(F("Successfully updated LOCAL data"));
//        }
      }
      break;

    case ACI_EVT_CONNECTED:
      Serial.println(F("BLE connected to host"));
      timing_change_done              = false;
      aci_state.data_credit_available = aci_state.data_credit_total;

      /*
        Get the device version of the nRF8001 and store it in the Hardware Revision String
       */
      lib_aci_device_version();
      break;

    case ACI_EVT_PIPE_STATUS:
//      Serial.println(F("BLE pipe status changed"));
     
     // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     // TO-DO: Re-enable this with a correct PIPE
     // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
     
//      if (lib_aci_is_pipe_available(&aci_state, PIPE_CUSTOM_THERMOMETER_TEMPERATURE_TX) && (false == timing_change_done))
//      {
//        lib_aci_change_timing_GAP_PPCP(); // change the timing on the link as specified in the nRFgo studio -> nRF8001 conf. -> GAP. 
//        // Used to increase or decrease bandwidth
//        timing_change_done = true;
//      }

      break;

    case ACI_EVT_TIMING:
      Serial.println(F("BLE link connection interval changed"));
      //        lib_aci_set_local_data(&aci_state, 
      //                                PIPE_UART_OVER_BTLE_UART_LINK_TIMING_CURRENT_SET,
      //                                (uint8_t *)&(aci_evt->params.timing.conn_rf_interval), /* Byte aligned */
      //                                PIPE_UART_OVER_BTLE_UART_LINK_TIMING_CURRENT_SET_MAX_SIZE);
      break;

    case ACI_EVT_DISCONNECTED:
      Serial.println(F("BLE host disconnected or advertising session timed out"));
      
      // Automatically start advertising again after disconnecting from host or
      //   when previous advertising session expires
      lib_aci_connect(180/* in seconds */, 0x0100 /* advertising interval 100ms*/);
      Serial.println(F("Beginning BLE advertisement"));       
      break;

    case ACI_EVT_DATA_RECEIVED:
//      Serial.print(F("Pipe number "));	  
//      Serial.print(aci_evt->params.data_received.rx_data.pipe_number, DEC);
//      Serial.print(F(" received data with length = "));
//      Serial.println(aci_evt->len - 2);  //Subtract for Opcode and Pipe number
      
      // Pass data to the assigned callback fn
      (*dataReceivedCallback)(&aci_evt->params.data_received.rx_data.aci_data[0], aci_evt->len - 2, aci_evt->params.data_received.rx_data.pipe_number);

      break;

    case ACI_EVT_DATA_CREDIT:
    
//      Serial.println(F("BLE rate-limiting recieved data TX credit(s)"));

      data_credit_pending = false;
      
      aci_state.data_credit_available = aci_state.data_credit_available + aci_evt->params.data_credit.credit;
      break;

    case ACI_EVT_PIPE_ERROR:
    
      //See the appendix in the nRF8001 Product Specication for details on the error codes
      Serial.print(F("ACI Evt Pipe Error: Pipe #:"));
      Serial.print(aci_evt->params.pipe_error.pipe_number, DEC);
      Serial.print(F("  Pipe Error Code: 0x"));
      Serial.println(aci_evt->params.pipe_error.error_code, HEX);

      //Increment the credit available as the data packet was not sent.
      //The pipe error also represents the Attribute protocol Error Response sent from the peer and that should not be counted 
      //for the credit.
      if (ACI_STATUS_ERROR_PEER_ATT_ERROR != aci_evt->params.pipe_error.error_code)
      {
        aci_state.data_credit_available++;
      }
      break;


    }
  }
  else
  {
    //Serial.println(F("No ACI Events available"));
    // No event in the ACI Event queue and if there is no event in the ACI command queue the arduino can go to sleep
    // Arduino can go to sleep now
    // Wakeup from sleep from the RDYN line
  }
}




void waitForACIResponse() {
  
  aci_cmd_pending = true;
  while (aci_cmd_pending) aci_loop();
}

void waitForDataCredit() {
  
  data_credit_pending = true;
  while (data_credit_pending) aci_loop();
}

void setValueForCharacteristic(uint8_t pipe, float value) {
  
  lib_aci_set_local_data(&aci_state, pipe, (uint8_t*) &value, sizeof(float));
  
  waitForACIResponse();
}

void setValueForCharacteristic(uint8_t pipe, uint8_t value) {
  
  lib_aci_set_local_data(&aci_state, pipe, (uint8_t*) &value, sizeof(uint8_t));
  
  waitForACIResponse();
}

void setValueForCharacteristic(uint8_t pipe, int value) {
  
  setValueForCharacteristic(pipe, (uint8_t) value);
}



boolean writeBufferToPipe(uint8_t *buffer, uint8_t byteCount, uint8_t pipe) {
  
  bool success = false;
  
  if (lib_aci_is_pipe_available(&aci_state, pipe) && (aci_state.data_credit_available >= 1))
  {
    
//    Serial.print(byteCount);
//    Serial.println(F(" bytes sent to pipe"));
    
    success = lib_aci_send_data(pipe, buffer, byteCount);
//    float test = 1234;
//    uint8_t *ptr = (uint8_t *) &test;
//    success = lib_aci_send_data(PIPE_CUSTOM_THERMOMETER_TEMPERATURE_TX, ptr, sizeof(float));
    if (success) {
      
      aci_state.data_credit_available--;
      
      waitForDataCredit();
    
    } else Serial.println(F("lib_aci_send_data() failed"));
  
  } //else Serial.println(F("Pipe not available or no remaining data credits"));
  	
  return success;
}

boolean notifyClientOfValueForCharacteristic(uint8_t pipe, float value) {
 
  return writeBufferToPipe((uint8_t *) &value, sizeof(float), pipe);
}

boolean notifyClientOfValueForCharacteristic(uint8_t pipe, uint8_t value) {
 
  return writeBufferToPipe((uint8_t *) &value, sizeof(uint8_t), pipe);
}

boolean notifyClientOfValueForCharacteristic(uint8_t pipe, int value) {
  
  return notifyClientOfValueForCharacteristic(pipe, (uint8_t) value);
}
