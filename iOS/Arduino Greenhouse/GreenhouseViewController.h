//
//  GreenhouseViewController.h
//  Arduino Greenhouse
//
//  Created by Chas Conway on 2/19/14.
//  Copyright (c) 2014 Chas Conway. All rights reserved.
//

#import "DynamicTableViewController.h"

#import <CoreBluetooth/CoreBluetooth.h>
#import "TableSectionDescriptor.h"
#import "TableRowDescriptor.h"

#define CELL_TITLE_LABEL_TAG 1
#define CELL_VALUE_LABEL_TAG 2
#define CELL_SWITCH_TAG 3
#define CELL_SLIDER_TAG 4
#define CELL_TEXT_FIELD_TAG 5
#define CELL_ALT_VALUE_LABEL 6

@interface GreenhouseViewController : DynamicTableViewController <CBCentralManagerDelegate, CBPeripheralDelegate>

- (void)bootstrapModel;

@property (strong, nonatomic) CBCentralManager *bleCentral;

@end
