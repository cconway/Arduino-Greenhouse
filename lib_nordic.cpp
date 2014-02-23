/* Copyright (c) 2014, Nordic Semiconductor ASA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
 
#include "lib_nordic.h"
#include "services.h"

ACIPostEventHandler postEventHandlerFn;

void aci_setup(void)
{ 
  
	// Point ACI data structures to the the setup data that the nRFgo studio generated for the nRF8001
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

	// Tell the ACI library, the MCU to nRF8001 pin connections.
	// The Active pin is optional and can be marked UNUSED
	aci_state.aci_pins.board_name = REDBEARLAB_SHIELD_V1_1; //See board.h for details REDBEARLAB_SHIELD_V1_1 or BOARD_DEFAULT
	aci_state.aci_pins.reqn_pin   = 9; //SS for Nordic board, 9 for REDBEARLAB_SHIELD_V1_1
	aci_state.aci_pins.rdyn_pin   = 8; //3 for Nordic board, 8 for REDBEARLAB_SHIELD_V1_1
	aci_state.aci_pins.mosi_pin   = MOSI;
	aci_state.aci_pins.miso_pin   = MISO;
	aci_state.aci_pins.sck_pin    = SCK;

	aci_state.aci_pins.spi_clock_divider          = SPI_CLOCK_DIV8;

	aci_state.aci_pins.reset_pin             = UNUSED; //4 for Nordic board, UNUSED for REDBEARLAB_SHIELD_V1_1
	aci_state.aci_pins.active_pin            = UNUSED;
	aci_state.aci_pins.optional_chip_sel_pin = UNUSED;

	aci_state.aci_pins.interface_is_interrupt	  = false;
	aci_state.aci_pins.interrupt_number			  = 1;

	//We reset the nRF8001 here by toggling the RESET line connected to the nRF8001
	//If the RESET line is not available we call the ACI Radio Reset to soft reset the nRF8001
	//then we initialize the data structures required to setup the nRF8001
	lib_aci_init(&aci_state);

    //Turn debug printing on for the ACI Commands and Events to be printed on the Serial
	lib_aci_debug_print(false);
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
        As soon as you reset the nRF8001 you will get an ACI Device Started Event
        */
        case ACI_EVT_DEVICE_STARTED:
        { 
          aci_state.data_credit_total = aci_evt->params.device_started.credit_available;
          switch(aci_evt->params.device_started.device_mode)
          {
            case ACI_DEVICE_SETUP:
            /**
            When the device is in the setup mode
            */
            Serial.println(F("Evt Device Started: Setup"));
            if (ACI_STATUS_TRANSACTION_COMPLETE != do_aci_setup(&aci_state))
            {
              Serial.println(F("Error in ACI Setup"));
            }
            break;
            
            case ACI_DEVICE_STANDBY:
              Serial.println(F("Evt Device Started: Standby"));
              //Looking for an iPhone by sending radio advertisements
              //When an iPhone connects to us we will get an ACI_EVT_CONNECTED event from the nRF8001
              if (aci_evt->params.device_started.hw_error)
              {
                delay(20); //Handle the HW error event correctly.
              }
              else
              {
              lib_aci_connect(0/* in seconds : 0 means forever */, 0x0050 /* advertising interval 50ms*/);
              Serial.println(F("Advertising started : Tap Connect on the nRF UART app"));
              }
              
              break;
          }
        }
        break; //ACI Device Started Event
        
      case ACI_EVT_CMD_RSP:
        //If an ACI command response event comes with an error -> stop
        if (ACI_STATUS_SUCCESS != aci_evt->params.cmd_rsp.cmd_status)
        {
          //ACI ReadDynamicData and ACI WriteDynamicData will have status codes of
          //TRANSACTION_CONTINUE and TRANSACTION_COMPLETE
          //all other ACI commands will have status code of ACI_STATUS_SCUCCESS for a successful command
          Serial.print(F("ACI Command "));
          Serial.println(aci_evt->params.cmd_rsp.cmd_opcode, HEX);
          Serial.print(F("Evt Cmd respone: Status "));
          Serial.println(aci_evt->params.cmd_rsp.cmd_status, HEX);
        }
        
//        if (ACI_CMD_GET_DEVICE_VERSION == aci_evt->params.cmd_rsp.cmd_opcode)
//        {
//          //Store the version and configuration information of the nRF8001 in the Hardware Revision String Characteristic
//          lib_aci_set_local_data(&aci_state, PIPE_DEVICE_INFORMATION_HARDWARE_REVISION_STRING_SET, 
//            (uint8_t *)&(aci_evt->params.cmd_rsp.params.get_device_version), sizeof(aci_evt_cmd_rsp_params_get_device_version_t));
//        }
//
//        if (ACI_CMD_SET_LOCAL_DATA == aci_evt->params.cmd_rsp.cmd_opcode)
//        {
//           char hello[]="Hello World, works";
//           uart_tx((uint8_t *)&hello[0], strlen(hello));
//           Serial.print(F("Sending :")); 
//           Serial.println(hello);
//        }

        break;
        
      case ACI_EVT_CONNECTED:
        Serial.println(F("Evt Connected"));
        timing_change_done              = false;
        aci_state.data_credit_available = aci_state.data_credit_total;
        
        /*
        Get the device version of the nRF8001 and store it in the Hardware Revision String
        */
