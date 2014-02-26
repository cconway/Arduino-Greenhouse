//
//  DynamicTableViewController.h
//  MusicInventory
//
//  Created by Chas Conway on 1/5/14.
//  Copyright (c) 2014 Chas Conway. All rights reserved.
//

#import <UIKit/UIKit.h>

@class TableRowDescriptor;

@interface DynamicTableViewController : UITableViewController

@property (strong, nonatomic) NSArray *sectionDescriptors;

- (void)rebuildVisibilityIndex;

- (void)rebuildVisibilityIndex:(BOOL)animated;

- (void)configureCell:(UITableViewCell *)cell forRowAtIndexPath:(NSIndexPath *)indexPath;

- (void)updateModelFromCell:(UITableViewCell *)cell forRowAtIndexPath:(NSIndexPath *)indexPath;

- (TableRowDescriptor *)rowDescriptorForCellAtIndexPath:(NSIndexPath *)indexPath;

- (NSIndexPath *)indexPathOfCellForRowDescriptor:(TableRowDescriptor *)rowDescriptor;

- (TableRowDescriptor *)rowDescriptorForRowIdentifier:(NSInteger)rowIdentifier;

- (NSArray *)rowDescriptorsWithModelObject:(id)modelObject;

@end
