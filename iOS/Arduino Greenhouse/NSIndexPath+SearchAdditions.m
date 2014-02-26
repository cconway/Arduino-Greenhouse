//
//  NSIndexPath+SearchAdditions.m
//  MusicInventory
//
//  Created by Chas Conway on 1/9/14.
//  Copyright (c) 2014 Chas Conway. All rights reserved.
//

#import "NSIndexPath+SearchAdditions.h"

@implementation NSIndexPath (SearchAdditions)

- (NSInteger)positionOfIndex:(NSUInteger)targetIndex startingFromPosition:(NSUInteger)startingPosition {
	
	NSAssert(self.length > startingPosition, @"starting position must be within length of NSIndexPath");
	
	for (NSUInteger position = startingPosition; position < self.length; position++) {
		
		if ([self indexAtPosition:position] == targetIndex) return position;
	}
	
	// else
	return NSNotFound;
}

@end
