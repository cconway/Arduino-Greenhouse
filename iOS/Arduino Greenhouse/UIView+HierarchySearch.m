//
//  UIView+HierarchySearch.m
//  MusicInventory
//
//  Created by Chas Conway on 1/9/14.
//  Copyright (c) 2014 Chas Conway. All rights reserved.
//

#import "UIView+HierarchySearch.h"

@implementation UIView (HierarchySearch)

- (UITableViewCell *)parentTableViewCell {
	
	UIView *viewToTest = self.superview;
	while (viewToTest != nil) {
		
		if ([viewToTest isKindOfClass:[UITableViewCell class]]) {
			
			return (UITableViewCell *)viewToTest;
		
		} else viewToTest = viewToTest.superview;
	}
	
	// else
	return nil;
};

@end
