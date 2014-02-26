//
//  SerialCommDataTypes.h
//  Bluetooth LE Test
//
//  Created by Chas Conway on 12/10/13.
//  Copyright (c) 2013 Chas Conway. All rights reserved.
//

#ifndef Bluetooth_LE_Test_SerialCommDataTypes_h
#define Bluetooth_LE_Test_SerialCommDataTypes_h

typedef struct __attribute__((packed)) {
    
	char lengthBytes;
    char responseID;
    float temperature;
    
} ResponsePayload;

typedef struct __attribute__((packed)) {
    
    ResponsePayload payload;
    char crc;
    
} ResponsePacket;

typedef struct /*__attribute__((packed))*/ {
    
	char lengthBytes;
    char commandID;
    
} RequestPayload;

typedef struct /*__attribute__((packed))*/ {
    
    RequestPayload payload;
    char crc;
    
} RequestPacket;


typedef enum {
	
	GetStatusCommand,
	GetTemperatureCommand,
	SetSendUpdatesCommand
	
} Command;

#endif
