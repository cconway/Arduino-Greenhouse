//
//  TableRowDescriptor.h
//  MusicInventory
//
//  Created by Chas Conway on 1/5/14.
//  Copyright (c) 2014 Chas Conway. All rights reserved.
//

#import <Foundation/Foundation.h>

extern NSString * const SourceObjectKey;
extern NSString * const FieldNameKey;

@interface TableRowDescriptor : NSObject

@property (nonatomic) BOOL hidden;

@property (nonatomic) NSInteger rowIdentifier;

@property (nonatomic) NSInteger rowType;

@property (strong, nonatomic) NSString *cellReuseIdentifier;

@property (strong, nonatomic) id modelObject;

@property (copy, nonatomic) void (^configureCellFromModelBlock)(UITableViewCell *cell, id modelObject);

@property (copy, nonatomic) void (^updateModelFromCellBlock)(UITableViewCell *cell, id modelObject);

@property (nonatomic) NSInteger associatedRowIdentifier;

@end
