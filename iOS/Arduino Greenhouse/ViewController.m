//
//  ViewController.m
//  Bluetooth LE Test
//
//  Created by Chas Conway on 12/10/13.
//  Copyright (c) 2013 Chas Conway. All rights reserved.
//

#import "ViewController.h"

#import "SerialCommDataTypes.h"
#import "NSData+CRC8.h"
//#import "BLEDefines.h"

#define SCAN_DURATION_SECONDS 3



@interface ViewController () {
	
	NSTimer *_scanTimer;
	NSTimer *_waitTimer;
	
	BOOL _isScanning;
	BOOL _isInteractionSetup;
}

- (IBAction)didTapConnectButton:(id)sender;

@property (strong, nonatomic) CBPeripheral *blePeripheral;


@property (weak, nonatomic) IBOutlet UIButton *connectButton;

@property (weak, nonatomic) IBOutlet UILabel *temperatureLabel;

@property (weak, nonatomic) IBOutlet UILabel *thresholdLabel;

@property (weak, nonatomic) IBOutlet UILabel *batteryLabel;

@property (weak, nonatomic) IBOutlet UIStepper *stepperControl;

@property (weak, nonatomic) IBOutlet UIActivityIndicatorView *activityIndicator;

@end




@implementation ViewController

- (void)awakeFromNib {
	
	[super awakeFromNib];
	
	[self bootstrap];
}

- (void)bootstrap {
	
	if (self.bleCentral == nil) self.bleCentral = [[CBCentralManager alloc] initWithDelegate:self queue:nil options:@{CBCentralManagerOptionRestoreIdentifierKey:CBCentralManagerIdentifier}];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.
	
	[self bootstrap];
	
//	self.stepperControl.value = [[NSUserDefaults standardUserDefaults] doubleForKey:AlarmThresholdUserDefaultKey];
//	[self didChangeThreshold:self.stepperControl];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(appBecameActive:) name:UIApplicationDidBecomeActiveNotification object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(appEnteredBackround:) name:UIApplicationDidEnterBackgroundNotification object:nil];
}

- (void)viewDidAppear:(BOOL)animated {
	
	[super viewDidAppear:animated];
	
//	[self scanForDevices];
}

