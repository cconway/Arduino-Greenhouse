//
//  NSIndexPath+SearchAdditions.h
//  MusicInventory
//
//  Created by Chas Conway on 1/9/14.
//  Copyright (c) 2014 Chas Conway. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NSIndexPath (SearchAdditions)

- (NSInteger)positionOfIndex:(NSUInteger)index startingFromPosition:(NSUInteger)position;

@end
