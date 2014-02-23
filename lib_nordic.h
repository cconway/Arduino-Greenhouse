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
 

/** @defgroup ble_uart_project_template ble_uart_project_template
@{
@ingroup projects
@brief Empty project that can be used as a template for new projects.

@details
This project is a firmware template for new projects. 
The project will run correctly in its current state.
It can send data on the UART TX characteristic
It can receive data on the UART RX characterisitc.
With this project you have a starting point for adding your own application functionality.

The following instructions describe the steps to be made on the Windows PC:

 -# Install the Master Control Panel on your computer. Connect the Master Emulator 
    (nRF2739) and make sure the hardware drivers are installed.

-# You can use the nRF UART app in the Apple iOS app store and Google Play for Android 4.3 for Samsung Galaxy S4
   with this UART template app

-# You can send data from the Arduino serial monitor, maximum length of a string is 19 bytes
   Set the line ending to "Newline" in the Serial monitor (The newline is also sent over the air

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
 
#ifndef NORDIC
#define NORDIC

#include <SPI.h>
//#include <avr/pgmspace.h>
#include <lib_aci.h>
#include <aci_setup.h>

/**
Put the nRF8001 setup in the RAM of the nRF8001.
*/
#include "services.h"
/**
Include the services_lock.h to put the setup in the OTP memory of the nRF8001.
This would mean that the setup cannot be changed once put in.
However this removes the need to do the setup of the nRF8001 on every reset.
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
static boolean timing_change_done          = false;

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
void aci_setup(void);

void aci_loop(void);

typedef void (*ACIPostEventHandler)(aci_state_t *aci_state, aci_evt_t *aci_evt);
static ACIPostEventHandler postEventHandlerFn;

#endif
