//
//  TableSectionDescriptor.h
//  MusicInventory
//
//  Created by Chas Conway on 1/5/14.
//  Copyright (c) 2014 Chas Conway. All rights reserved.
//

#import <Foundation/Foundation.h>

@class TableRowDescriptor;

@interface TableSectionDescriptor : NSObject

@property (nonatomic) BOOL hidden;

@property (strong, nonatomic) NSNumber *rowCount;
@property (copy, nonatomic) NSNumber * (^rowCountBlock)(void);

@property (strong, nonatomic) NSString *sectionName;
@property (copy, nonatomic) NSString * (^sectionNameBlock)(void);

@property (strong, nonatomic) NSArray *rowDescriptors;

//- (TableRowDescriptor *)rowDescriptorForIndex:(NSInteger)row;

@end