- (void)dealloc {
	
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

#pragma mark - IBActions

- (IBAction)didTapConnectButton:(id)sender {

	if (self.blePeripheral.state == CBPeripheralStateConnected) {  // Already connected
		
		[self.connectButton setTitle:@"Disconnecting..." forState:UIControlStateNormal];
		
		[self.bleCentral cancelPeripheralConnection:self.blePeripheral];
	
	} else if (_isScanning) {  // Scan already in-progress
		
		[self stopScanningForDevices];
		
	} else {  // Not connected & not scanning
		
		self.connectButton.selected = YES;
		
		[self scanForDevices];
	}
}

//- (IBAction)didChangeThreshold:(id)sender {
//	
//	float thresholdTemp = ((UIStepper *)sender).value;
//	
//	[[NSUserDefaults standardUserDefaults] setFloat:thresholdTemp forKey:AlarmThresholdUserDefaultKey];
//	[[NSUserDefaults standardUserDefaults] synchronize];
//	
//	self.thresholdLabel.text = [NSString stringWithFormat:@"%1.0f", thresholdTemp];
//	
//	if (self.blePeripheral.state == CBPeripheralStateConnected) {
//		
////		NSLog(@"Writing threshold to device");
//		NSData *buffer = [NSData dataWithBytes:&thresholdTemp length:sizeof(float)];
//		[self.blePeripheral writeValue:buffer forCharacteristic:[self bleCharacteristicWithString:THRESHOLD_CHARACTERISTIC_UUID] type:CBCharacteristicWriteWithResponse];
//		
////		[bleShield writeValue:[CBUUID UUIDWithString:@RBL_SERVICE_UUID] characteristicUUID:[CBUUID UUIDWithString:@THRESHOLD_RX_UUID] p:bleShield.activePeripheral data:buffer];
//	}
//}

#pragma mark -

- (void)appBecameActive:(NSNotification *)notification {
	
	//	[self scanForDevices];
	
//	if (_isInteractionSetup) {
//		
//		// Update Temperature value immediately
//		CBCharacteristic *tempCharacteristic = [self bleCharacteristicWithString:TEMP_CHARACTERISTIC_UUID];
//		[self.blePeripheral readValueForCharacteristic:tempCharacteristic];
//		
//		// Re-start watching for value updates of Temperature
//		[self.blePeripheral setNotifyValue:YES forCharacteristic:tempCharacteristic];
//		
//		
//		CBCharacteristic *batteryVoltageCharacteristic = [self bleCharacteristicWithString:BATTERY_VOLTAGE_CHARACTERISTIC_UUID];
//		[self.blePeripheral readValueForCharacteristic:batteryVoltageCharacteristic];
//		
//		[self.blePeripheral setNotifyValue:YES forCharacteristic:batteryVoltageCharacteristic];
//	}
}

- (void)appEnteredBackround:(NSNotification *)notification {
	
//	if (_isInteractionSetup) {
//		
//		// Stop watching for value updates of Temperature while in the background
//		CBCharacteristic *tempCharacteristic = [self bleCharacteristicWithString:TEMP_CHARACTERISTIC_UUID];
//		[self.blePeripheral setNotifyValue:NO forCharacteristic:tempCharacteristic];
//		
//		CBCharacteristic *batteryVoltageCharacteristic = [self bleCharacteristicWithString:BATTERY_VOLTAGE_CHARACTERISTIC_UUID];
//		[self.blePeripheral setNotifyValue:NO forCharacteristic:batteryVoltageCharacteristic];
//	}
}

#pragma mark - BLE

- (void)scanForDevices {
	
	if (_isScanning == NO) {  // Don't continue if we're already scanning
    		
		[self.activityIndicator startAnimating];
		
		_isScanning = YES;
				
		NSArray *serviceUUIDs = @[[CBUUID UUIDWithString:USER_ADJUSTMENTS_SERVICE_UUID]];
		[self.bleCentral scanForPeripheralsWithServices:serviceUUIDs options:nil];
//		[self.bleCentral scanForPeripheralsWithServices:nil options:nil];
		
		_scanTimer = [NSTimer scheduledTimerWithTimeInterval:SCAN_DURATION_SECONDS target:self selector:@selector(blePeripheralScanTimedOut:) userInfo:nil repeats:NO];
	}
}

- (void)stopScanningForDevices {
	
	if ([_scanTimer isValid]) {
		
		[_scanTimer invalidate];
	}
	
	[self.bleCentral stopScan];
	
	self.connectButton.selected = NO;
	
	[self.activityIndicator stopAnimating];
	
	_isScanning = NO;
}

- (void)blePeripheralScanTimedOut:(NSTimer *)timer {
	
	[self stopScanningForDevices];
}

- (void)connectToPeripheral:(CBPeripheral *)peripheral {
	
	[self.connectButton setTitle:@"Connecting..." forState:UIControlStateSelected];
	
	peripheral.delegate = self;
	self.blePeripheral = peripheral;
	
	[self.bleCentral connectPeripheral:self.blePeripheral options:nil];
}

- (void)discoverCharacteristicsForService:(CBUUID *)serviceUUID {
	
	[self.blePeripheral discoverServices:@[serviceUUID]];
}

- (CBCharacteristic *)bleCharacteristicWithString:(NSString *)uuidString {
	
	CBUUID *characteristicUUID = [CBUUID UUIDWithString:uuidString];
//	CBUUID *serviceUUID = [CBUUID UUIDWithString:TEMPERATURE_SERVICE_UUID];
	
	NSArray *allCharacteristics = [self.blePeripheral.services valueForKeyPath:@"@unionOfArrays.characteristics"];
	
	if (allCharacteristics) {
		
		NSInteger characteristicIndex = [allCharacteristics indexOfObjectPassingTest:^BOOL(id obj, NSUInteger idx, BOOL *stop) {
			
			return [((CBCharacteristic *)obj).UUID isEqual:characteristicUUID];
		}];
		
		return (characteristicIndex == NSNotFound) ? nil : allCharacteristics[characteristicIndex];
	}
	
//	if (self.blePeripheral.services) {
//		
//		NSInteger serviceIndex = [self.blePeripheral.services indexOfObjectPassingTest:^BOOL(id obj, NSUInteger idx, BOOL *stop) {
//			
//			return [((CBService *)obj).UUID isEqual:serviceUUID];
//		}];
//		
//		CBService *bleService = (serviceIndex == NSNotFound) ? nil : allCharacteristics[serviceIndex];
//		
//		if (bleService.characteristics) {
//			
//			NSInteger characteristicIndex = [bleService.characteristics indexOfObjectPassingTest:^BOOL(id obj, NSUInteger idx, BOOL *stop) {
//				
//				return [((CBCharacteristic *)obj).UUID isEqual:characteristicUUID];
//			}];
//			
//			return (characteristicIndex == NSNotFound) ? nil : bleService.characteristics[characteristicIndex];
//		}
//	}

	// else
	return nil;
}

- (void)setupMeasurementInteraction {
	
	if (_isInteractionSetup) return;
	
	// else
	// -----------------------------
	
	// Get the current temperature
//	CBCharacteristic *tempCharacteristic = [self bleCharacteristicWithString:TEMP_CHARACTERISTIC_UUID];
//	[self.blePeripheral readValueForCharacteristic:tempCharacteristic];
	
	// Register for NOTIFY's
//	[self.blePeripheral setNotifyValue:YES forCharacteristic:tempCharacteristic];
	
//	CBCharacteristic *alarmCharacteristic = [self bleCharacteristicWithString:ALARM_CHARACTERISTIC_UUID];
//	[self.blePeripheral setNotifyValue:YES forCharacteristic:alarmCharacteristic];
	
	// Send our current threshold value
//	[self didChangeThreshold:self.stepperControl];
	
	for (NSString *aUUIDString in @[EXTERIOR_HUMIDITY_CHARACTERISTIC_UUID, EXTERIOR_TEMPERATURE_CHARACTERISTIC_UUID, INTERIOR_HUMIDITY_CHARACTERISTIC_UUID, INTERIOR_TEMPERATURE_CHARACTERISTIC_UUID]) {
	
		CBCharacteristic *aCharacteristic = [self bleCharacteristicWithString:aUUIDString];
		[self.blePeripheral setNotifyValue:YES forCharacteristic:aCharacteristic];
	}
	
	_isInteractionSetup = YES;
}

//- (void)setupBatteryInteraction {
//	
//	CBCharacteristic *batteryVoltageCharacteristic = [self bleCharacteristicWithString:BATTERY_VOLTAGE_CHARACTERISTIC_UUID];
//	[self.blePeripheral readValueForCharacteristic:batteryVoltageCharacteristic];
//	
//	[self.blePeripheral setNotifyValue:YES forCharacteristic:batteryVoltageCharacteristic];
//}


#pragma mark - CBCentralManagerDelegate methods

- (void)centralManagerDidUpdateState:(CBCentralManager *)central {
	
	if (central.state == CBCentralManagerStatePoweredOn) {
		
		self.connectButton.enabled = YES;
		
		[self.connectButton setTitle:@"Connect" forState:UIControlStateNormal];
		[self.connectButton setTitle:@"Scanning..." forState:UIControlStateSelected];
	
	} else {
		
		self.connectButton.enabled = NO;
	}
}

- (void)centralManager:(CBCentralManager *)central willRestoreState:(NSDictionary *)dict {
	
	NSArray *peripherals = dict[CBCentralManagerRestoredStatePeripheralsKey];
	
	if (peripherals.count > 0) {
		
		NSLog(@"Attempting connection to restored CBPeripheral");
		[self connectToPeripheral:peripherals[0]];
		
	} else NSLog(@"Restored CentralManager had no peripherals");
}

- (void)centralManager:(CBCentralManager *)central didDiscoverPeripheral:(CBPeripheral *)peripheral advertisementData:(NSDictionary *)advertisementData RSSI:(NSNumber *)RSSI {
	
	[self stopScanningForDevices];
	
	[self connectToPeripheral:peripheral];
}

- (void)centralManager:(CBCentralManager *)central didConnectPeripheral:(CBPeripheral *)peripheral {
	
	self.connectButton.selected = NO;
	
	[self.connectButton setTitle:@"Disconnect" forState:UIControlStateNormal];
	[self.connectButton setTitle:@"Disconnecting..." forState:UIControlStateSelected];
	
	[self discoverCharacteristicsForService:[CBUUID UUIDWithString:MEASUREMENTS_SERVICE_UUID]];
//	[self discoverCharacteristicsForService:[CBUUID UUIDWithString:BATTERY_SERVICE_UUID]];
}

- (void)centralManager:(CBCentralManager *)central didDisconnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error {
	
	self.connectButton.selected = NO;
	
	[self.connectButton setTitle:@"Connect" forState:UIControlStateNormal];
	[self.connectButton setTitle:@"Scanning..." forState:UIControlStateSelected];
}

- (void)centralManager:(CBCentralManager *)central didFailToConnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error {
	
	self.connectButton.selected = NO;
	
	[self.connectButton setTitle:@"Connect" forState:UIControlStateNormal];
	[self.connectButton setTitle:@"Scanning..." forState:UIControlStateSelected];
}



#pragma mark - CBPeripheralDelegate methods

- (void)peripheral:(CBPeripheral *)peripheral didDiscoverServices:(NSError *)error {
	
	for (CBService *aService in peripheral.services) {
		
		[peripheral discoverCharacteristics:nil forService:aService];
	}
}

- (void)peripheral:(CBPeripheral *)peripheral didDiscoverCharacteristicsForService:(CBService *)service error:(NSError *)error {
	
//	if ([service.UUID isEqual:[CBUUID UUIDWithString:BATTERY_SERVICE_UUID]]) {
//		
//		[self setupBatteryInteraction];
//		
//	} else {
		
		[self setupMeasurementInteraction];
//	}
}

- (void)peripheral:(CBPeripheral *)peripheral didUpdateNotificationStateForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error {
	
//	NSLog(@"NOTIFY changed: %@", characteristic);
}

- (void)peripheral:(CBPeripheral *)peripheral didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error {
	
	NSAssert(characteristic.value.length == sizeof(float), @"WARNING: Mismatch between expected and received datatype length for Temperature");
	
	float temperature;
	[characteristic.value getBytes:&temperature length:characteristic.value.length];
	
	NSLog(@"Got value: %1.2f", temperature);
	
	/*
	if (characteristic == [self bleCharacteristicWithString:TEMP_CHARACTERISTIC_UUID]) {  // Updated value for Temperature
		
		NSAssert(characteristic.value.length == sizeof(float), @"WARNING: Mismatch between expected and received datatype length for Temperature");
		
		float temperature;
		[characteristic.value getBytes:&temperature length:characteristic.value.length];
		
//		NSLog(@"Read Temperature = %1.2f°", temperature);
		
		self.temperatureLabel.text = [NSString stringWithFormat:@"%1.0f°", temperature];
	
	} else if (characteristic == [self bleCharacteristicWithString:ALARM_CHARACTERISTIC_UUID]) {  // Updated value for Temperature
		
		NSAssert(characteristic.value.length == sizeof(UInt8), @"WARNING: Mismatch between expected and received datatype length for Alarm");
		
		UInt8 alarmFiring;
		[characteristic.value getBytes:&alarmFiring length:characteristic.value.length];
		
		NSLog(@"The temperature alarm has fired!");
		
		if ([UIApplication sharedApplication].applicationState == UIApplicationStateBackground) {
			
			UILocalNotification *localNotification = [UILocalNotification new];
			localNotification.alertBody = @"Temperture Alert!";//[NSString stringWithFormat:@"Temperature is %1.0f°", incomingPacket->temperature];
			localNotification.soundName = UILocalNotificationDefaultSoundName;
			
			[[UIApplication sharedApplication] presentLocalNotificationNow:localNotification];
		}
		
	} else if (characteristic == [self bleCharacteristicWithString:BATTERY_VOLTAGE_CHARACTERISTIC_UUID]) {  // Updated value for Battery Voltage
		
		NSAssert(characteristic.value.length == sizeof(float), @"WARNING: Mismatch between expected and received datatype length for Battery Voltage");
		
		float batteryVoltage;
		[characteristic.value getBytes:&batteryVoltage length:characteristic.value.length];
		
//		NSLog(@"Battery %1.2fv", batteryVoltage);
		
		float voltageRange = 4 * (AA_BATTERY_MAX_VOLTAGE - AA_BATTERY_DEPLETED_VOLTAGE);
		
		float percentRemaining = 100.0f * MIN(1, ((batteryVoltage - (4*AA_BATTERY_DEPLETED_VOLTAGE)) / voltageRange));
		
		self.batteryLabel.text = [NSString stringWithFormat:@"%1.2fv (~%1.0f%%)", batteryVoltage, percentRemaining];
	}
	*/
}

- (void)peripheral:(CBPeripheral *)peripheral didWriteValueForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error {
	
	// NOTE: Called only for WRITE WITH RESPONSE (i.e. ACK'd)
//	NSLog(@"Got confirmation of WRITE");
	
}












/*
#pragma mark - BLEDelegate Methods

- (void)bleDidConnect {
	
	[self.connectButton setTitle:@"Disconnect" forState:UIControlStateNormal];
	
	RequestPayload payload;
	payload.commandID = GetTemperatureCommand;
	payload.lengthBytes = sizeof(RequestPayload);
	
	RequestPacket packet;
	packet.payload = payload;
	packet.crc = [NSData CRC8ChecksumFromBuffer:(char *)&payload bytesToRead:sizeof(RequestPayload)];
	
	NSLog(@"Writing %lu bytes", sizeof(RequestPacket));
	[bleShield write:[NSData dataWithBytes:&packet length:sizeof(RequestPacket)]];
	
//    [self.spinner stopAnimating];
//    [self.buttonConnect setTitle:@"Disconnect" forState:UIControlStateNormal];
    
    // Schedule to read RSSI every 1 sec.
//    rssiTimer = [NSTimer scheduledTimerWithTimeInterval:(float)1.0 target:self selector:@selector(readRSSITimer:) userInfo:nil repeats:YES];
}

- (void)bleDidDisconnect {
	
	[self.connectButton setTitle:@"Connect" forState:UIControlStateNormal];
	
	self.temperatureLabel.text = @"?";
	
//    [self.buttonConnect setTitle:@"Connect" forState:UIControlStateNormal];
    
//    [rssiTimer invalidate];
}

- (void)bleDidUpdateRSSI:(NSNumber *)rssi {
	
//    self.labelRSSI.text = rssi.stringValue;
}

- (void)bleDidReceiveData:(unsigned char *)data length:(int)length {
    
    NSData *receivedData = [NSData dataWithBytes:data length:length];
    
	float temperature;
	[receivedData getBytes:&temperature length:length];
	
	self.temperatureLabel.text = [NSString stringWithFormat:@"%1.0f°", temperature];
	
	
    int bytesToProcess = length;
    
    while (bytesToProcess > 2) {
        
        int index = (length - bytesToProcess);
        
        char header[2];
        [receivedData getBytes:&header range:NSMakeRange(index, 2)];
//		char numBytes = header[1];
        
        UInt8 payloadLength = header[0];
        UInt8 responseLength = payloadLength;  // To account for CRC byte at the end
        
        NSLog(@"Got response with length = %u", responseLength);//(UInt8)header[1]);
        
        char *payload = (char *) malloc(responseLength * sizeof(char));
        [receivedData getBytes:payload range:NSMakeRange(index, responseLength)];
        
        char sentChecksum = payload[payloadLength];
        
        bytesToProcess -= (index + responseLength);
        
        char calculatedChecksum = [NSData CRC8ChecksumFromBuffer:payload bytesToRead:payloadLength];//crc8(payload, payloadLength);
        if (sentChecksum == calculatedChecksum) {
            
            NSLog(@"Checksums match");
            
			UInt8 *ptr = (UInt8 *)payload;
			
			UInt8 lengthBytes = *ptr;
			ptr += sizeof(UInt8);
			
			UInt8 responseID = *ptr;
			ptr += sizeof(UInt8);
			
			float temperature = *ptr;
			
            //ResponsePayload *incomingPacket = (ResponsePayload *)payload;
            
			//            NSLog(@"Received %d bytes into %ld byte buffer", length, sizeof(ResponsePayload));

			//    unsigned char *bytes = (unsigned char *)&incomingPacket.temperature;
			//    for (NSInteger i=0; i < sizeof(float); i++) NSLog(@"Byte %d = %d", i, bytes[i]);

			//            char CRC = crc8((char*)&incomingPacket, ((char)sizeof(incomingPacket)));
            
            NSLog(@"ID = %d, Length = %d, Temp = %1.2f deg", responseID, lengthBytes, temperature);
            
            self.temperatureLabel.text = [NSString stringWithFormat:@"%1.0f°", temperature];
			
			if ([UIApplication sharedApplication].applicationState == UIApplicationStateBackground) {
				
				UILocalNotification *localNotification = [UILocalNotification new];
				localNotification.alertBody = [NSString stringWithFormat:@"Temperature is %1.0f°", incomingPacket->temperature];
				
				[[UIApplication sharedApplication] presentLocalNotificationNow:localNotification];
			}
 
        } else {
            
            NSLog(@"Checksum mismatch! %u vs %u. Not processing further data from this response", (UInt8)sentChecksum, (UInt8)calculatedChecksum);
            
            break;
        }
        
        free(payload);
    }
	
	
}
*/

@end
