//
//  CBPeripheral+Additions.h
//  Arduino Greenhouse
//
//  Created by Chas Conway on 2/20/14.
//  Copyright (c) 2014 Chas Conway. All rights reserved.
//

#import <CoreBluetooth/CoreBluetooth.h>

@interface CBPeripheral (Additions)

- (NSArray *)allCharacteristics;

@end
