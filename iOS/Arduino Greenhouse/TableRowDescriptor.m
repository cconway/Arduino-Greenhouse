//
//  TableRowDescriptor.m
//  MusicInventory
//
//  Created by Chas Conway on 1/5/14.
//  Copyright (c) 2014 Chas Conway. All rights reserved.
//

#import "TableRowDescriptor.h"

NSString * const SourceObjectKey = @"com.ChasConway.DynamicTableViewController.SourceObjectKey";
NSString * const FieldNameKey = @"com.ChasConway.DynamicTableViewController.FieldNameKey";

@implementation TableRowDescriptor

- (id)init
{
    self = [super init];
    if (self) {
        
		self.rowIdentifier = -1;  // Make sure default value doesn't collide with ENUM values
		self.rowType = -1;  // Make sure default value doesn't collide with ENUM values
    }
    return self;
}

@end
