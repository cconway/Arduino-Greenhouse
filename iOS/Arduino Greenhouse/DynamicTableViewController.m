//
//  DynamicTableViewController.m
//  MusicInventory
//
//  Created by Chas Conway on 1/5/14.
//  Copyright (c) 2014 Chas Conway. All rights reserved.
//

#import "DynamicTableViewController.h"

#import "TableSectionDescriptor.h"
#import "TableRowDescriptor.h"
#import "DynamicHeightCell.h"
#import "NSIndexPath+SearchAdditions.h"

@interface DynamicTableViewController () {
	
	NSMutableArray *_visibleRowDescriptorsBySection;
}

@property (strong, nonatomic) NSArray *visibleSections;
@property (strong, nonatomic) NSMutableDictionary *cellHeightDictionary;

@end



@implementation DynamicTableViewController

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self) {
        
		self.cellHeightDictionary = [NSMutableDictionary new];
    }
    return self;
}

#pragma mark -

- (void)configureCell:(UITableViewCell *)cell forRowAtIndexPath:(NSIndexPath *)indexPath {
	
	TableRowDescriptor *rowDescriptor = [self rowDescriptorForCellAtIndexPath:indexPath];
	
	if (rowDescriptor.configureCellFromModelBlock) rowDescriptor.configureCellFromModelBlock(cell, rowDescriptor.modelObject);
}

- (void)updateModelFromCell:(UITableViewCell *)cell forRowAtIndexPath:(NSIndexPath *)indexPath {
		
	TableRowDescriptor *rowDescriptor = [self rowDescriptorForCellAtIndexPath:indexPath];
	
	if (rowDescriptor.updateModelFromCellBlock) rowDescriptor.updateModelFromCellBlock(cell, rowDescriptor.modelObject);
	
}

#pragma mark -

- (void)rebuildVisibilityIndex:(BOOL)animated {
	
	if (animated) {
		
		[self.tableView beginUpdates];
		
		NSArray *tableSectionsBefore = [NSArray arrayWithArray:_visibleRowDescriptorsBySection];
		
		[self rebuildVisibilityIndex];
		
		NSMutableArray *tableSectionsAfter = _visibleRowDescriptorsBySection;
		
		NSMutableArray *indexPathsToInsert = [NSMutableArray new];
		NSMutableArray *indexPathsToDelete = [NSMutableArray new];
		
		for (NSInteger section = 0; section < self.sectionDescriptors.count; section++) {  // For each section
			
			NSArray *rowDescriptorsBefore = tableSectionsBefore[section];
			NSArray *rowDescriptorsAfter = tableSectionsAfter[section];
			
			NSMutableArray *transformArray = [NSMutableArray arrayWithArray:rowDescriptorsBefore];
			
			// Look for rows removed
			[rowDescriptorsBefore enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
				
				TableRowDescriptor *rowDescriptorBefore = obj;
				NSUInteger indexBefore = idx;
				
				NSInteger indexAfter = [rowDescriptorsAfter indexOfObject:rowDescriptorBefore];
				
				if (indexAfter == NSNotFound) {
					
					[indexPathsToDelete addObject:[NSIndexPath indexPathForRow:indexBefore inSection:section]];
					
					[transformArray removeObjectAtIndex:indexBefore];
				}
			}];
			
			// Look for rows added
			[rowDescriptorsAfter enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
				
				TableRowDescriptor *rowDescriptorAfter = obj;
				NSUInteger indexAfter = idx;
				
				NSInteger indexBefore = [transformArray indexOfObject:rowDescriptorAfter];
				
				if (indexBefore == NSNotFound) {
					
					[indexPathsToInsert addObject:[NSIndexPath indexPathForRow:indexAfter inSection:section]];
				}
			}];
		}
		
//		NSLog(@"Index paths to add: %@", indexPathsToInsert);
//		NSLog(@"Index paths to delete: %@", indexPathsToDelete);
		
		[self.tableView deleteRowsAtIndexPaths:indexPathsToDelete withRowAnimation:UITableViewRowAnimationFade];
		[self.tableView insertRowsAtIndexPaths:indexPathsToInsert withRowAnimation:UITableViewRowAnimationFade];
		
		[self.tableView endUpdates];
	}
}

- (void)rebuildVisibilityIndex {
	
	_visibleRowDescriptorsBySection = [NSMutableArray new];
	
	for (TableSectionDescriptor *aSection in self.sectionDescriptors) {
		
		NSIndexSet *visibleRowSet = [aSection.rowDescriptors indexesOfObjectsPassingTest:^BOOL(id obj, NSUInteger idx, BOOL *stop) {
			
			TableRowDescriptor *rowDescriptor = obj;
			return rowDescriptor.hidden == NO;
		}];
		
		/*
		// Convert from NSMutableIndexSet to NSIndexPath to facilitate -indexAtPosition: lookups
		NSUInteger *indexes = malloc(sizeof(NSUInteger) * visibleRowSet.count);
		[visibleRowSet getIndexes:indexes maxCount:visibleRowSet.count inIndexRange:nil];
		NSIndexPath *visibilityIndexPath = [NSIndexPath indexPathWithIndexes:indexes length:visibleRowSet.count];
		[_visibleRowsIndexPathBySection addObject:visibilityIndexPath];
		*/
		
		NSArray *visibleRowDescriptors = [aSection.rowDescriptors objectsAtIndexes:visibleRowSet];
		[_visibleRowDescriptorsBySection addObject:visibleRowDescriptors];
	}
}

