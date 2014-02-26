//
//  CBPeripheral+Additions.m
//  Arduino Greenhouse
//
//  Created by Chas Conway on 2/20/14.
//  Copyright (c) 2014 Chas Conway. All rights reserved.
//

#import "CBPeripheral+Additions.h"

@implementation CBPeripheral (Additions)

- (NSArray *)allCharacteristics {
	
	return [self.services valueForKeyPath:@"@unionOfArrays.characteristics"];
}

@end
