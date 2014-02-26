//
//  NSData+CRC8.h
//  Bluetooth LE Test
//
//  Created by Chas Conway on 12/11/13.
//  Copyright (c) 2013 Chas Conway. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NSData (CRC8)

+ (char)CRC8ChecksumFromBuffer:(char *)dataBuffer bytesToRead:(char)bytesToRead;

@end