//        lib_aci_device_version();
        break;
        
      case ACI_EVT_PIPE_STATUS:
        Serial.println(F("Evt Pipe Status"));
//        if (lib_aci_is_pipe_available(&aci_state, PIPE_UART_OVER_BTLE_UART_TX_TX) && (false == timing_change_done))
//        {
//          lib_aci_change_timing_GAP_PPCP(); // change the timing on the link as specified in the nRFgo studio -> nRF8001 conf. -> GAP. 
//                                            // Used to increase or decrease bandwidth
//          timing_change_done = true;
//        }
        break;
        
      case ACI_EVT_TIMING:
        Serial.println(F("Evt link connection interval changed"));
//        lib_aci_set_local_data(&aci_state, 
//                                PIPE_UART_OVER_BTLE_UART_LINK_TIMING_CURRENT_SET,
//                                (uint8_t *)&(aci_evt->params.timing.conn_rf_interval), /* Byte aligned */
//                                PIPE_UART_OVER_BTLE_UART_LINK_TIMING_CURRENT_SET_MAX_SIZE);
        break;
        
      case ACI_EVT_DISCONNECTED:
        Serial.println(F("Evt Disconnected/Advertising timed out"));
        lib_aci_connect(0/* in seconds  : 0 means forever */, 0x0050 /* advertising interval 50ms*/);
        Serial.println(F("Advertising started. Tap Connect on the nRF UART app"));        
        break;
        
      case ACI_EVT_DATA_RECEIVED:
		
//                Serial.print(F("Pipe Number: "));	  
//		Serial.println(aci_evt->params.data_received.rx_data.pipe_number, DEC);
//
//		if (PIPE_UART_OVER_BTLE_UART_RX_RX == aci_evt->params.data_received.rx_data.pipe_number)					
//			{
//
//			  Serial.print(F(" Data(Hex) : "));
//			  for(int i=0; i<aci_evt->len - 2; i++)
//			  {
//				Serial.print((char)aci_evt->params.data_received.rx_data.aci_data[i]);
//				uart_buffer[i] = aci_evt->params.data_received.rx_data.aci_data[i];
//				Serial.print(F(" "));
//			  }
//			  uart_buffer_len = aci_evt->len - 2;			
//			  Serial.println(F(""));
//			  if (lib_aci_is_pipe_available(&aci_state, PIPE_UART_OVER_BTLE_UART_TX_TX))
//			  {
//
//					/*Do this to test the loopback otherwise comment it out
//					*/
//					/*
//					if (!uart_tx(&uart_buffer[0], aci_evt->len - 2))
//					{
//						Serial.println(F("UART loopback failed"));
//					}
//					else
//					{
//						Serial.println(F("UART loopback OK"));
//					}
//					*/
//			  }
//		}
//        if (PIPE_UART_OVER_BTLE_UART_CONTROL_POINT_RX == aci_evt->params.data_received.rx_data.pipe_number)
//        {
//            uart_process_control_point_rx(&aci_evt->params.data_received.rx_data.aci_data[0], aci_evt->len - 2); //Subtract for Opcode and Pipe number
//        }
        break;
   
      case ACI_EVT_DATA_CREDIT:
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
      
      case ACI_EVT_HW_ERROR:
        Serial.print(F("HW error: "));
        Serial.println(aci_evt->params.hw_error.line_num, DEC);
        
        for(uint8_t counter = 0; counter <= (aci_evt->len - 3); counter++)
        {
        Serial.write(aci_evt->params.hw_error.file_name[counter]); //uint8_t file_name[20];
        }
        Serial.println();
        lib_aci_connect(0/* in seconds, 0 means forever */, 0x0050 /* advertising interval 50ms*/);
        Serial.println(F("Advertising started. Tap Connect on the nRF UART app"));
        break;
           
    }
    
    // Now run our post-event handler
//    postEventHandlerFn(&aci_state, aci_evt);
    
  }
  else
  {
    //Serial.println(F("No ACI Events available"));
    // No event in the ACI Event queue and if there is no event in the ACI command queue the arduino can go to sleep
    // Arduino can go to sleep now
    // Wakeup from sleep from the RDYN line
  }
}

void setACIPostEventHandler(ACIPostEventHandler handlerFn) {
  
  postEventHandlerFn = handlerFn;
}

