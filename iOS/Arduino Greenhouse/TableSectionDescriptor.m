//
//  TableSectionDescriptor.m
//  MusicInventory
//
//  Created by Chas Conway on 1/5/14.
//  Copyright (c) 2014 Chas Conway. All rights reserved.
//

#import "TableSectionDescriptor.h"

@interface TableSectionDescriptor ()

@end



@implementation TableSectionDescriptor

- (TableRowDescriptor *)rowDescriptorForIndex:(NSInteger)row {
	
	NSAssert(self.rowDescriptors.count > row, @"'row' = %d is outside the range of 'rowDescriptors' array with length %d", (int)row, (int)self.rowDescriptors.count);

	return self.rowDescriptors[row];
}

@end