- (TableRowDescriptor *)rowDescriptorForCellAtIndexPath:(NSIndexPath *)indexPath {
	
//	NSArray *rows = self.visibleSections[indexPath.section];
//	TableRowDescriptor *rowDescriptor = rows[indexPath.row];
		
	NSArray *visibleRowDescriptors = _visibleRowDescriptorsBySection[indexPath.section];
	TableRowDescriptor *rowDescriptor = [visibleRowDescriptors objectAtIndex:indexPath.row];
	
	return rowDescriptor;
}

- (NSIndexPath *)indexPathOfCellForRowDescriptor:(TableRowDescriptor *)rowDescriptor {
	
	__block NSIndexPath *indexPath = nil;
	
	[_visibleRowDescriptorsBySection enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
		
		NSArray *rowDescriptors = obj;
		NSInteger rowIndex = [rowDescriptors indexOfObject:rowDescriptor];
		if (rowIndex != NSNotFound) {
			
			*stop = YES;
			indexPath = [NSIndexPath indexPathForRow:rowIndex inSection:idx];
		}
	}];
	
	return indexPath;
}

- (TableRowDescriptor *)rowDescriptorForRowIdentifier:(NSInteger)rowIdentifier {
	
	for (TableSectionDescriptor *aSectionDescriptor in self.sectionDescriptors) {
		
		for (TableRowDescriptor *aRowDescriptor in aSectionDescriptor.rowDescriptors) {
			
			if (aRowDescriptor.rowIdentifier == rowIdentifier) return aRowDescriptor;
		}
	}
	
	return nil;
}

- (NSArray *)rowDescriptorsWithModelObject:(id)modelObject {
	
	for (NSArray *rowDescriptors in _visibleRowDescriptorsBySection) {
		
		NSArray *results = [rowDescriptors filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"modelObject = %@", modelObject]];
		if (results.count > 0) return results;
	}
	
	// else
	return nil;
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{

    // Return the number of sections.
    return self.sectionDescriptors.count;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{

    // Return the number of rows in the section.
//    NSArray *sectionIndices = self.visibleSections[section];
//	  return sectionIndices.count;
	
	NSArray *visibleRowDescriptors = _visibleRowDescriptorsBySection[section];
	return visibleRowDescriptors.count;
}


- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
	TableRowDescriptor *rowDescriptor = [self rowDescriptorForCellAtIndexPath:indexPath];
	
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:rowDescriptor.cellReuseIdentifier forIndexPath:indexPath];
    
    // Configure the cell...
	[self configureCell:cell forRowAtIndexPath:indexPath];
    
    return cell;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
	
	TableRowDescriptor *rowDescriptor = [self rowDescriptorForCellAtIndexPath:indexPath];
	
	NSValue *cachedValue = self.cellHeightDictionary[rowDescriptor.cellReuseIdentifier];
	if (cachedValue) {
		
		//NSLog(@"Returning cached cell height = %f", [cachedValue CGSizeValue].height);
		return [cachedValue CGSizeValue].height;
	
	} else {
		
		UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:rowDescriptor.cellReuseIdentifier];
		
		if ([cell conformsToProtocol:@protocol(DynamicHeightCell)]) {
		
			cell.frame = CGRectMake(0, 0, CGRectGetWidth(tableView.frame), CGRectGetHeight(cell.frame));
			
			[cell setNeedsLayout];
			[cell layoutIfNeeded];
			
			CGSize contentSize = [cell.contentView systemLayoutSizeFittingSize:UILayoutFittingCompressedSize];
			
			[self.cellHeightDictionary setValue:[NSValue valueWithCGSize:contentSize] forKey:rowDescriptor.cellReuseIdentifier];
			
			//NSLog(@"Calculated dynamic cell height = %f", contentSize.height);
			return contentSize.height;
		
		} else {
			
			[self.cellHeightDictionary setValue:[NSValue valueWithCGSize:cell.frame.size] forKey:rowDescriptor.cellReuseIdentifier];
			
			//NSLog(@"Returning cell intrinsic layout height = %f", cell.frame.size.height);
			return cell.frame.size.height;
		}
		
	}
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
	
	TableSectionDescriptor *sectionDescriptor = self.sectionDescriptors[section];
	return sectionDescriptor.sectionName;
}

/*
// Override to support conditional editing of the table view.
- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Return NO if you do not want the specified item to be editable.
    return YES;
}
*/

/*
// Override to support editing the table view.
- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        // Delete the row from the data source
        [tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
    } else if (editingStyle == UITableViewCellEditingStyleInsert) {
        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
    }   
}
*/

/*
// Override to support rearranging the table view.
- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath
{
}
*/

/*
// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Return NO if you do not want the item to be re-orderable.
    return YES;
}
*/

#pragma mark -

/* TODO: Figure out how to copy values from cell before datasource no longer contains the related rowDescriptor
- (void)tableView:(UITableView *)tableView didEndDisplayingCell:(UITableViewCell *)cell forRowAtIndexPath:(NSIndexPath *)indexPath {
	
	[self updateModelFromCell:cell forRowAtIndexPath:indexPath];
}
*/

@end
