//
//  ViewController.h
//  Bluetooth LE Test
//
//  Created by Chas Conway on 12/10/13.
//  Copyright (c) 2013 Chas Conway. All rights reserved.
//

#import <UIKit/UIKit.h>

#import <CoreBluetooth/CoreBluetooth.h>

@interface ViewController : UIViewController <CBCentralManagerDelegate, CBPeripheralDelegate>

@property (strong, nonatomic) CBCentralManager *bleCentral;

@end
