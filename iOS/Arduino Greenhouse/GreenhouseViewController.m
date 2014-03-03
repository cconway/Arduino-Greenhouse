//
//  GreenhouseViewController.m
//  Arduino Greenhouse
//
//  Created by Chas Conway on 2/19/14.
//  Copyright (c) 2014 Chas Conway. All rights reserved.
//

#import "GreenhouseViewController.h"

#import "UIView+HierarchySearch.h"
#import "CBPeripheral+Additions.h"

#define SCAN_DURATION_SECONDS 3

typedef NS_ENUM(UInt8, ClimateState) {
	
	ClimateStateSteady,
	ClimateStateIncreasingHumidity,
	ClimateStateDecreasingHumidity,
	ClimateStateIncreasingTemperature,
	ClimateStateDecreasingTemperature
	
};


@interface GreenhouseViewController () {
	
	NSNumberFormatter *_humidityFormatter;
	NSNumberFormatter *_temperatureFormatter;
	NSNumberFormatter *_floatFormatter;
	
	NSTimer *_scanTimer;
	NSTimer *_waitTimer;
	
	BOOL _isScanning;
	BOOL _isInteractionSetup;
	BOOL _previousPeripheralAttempt;
}

@property (strong, nonatomic) CBPeripheral *blePeripheral;

@property (weak, nonatomic) IBOutlet UIButton *connectButton;

@property (strong, nonatomic) NSMutableDictionary *greenhouseValues;

@end

@implementation GreenhouseViewController


- (void)awakeFromNib {
	
	[super awakeFromNib];
	
	self.greenhouseValues = [NSMutableDictionary new];
	
	_humidityFormatter = [NSNumberFormatter new];
	[_humidityFormatter setNumberStyle:NSNumberFormatterPercentStyle];
	[_humidityFormatter setMaximumFractionDigits:2];
	[_humidityFormatter setMultiplier:@1];
	[_humidityFormatter setNilSymbol:@"---"];
	
	_temperatureFormatter = [NSNumberFormatter new];
	[_temperatureFormatter setNumberStyle:NSNumberFormatterDecimalStyle];
	[_temperatureFormatter setMaximumFractionDigits:2];
	[_temperatureFormatter setPositiveSuffix:@" °F"];
	[_temperatureFormatter setNegativeSuffix:@" °F"];
	[_temperatureFormatter setNilSymbol:@"---"];
	
	_floatFormatter = [NSNumberFormatter new];
	[_floatFormatter setNumberStyle:NSNumberFormatterDecimalStyle];
	[_floatFormatter setMaximumFractionDigits:2];
	[_floatFormatter setNilSymbol:@"---"];
}

- (void)viewDidLoad {
	
	[super viewDidLoad];
	
	//	if (self.bleCentral == nil) self.bleCentral = [[CBCentralManager alloc] initWithDelegate:self queue:nil options:@{CBCentralManagerOptionRestoreIdentifierKey:CBCentralManagerIdentifier}];
	self.bleCentral = [[CBCentralManager alloc] initWithDelegate:self queue:nil];
	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(appBecameActive:) name:UIApplicationDidBecomeActiveNotification object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(appEnteredBackround:) name:UIApplicationDidEnterBackgroundNotification object:nil];
	
	[self bootstrapModel];
}

- (void)bootstrapModel {
	
	// Setup Dynamic TableView section descriptors
	self.sectionDescriptors = @[({
		TableSectionDescriptor *sectionDescriptor = [TableSectionDescriptor new];
		sectionDescriptor.sectionName = @"Greenhouse State";
		sectionDescriptor.rowCount = @1;
		sectionDescriptor.rowDescriptors = @[
											 ({
												 TableRowDescriptor *rowDescriptor = [TableRowDescriptor new];
												 rowDescriptor.cellReuseIdentifier = @"LabelCell";
												 rowDescriptor.modelObject = VENTING_NECESSITY_CHARACTERISTIC_UUID;
												 rowDescriptor.configureCellFromModelBlock = ^(UITableViewCell *cell, id modelObject) {
													 
													 UILabel *titleLabel = (UILabel *)[cell.contentView viewWithTag:CELL_TITLE_LABEL_TAG];
													 UILabel *valueLabel = (UILabel *)[cell.contentView viewWithTag:CELL_VALUE_LABEL_TAG];
													 NSNumber *value = self.greenhouseValues[modelObject];
													 
													 // Configure Cell
													 titleLabel.text = NSLocalizedString(@"Venting Necessity", @"Cell title label");
													 valueLabel.text = [_floatFormatter stringForObjectValue:value];
												 };
												 rowDescriptor;
											 }),
											 ({
												 TableRowDescriptor *rowDescriptor = [TableRowDescriptor new];
												 rowDescriptor.cellReuseIdentifier = @"LabelCell";
												 rowDescriptor.modelObject = VENTING_NECESS_TARGET_CHARACTERISTIC_UUID;
												 rowDescriptor.configureCellFromModelBlock = ^(UITableViewCell *cell, id modelObject) {
													 
													 UILabel *titleLabel = (UILabel *)[cell.contentView viewWithTag:CELL_TITLE_LABEL_TAG];
													 UILabel *valueLabel = (UILabel *)[cell.contentView viewWithTag:CELL_VALUE_LABEL_TAG];
													 NSNumber *value = self.greenhouseValues[modelObject];
													 
													 // Configure Cell
													 titleLabel.text = NSLocalizedString(@"Venting Necessity Target", @"Cell title label");
													 valueLabel.text = [_floatFormatter stringForObjectValue:value];
												 };
												 rowDescriptor;
											 }),
											 ({
												 TableRowDescriptor *rowDescriptor = [TableRowDescriptor new];
												 rowDescriptor.cellReuseIdentifier = @"LabelCell";
												 rowDescriptor.modelObject = CONTROL_STATE_CHARACTERISTIC_UUID;
												 rowDescriptor.configureCellFromModelBlock = ^(UITableViewCell *cell, id modelObject) {
													 
													 UILabel *titleLabel = (UILabel *)[cell.contentView viewWithTag:CELL_TITLE_LABEL_TAG];
													 UILabel *valueLabel = (UILabel *)[cell.contentView viewWithTag:CELL_VALUE_LABEL_TAG];
													 NSNumber *value = self.greenhouseValues[modelObject];
													 
													 // Configure Cell
													 titleLabel.text = NSLocalizedString(@"Control State", @"Cell title label");
													 NSString *stateLabel = @"---";
													 
													 if (value) switch (value.integerValue) {
														 case ClimateStateSteady: stateLabel = NSLocalizedString(@"Steady", @"Cell value label"); break;
														 case ClimateStateDecreasingHumidity: stateLabel = NSLocalizedString(@"Lowering Humidity", @"Cell value label"); break;
														 default: stateLabel = @"---"; break;
													 }
													 valueLabel.text = stateLabel;
												 };
												 rowDescriptor;
											 })
											 ];
		sectionDescriptor;
	}),
								({
									TableSectionDescriptor *sectionDescriptor = [TableSectionDescriptor new];
									sectionDescriptor.sectionName = @"Measurements";
									sectionDescriptor.rowCount = @1;
									sectionDescriptor.rowDescriptors = @[
																		 ({
																			 TableRowDescriptor *rowDescriptor = [TableRowDescriptor new];
																			 rowDescriptor.cellReuseIdentifier = @"LabelCell";
																			 rowDescriptor.modelObject = EXTERIOR_HUMIDITY_CHARACTERISTIC_UUID;
																			 rowDescriptor.configureCellFromModelBlock = ^(UITableViewCell *cell, id modelObject) {
																				 
																				 UILabel *titleLabel = (UILabel *)[cell.contentView viewWithTag:CELL_TITLE_LABEL_TAG];
																				 UILabel *valueLabel = (UILabel *)[cell.contentView viewWithTag:CELL_VALUE_LABEL_TAG];
																				 NSNumber *value = self.greenhouseValues[modelObject];
																				 
																				 // Configure Cell
																				 titleLabel.text = NSLocalizedString(@"Exterior Humidity", @"Cell title label");
																				 valueLabel.text = [_humidityFormatter stringForObjectValue:value];
																			 };
																			 rowDescriptor;
																		 }),
																		 ({
																			 TableRowDescriptor *rowDescriptor = [TableRowDescriptor new];
																			 rowDescriptor.cellReuseIdentifier = @"LabelCell";
																			 rowDescriptor.modelObject = INTERIOR_HUMIDITY_CHARACTERISTIC_UUID;
																			 rowDescriptor.configureCellFromModelBlock = ^(UITableViewCell *cell, id modelObject) {
																				 
																				 UILabel *titleLabel = (UILabel *)[cell.contentView viewWithTag:CELL_TITLE_LABEL_TAG];
																				 UILabel *valueLabel = (UILabel *)[cell.contentView viewWithTag:CELL_VALUE_LABEL_TAG];
																				 NSNumber *value = self.greenhouseValues[modelObject];
																				 
																				 // Configure Cell
																				 titleLabel.text = NSLocalizedString(@"Interior Humidity", @"Cell title label");
																				 valueLabel.text = [_humidityFormatter stringForObjectValue:value];
																			 };
																			 rowDescriptor;
																		 }),
																		 ({
																			 TableRowDescriptor *rowDescriptor = [TableRowDescriptor new];
																			 rowDescriptor.cellReuseIdentifier = @"LabelCell";
																			 rowDescriptor.modelObject = EXTERIOR_TEMPERATURE_CHARACTERISTIC_UUID;
																			 rowDescriptor.configureCellFromModelBlock = ^(UITableViewCell *cell, id modelObject) {
																				 
																				 UILabel *titleLabel = (UILabel *)[cell.contentView viewWithTag:CELL_TITLE_LABEL_TAG];
																				 UILabel *valueLabel = (UILabel *)[cell.contentView viewWithTag:CELL_VALUE_LABEL_TAG];
																				 NSNumber *celciusValue = self.greenhouseValues[modelObject];
																				 NSNumber *fahrenheitValue = [[NSValueTransformer valueTransformerForName:@"CelciusToFahrenheitTransformer"] transformedValue:celciusValue];
																				 
																				 // Configure Cell
																				 titleLabel.text = NSLocalizedString(@"Exterior Temperature", @"Cell title label");
																				 valueLabel.text = [_temperatureFormatter stringForObjectValue:fahrenheitValue];
																			 };
																			 rowDescriptor;
																		 }),
																		 ({
																			 TableRowDescriptor *rowDescriptor = [TableRowDescriptor new];
																			 rowDescriptor.cellReuseIdentifier = @"LabelCell";
																			 rowDescriptor.modelObject = INTERIOR_TEMPERATURE_CHARACTERISTIC_UUID;
																			 rowDescriptor.configureCellFromModelBlock = ^(UITableViewCell *cell, id modelObject) {
																				 
																				 UILabel *titleLabel = (UILabel *)[cell.contentView viewWithTag:CELL_TITLE_LABEL_TAG];
																				 UILabel *valueLabel = (UILabel *)[cell.contentView viewWithTag:CELL_VALUE_LABEL_TAG];
																				 NSNumber *celciusValue = self.greenhouseValues[modelObject];
																				 NSNumber *fahrenheitValue = [[NSValueTransformer valueTransformerForName:@"CelciusToFahrenheitTransformer"] transformedValue:celciusValue];
																				 
																				 // Configure Cell
																				 titleLabel.text = NSLocalizedString(@"Interior Temperature", @"Cell title label");
																				 valueLabel.text = [_temperatureFormatter stringForObjectValue:fahrenheitValue];
																			 };
																			 rowDescriptor;
																		 })
																		 ];
									sectionDescriptor;
								}),
								({
									TableSectionDescriptor *sectionDescriptor = [TableSectionDescriptor new];
									sectionDescriptor.sectionName = @"Controls";
									sectionDescriptor.rowCount = @1;
									sectionDescriptor.rowDescriptors = @[
																		 ({
																			 TableRowDescriptor *rowDescriptor = [TableRowDescriptor new];
																			 rowDescriptor.cellReuseIdentifier = @"SwitchCell";
																			 rowDescriptor.modelObject = VENT_FLAP_CHARACTERISTIC_UUID;
																			 rowDescriptor.configureCellFromModelBlock = ^(UITableViewCell *cell, id modelObject) {
																				 
																				 UILabel *titleLabel = (UILabel *)[cell.contentView viewWithTag:CELL_TITLE_LABEL_TAG];
																				 UILabel *valueLabel = (UILabel *)[cell.contentView viewWithTag:CELL_VALUE_LABEL_TAG];
																				 UISwitch *aSwitch = (UISwitch *)[cell.contentView viewWithTag:CELL_SWITCH_TAG];
																				 NSNumber *value = self.greenhouseValues[modelObject];
																				 
																				 // Configure Cell
																				 titleLabel.text = NSLocalizedString(@"Vent Flap Open", @"Cell title label");
																				 valueLabel.text = [NSString stringWithFormat:@"%d", (int) value.integerValue];
																				 aSwitch.on = (value.integerValue == VENT_FLAP_OPEN);
																				 
																			 };
																			 //																			 rowDescriptor.updateModelFromCellBlock = ^(UITableViewCell *cell, id modelObject) {
																			 //
																			 //																				 UISwitch *aSwitch = (UISwitch *)[cell.contentView viewWithTag:CELL_SWITCH_TAG];
																			 //																				 [modelObject setValue:@((aSwitch.on) ? VENT_FLAP_OPEN : VENT_FLAP_CLOSED) forKey:VENT_FLAP_CHARACTERISTIC_UUID];
																			 //																			 };
																			 rowDescriptor;
																		 }),
																		 ({
																			 TableRowDescriptor *rowDescriptor = [TableRowDescriptor new];
																			 rowDescriptor.cellReuseIdentifier = @"SwitchCell";
																			 rowDescriptor.modelObject = LIGHTBANK_1_CHARACTERISTIC_UUID;
																			 rowDescriptor.configureCellFromModelBlock = ^(UITableViewCell *cell, id modelObject) {
																				 
																				 UILabel *titleLabel = (UILabel *)[cell.contentView viewWithTag:CELL_TITLE_LABEL_TAG];
																				 UILabel *valueLabel = (UILabel *)[cell.contentView viewWithTag:CELL_VALUE_LABEL_TAG];
																				 UISwitch *aSwitch = (UISwitch *)[cell.contentView viewWithTag:CELL_SWITCH_TAG];
																				 NSNumber *value = self.greenhouseValues[modelObject];
																				 
																				 // Configure Cell
																				 titleLabel.text = NSLocalizedString(@"Light Bank 1", @"Cell title label");
																				 valueLabel.text = [NSString stringWithFormat:@"%d", (int) value.integerValue];
																				 aSwitch.on = (value.integerValue == LIGHTBANK_ON);
																				 aSwitch.userInteractionEnabled = NO;
																				 
																			 };
																			 //																			 rowDescriptor.updateModelFromCellBlock = ^(UITableViewCell *cell, id modelObject) {
																			 //
																			 //																				 UISwitch *aSwitch = (UISwitch *)[cell.contentView viewWithTag:CELL_SWITCH_TAG];
																			 //																				 [modelObject setValue:@((aSwitch.on) ? VENT_FLAP_OPEN : VENT_FLAP_CLOSED) forKey:LIGHTBANK_1_CHARACTERISTIC_UUID];
																			 //																			 };
																			 rowDescriptor;
																		 }),
																		 ({
																			 TableRowDescriptor *rowDescriptor = [TableRowDescriptor new];
																			 rowDescriptor.cellReuseIdentifier = @"SwitchCell";
																			 rowDescriptor.modelObject = LIGHTBANK_2_CHARACTERISTIC_UUID;
																			 rowDescriptor.configureCellFromModelBlock = ^(UITableViewCell *cell, id modelObject) {
																				 
																				 UILabel *titleLabel = (UILabel *)[cell.contentView viewWithTag:CELL_TITLE_LABEL_TAG];
																				 UILabel *valueLabel = (UILabel *)[cell.contentView viewWithTag:CELL_VALUE_LABEL_TAG];
																				 UISwitch *aSwitch = (UISwitch *)[cell.contentView viewWithTag:CELL_SWITCH_TAG];
																				 NSNumber *value = self.greenhouseValues[modelObject];
																				 
																				 // Configure Cell
																				 titleLabel.text = NSLocalizedString(@"Light Bank 2", @"Cell title label");
																				 valueLabel.text = [NSString stringWithFormat:@"%d", (int) value.integerValue];
																				 aSwitch.on = (value.integerValue == LIGHTBANK_ON);
																				 aSwitch.userInteractionEnabled = NO;
																				 
																			 };
																			 //																			 rowDescriptor.updateModelFromCellBlock = ^(UITableViewCell *cell, id modelObject) {
																			 //
																			 //																				 UISwitch *aSwitch = (UISwitch *)[cell.contentView viewWithTag:CELL_SWITCH_TAG];
																			 //																				 [modelObject setValue:@((aSwitch.on) ? VENT_FLAP_OPEN : VENT_FLAP_CLOSED) forKey:LIGHTBANK_1_CHARACTERISTIC_UUID];
																			 //																			 };
																			 rowDescriptor;
																		 })
																		 ];
									sectionDescriptor;
								}),
								({
									TableSectionDescriptor *sectionDescriptor = [TableSectionDescriptor new];
									sectionDescriptor.sectionName = @"User Adjustments";
									sectionDescriptor.rowCount = @1;
									sectionDescriptor.rowDescriptors = @[
																		 ({
																			 TableRowDescriptor *rowDescriptor = [TableRowDescriptor new];
																			 rowDescriptor.cellReuseIdentifier = @"SliderCell";
																			 rowDescriptor.modelObject = HUMIDITY_SETPOINT_CHARACTERISTIC_UUID;
																			 rowDescriptor.configureCellFromModelBlock = ^(UITableViewCell *cell, id modelObject) {
																				 
																				 UILabel *titleLabel = (UILabel *)[cell.contentView viewWithTag:CELL_TITLE_LABEL_TAG];
																				 UILabel *valueLabel = (UILabel *)[cell.contentView viewWithTag:CELL_VALUE_LABEL_TAG];
																				 UISlider *slider = (UISlider *)[cell.contentView viewWithTag:CELL_SLIDER_TAG];
																				 //
																				 NSNumber *value = self.greenhouseValues[modelObject];
																				 
																				 // Configure Cell
																				 titleLabel.text = NSLocalizedString(@"Humidity Setpoint", @"Cell title label");
																				 
																				 slider.maximumValue = HUMIDITY_SETPOINT_MAX;
																				 slider.minimumValue = HUMIDITY_SETPOINT_MIN;
																				 [slider setValue:value.floatValue animated:NO];
																				 
																				 NSUInteger origValue = _humidityFormatter.maximumFractionDigits;
																				 _humidityFormatter.maximumFractionDigits = 0;
																				 valueLabel.text = [_humidityFormatter stringForObjectValue:value];
																				 _humidityFormatter.maximumFractionDigits = origValue;
																			 };
																			 rowDescriptor.updateModelFromCellBlock = ^(UITableViewCell *cell, id modelObject) {
																				 
																				 UISlider *slider = (UISlider *)[cell.contentView viewWithTag:CELL_SLIDER_TAG];
																				 [self.greenhouseValues setValue:@(slider.value) forKey:modelObject];
																			 };
																			 rowDescriptor;
																		 }),
																		 ({
																			 TableRowDescriptor *rowDescriptor = [TableRowDescriptor new];
																			 rowDescriptor.cellReuseIdentifier = @"SliderCell";
																			 rowDescriptor.modelObject = TEMPERATURE_SETPOINT_CHARACTERISTIC_UUID;
																			 rowDescriptor.configureCellFromModelBlock = ^(UITableViewCell *cell, id modelObject) {
																				 
																				 UILabel *titleLabel = (UILabel *)[cell.contentView viewWithTag:CELL_TITLE_LABEL_TAG];
																				 UILabel *valueLabel = (UILabel *)[cell.contentView viewWithTag:CELL_VALUE_LABEL_TAG];
																				 UISlider *slider = (UISlider *)[cell.contentView viewWithTag:CELL_SLIDER_TAG];
																				 //
																				 NSNumber *celciusValue = self.greenhouseValues[modelObject];
																				 NSNumber *fahrenheitValue = [[NSValueTransformer valueTransformerForName:@"CelciusToFahrenheitTransformer"] transformedValue:celciusValue];
																				 
																				 
																				 // Configure Cell
																				 titleLabel.text = NSLocalizedString(@"Temperature Setpoint", @"Cell title label");
																				 
																				 slider.maximumValue = TEMPERATURE_SETPOINT_MAX;
																				 slider.minimumValue = TEMPERATURE_SETPOINT_MIN;
																				 [slider setValue:celciusValue.floatValue animated:NO];
																				 
																				 NSUInteger origValue = _temperatureFormatter.maximumFractionDigits;
																				 _temperatureFormatter.maximumFractionDigits = 0;
																				 valueLabel.text = [_temperatureFormatter stringForObjectValue:fahrenheitValue];
																				 _temperatureFormatter.maximumFractionDigits = origValue;
																			 };
																			 rowDescriptor.updateModelFromCellBlock = ^(UITableViewCell *cell, id modelObject) {
																				 
																				 UISlider *slider = (UISlider *)[cell.contentView viewWithTag:CELL_SLIDER_TAG];
																				 [self.greenhouseValues setValue:@(slider.value) forKey:modelObject];
																			 };
																			 rowDescriptor;
																		 }),
																		 ({
																			 TableRowDescriptor *rowDescriptor = [TableRowDescriptor new];
																			 rowDescriptor.cellReuseIdentifier = @"SliderCell";
																			 rowDescriptor.modelObject = ILLUMINATION_ON_TIME_CHARACTERISTIC_UUID;
																			 rowDescriptor.configureCellFromModelBlock = ^(UITableViewCell *cell, id modelObject) {
																				 
																				 UILabel *titleLabel = (UILabel *)[cell.contentView viewWithTag:CELL_TITLE_LABEL_TAG];
																				 UILabel *valueLabel = (UILabel *)[cell.contentView viewWithTag:CELL_VALUE_LABEL_TAG];
																				 UISlider *slider = (UISlider *)[cell.contentView viewWithTag:CELL_SLIDER_TAG];
																				 //
																				 NSNumber *value = self.greenhouseValues[modelObject];
																				 
																				 NSInteger minutesSinceMidnightUTC = value.integerValue;
																				 NSInteger minutesSinceMidnight = (minutesSinceMidnightUTC + (24 - 6)*60) % (24*60);
																				 
																				 // Configure Cell
																				 titleLabel.text = NSLocalizedString(@"Illumination On", @"Cell title label");
																				 
																				 slider.maximumValue = (60 * 24) - 1;
																				 slider.minimumValue = 0;
																				 
																				 if (value == nil || minutesSinceMidnightUTC == UNAVAILABLE_u) {
																					 
																					 [slider setValue:0 animated:NO];
																					 
																					 valueLabel.text = NSLocalizedString(@"Always On", @"Illumination on-time disabled label");
																					 
																				 } else {
																					 
																					 [slider setValue:minutesSinceMidnight animated:NO];
																					 
																					 UInt8 hours = floor(minutesSinceMidnight / 60);
																					 UInt8 minutes = minutesSinceMidnight - (hours * 60);
																					 
																					 UInt8 displayHours = fmod((hours + 11), 12) + 1;
																					 valueLabel.text = [NSString stringWithFormat:@"%02u:%02u %@", displayHours, minutes, (hours < 24 && hours >= 12) ? @"pm" : @"am"];
																				 }
																			 };
																			 rowDescriptor.updateModelFromCellBlock = ^(UITableViewCell *cell, id modelObject) {
																				 
																				 UISlider *slider = (UISlider *)[cell.contentView viewWithTag:CELL_SLIDER_TAG];
																				 
																				 NSInteger minutesSinceMidnight = slider.value;
																				 NSInteger minutesSinceMidnightUTC = (minutesSinceMidnight + (6*60)) % (24*60);
																				 
																				 NSNumber *aValue = (minutesSinceMidnight == 0) ? @(UNAVAILABLE_u) : @(minutesSinceMidnightUTC);  // Allow user to disable timer with min. slider position
																				 
																				 [self.greenhouseValues setValue:aValue forKey:modelObject];
																			 };
																			 rowDescriptor;
																		 }),
																		 ({
																			 TableRowDescriptor *rowDescriptor = [TableRowDescriptor new];
																			 rowDescriptor.cellReuseIdentifier = @"SliderCell";
																			 rowDescriptor.modelObject = ILLUMINATION_OFF_TIME_CHARACTERISTIC_UUID;
																			 rowDescriptor.configureCellFromModelBlock = ^(UITableViewCell *cell, id modelObject) {
																				 
																				 UILabel *titleLabel = (UILabel *)[cell.contentView viewWithTag:CELL_TITLE_LABEL_TAG];
																				 UILabel *valueLabel = (UILabel *)[cell.contentView viewWithTag:CELL_VALUE_LABEL_TAG];
																				 UISlider *slider = (UISlider *)[cell.contentView viewWithTag:CELL_SLIDER_TAG];
																				 //
																				 NSNumber *value = self.greenhouseValues[modelObject];
																				 
																				 NSInteger minutesSinceMidnightUTC = value.integerValue;
																				 NSInteger minutesSinceMidnight = (minutesSinceMidnightUTC + (24 - 6)*60) % (24*60);
																				 
																				 // Configure Cell
																				 titleLabel.text = NSLocalizedString(@"Illumination Off", @"Cell title label");
																				 
																				 slider.maximumValue = (60 * 24) - 1;
																				 slider.minimumValue = 0;
																				 
																				 if (value == nil || minutesSinceMidnightUTC == UNAVAILABLE_u) {
																					 
																					 [slider setValue:0 animated:NO];
																					 
																					 valueLabel.text = NSLocalizedString(@"Always On", @"Illumination on-time disabled label");
																					 
																				 } else {
																					 
																					 [slider setValue:minutesSinceMidnight animated:NO];
																					 
																					 UInt8 hours = floor(minutesSinceMidnight / 60);
																					 UInt8 minutes = minutesSinceMidnight - (hours * 60);
																					 
																					 UInt8 displayHours = fmod((hours + 11), 12) + 1;
																					 valueLabel.text = [NSString stringWithFormat:@"%02u:%02u %@", displayHours, minutes, (hours < 24 && hours >= 12) ? @"pm" : @"am"];
																				 }
																			 };
																			 rowDescriptor.updateModelFromCellBlock = ^(UITableViewCell *cell, id modelObject) {
																				 
																				 UISlider *slider = (UISlider *)[cell.contentView viewWithTag:CELL_SLIDER_TAG];
																				 
																				 NSInteger minutesSinceMidnight = slider.value;
																				 NSInteger minutesSinceMidnightUTC = (minutesSinceMidnight + (6*60)) % (24*60);
																				 
																				 NSNumber *aValue = (minutesSinceMidnight == 0) ? @(UNAVAILABLE_u) : @(minutesSinceMidnightUTC);  // Allow user to disable timer with min. slider position
																				 
																				 [self.greenhouseValues setValue:aValue forKey:modelObject];
																			 };
																			 rowDescriptor;
																		 }),
																		 ({
																			 TableRowDescriptor *rowDescriptor = [TableRowDescriptor new];
																			 rowDescriptor.cellReuseIdentifier = @"SliderCell";
																			 rowDescriptor.modelObject = HUMIDITY_NECESS_COEFF_CHARACTERISTIC_UUID;
																			 rowDescriptor.configureCellFromModelBlock = ^(UITableViewCell *cell, id modelObject) {
																				 
																				 UILabel *titleLabel = (UILabel *)[cell.contentView viewWithTag:CELL_TITLE_LABEL_TAG];
																				 UILabel *valueLabel = (UILabel *)[cell.contentView viewWithTag:CELL_VALUE_LABEL_TAG];
																				 UISlider *slider = (UISlider *)[cell.contentView viewWithTag:CELL_SLIDER_TAG];
																				 //
																				 NSNumber *value = self.greenhouseValues[modelObject];
																				 
																				 // Configure Cell
																				 titleLabel.text = NSLocalizedString(@"Hum. Necessity Coeff", @"Cell title label");
																				 
																				 slider.maximumValue = NECESSITY_COEFF_MAX;
																				 slider.minimumValue = NECESSITY_COEFF_MIN;
																				 [slider setValue:value.floatValue animated:NO];
																				 
																				 NSUInteger origValue = _floatFormatter.minimumFractionDigits;
																				 _floatFormatter.minimumFractionDigits = 2;
																				 valueLabel.text = [_floatFormatter stringForObjectValue:value];
																				 _floatFormatter.minimumFractionDigits = origValue;
																			 };
																			 rowDescriptor.updateModelFromCellBlock = ^(UITableViewCell *cell, id modelObject) {
																				 
																				 UISlider *slider = (UISlider *)[cell.contentView viewWithTag:CELL_SLIDER_TAG];
																				 [self.greenhouseValues setValue:@(slider.value) forKey:modelObject];
																			 };
																			 rowDescriptor;
																		 }),
																		 ({
																			 TableRowDescriptor *rowDescriptor = [TableRowDescriptor new];
																			 rowDescriptor.cellReuseIdentifier = @"SliderCell";
																			 rowDescriptor.modelObject = TEMP_NECESS_COEFF_CHARACTERISTIC_UUID;
																			 rowDescriptor.configureCellFromModelBlock = ^(UITableViewCell *cell, id modelObject) {
																				 
																				 UILabel *titleLabel = (UILabel *)[cell.contentView viewWithTag:CELL_TITLE_LABEL_TAG];
																				 UILabel *valueLabel = (UILabel *)[cell.contentView viewWithTag:CELL_VALUE_LABEL_TAG];
																				 UISlider *slider = (UISlider *)[cell.contentView viewWithTag:CELL_SLIDER_TAG];
																				 //
																				 NSNumber *value = self.greenhouseValues[modelObject];
																				 
																				 // Configure Cell
																				 titleLabel.text = NSLocalizedString(@"Temp. Necessity Coeff", @"Cell title label");
																				 
																				 slider.maximumValue = NECESSITY_COEFF_MAX;
																				 slider.minimumValue = NECESSITY_COEFF_MIN;
																				 [slider setValue:value.floatValue animated:NO];
																				 
																				 NSUInteger origValue = _floatFormatter.minimumFractionDigits;
																				 _floatFormatter.minimumFractionDigits = 2;
																				 valueLabel.text = [_floatFormatter stringForObjectValue:value];
																				 _floatFormatter.minimumFractionDigits = origValue;
																			 };
																			 rowDescriptor.updateModelFromCellBlock = ^(UITableViewCell *cell, id modelObject) {
																				 
																				 
																				 UISlider *slider = (UISlider *)[cell.contentView viewWithTag:CELL_SLIDER_TAG];
																				 [self.greenhouseValues setValue:@(slider.value) forKey:modelObject];
																			 };
																			 rowDescriptor;
																		 }),
																		 ({
																			 TableRowDescriptor *rowDescriptor = [TableRowDescriptor new];
																			 rowDescriptor.cellReuseIdentifier = @"SliderCell";
																			 rowDescriptor.modelObject = VENTING_NECESS_COEFF_CHARACTERISTIC_UUID;
																			 rowDescriptor.configureCellFromModelBlock = ^(UITableViewCell *cell, id modelObject) {
																				 
																				 UILabel *titleLabel = (UILabel *)[cell.contentView viewWithTag:CELL_TITLE_LABEL_TAG];
																				 UILabel *valueLabel = (UILabel *)[cell.contentView viewWithTag:CELL_VALUE_LABEL_TAG];
																				 UISlider *slider = (UISlider *)[cell.contentView viewWithTag:CELL_SLIDER_TAG];
																				 //
																				 NSNumber *value = self.greenhouseValues[modelObject];
																				 
																				 // Configure Cell
																				 titleLabel.text = NSLocalizedString(@"Venting Threshold", @"Cell title label");
																				 
																				 slider.maximumValue = NECESSITY_THRESHOLD_MAX;
																				 slider.minimumValue = NECESSITY_THRESHOLD_MIN;
																				 [slider setValue:value.floatValue animated:NO];
																				 
																				 NSUInteger origValue = _floatFormatter.minimumFractionDigits;
																				 _floatFormatter.minimumFractionDigits = 2;
																				 valueLabel.text = [_floatFormatter stringForObjectValue:value];
																				 _floatFormatter.minimumFractionDigits = origValue;
																			 };
																			 rowDescriptor.updateModelFromCellBlock = ^(UITableViewCell *cell, id modelObject) {
																				 
																				 
																				 UISlider *slider = (UISlider *)[cell.contentView viewWithTag:CELL_SLIDER_TAG];
																				 [self.greenhouseValues setValue:@(slider.value) forKey:modelObject];
																			 };
																			 rowDescriptor;
																		 }),
																		 ({
																			 TableRowDescriptor *rowDescriptor = [TableRowDescriptor new];
																			 rowDescriptor.cellReuseIdentifier = @"SliderCell";
																			 rowDescriptor.modelObject = VENT_NECESS_OVERSHOOT_CHARACTERISTIC_UUID;
																			 rowDescriptor.configureCellFromModelBlock = ^(UITableViewCell *cell, id modelObject) {
																				 
																				 UILabel *titleLabel = (UILabel *)[cell.contentView viewWithTag:CELL_TITLE_LABEL_TAG];
																				 UILabel *valueLabel = (UILabel *)[cell.contentView viewWithTag:CELL_VALUE_LABEL_TAG];
																				 UISlider *slider = (UISlider *)[cell.contentView viewWithTag:CELL_SLIDER_TAG];
																				 //
																				 NSNumber *value = self.greenhouseValues[modelObject];
																				 
																				 // Configure Cell
																				 titleLabel.text = NSLocalizedString(@"Venting Overshoot", @"Cell title label");
																				 
																				 slider.maximumValue = VENTING_OVERSHOOT_MAX;
																				 slider.minimumValue = VENTING_OVERSHOOT_MIN;
																				 [slider setValue:value.floatValue animated:NO];
																				 
																				 NSUInteger origValue = _humidityFormatter.maximumFractionDigits;
																				 _humidityFormatter.maximumFractionDigits = 0;
																				 valueLabel.text = [_humidityFormatter stringForObjectValue:value];
																				 _humidityFormatter.maximumFractionDigits = origValue;
																			 };
																			 rowDescriptor.updateModelFromCellBlock = ^(UITableViewCell *cell, id modelObject) {
																				 
																				 
																				 UISlider *slider = (UISlider *)[cell.contentView viewWithTag:CELL_SLIDER_TAG];
																				 [self.greenhouseValues setValue:@(slider.value) forKey:modelObject];
																			 };
																			 rowDescriptor;
																		 })
																		 ];
									sectionDescriptor;
								})
								];
	
	[self rebuildVisibilityIndex];
}

#pragma mark - IBActions

- (IBAction)didTapConnectButton:(id)sender {
	
	if (self.blePeripheral.state == CBPeripheralStateConnected) {  // Already connected
		
		[self.connectButton setTitle:@"Disconnecting..." forState:UIControlStateNormal];
		
		[self setNotificationForCharacteristics:[self.blePeripheral allCharacteristics] enabled:NO];
		
		[self.bleCentral cancelPeripheralConnection:self.blePeripheral];
		
	} else if (_isScanning) {  // Scan already in-progress
		
		[self stopScanningForDevices];
		
	} else {  // Not connected & not scanning
		
		[self tryToEstablishConnection];
	}
}

- (IBAction)didTapSwitch:(id)sender {
	
	UITableViewCell *tappedCell = [((UIView *)sender) parentTableViewCell];
	
	if (tappedCell) {
		
		NSIndexPath *indexPath = [self.tableView indexPathForCell:tappedCell];
		TableRowDescriptor *rowDescriptor = [self rowDescriptorForCellAtIndexPath:indexPath];
		
		if (rowDescriptor.updateModelFromCellBlock) rowDescriptor.updateModelFromCellBlock(tappedCell, rowDescriptor.modelObject);
		
		//		[self.tableView reloadRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationNone];
	}
}


- (IBAction)sliderValueChanged:(id)sender {
	
	UITableViewCell *tappedCell = [((UIView *)sender) parentTableViewCell];
	
	if (tappedCell) {
		
		NSIndexPath *indexPath = [self.tableView indexPathForCell:tappedCell];
		TableRowDescriptor *rowDescriptor = [self rowDescriptorForCellAtIndexPath:indexPath];
		
		if (rowDescriptor.updateModelFromCellBlock) rowDescriptor.updateModelFromCellBlock(tappedCell, rowDescriptor.modelObject);
		
		if (rowDescriptor.configureCellFromModelBlock) rowDescriptor.configureCellFromModelBlock(tappedCell, rowDescriptor.modelObject);
		
		//[self.tableView reloadRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationNone];
	}
}

- (IBAction)didStopDraggingSlider:(id)sender {
	
	if (self.blePeripheral.state == CBPeripheralStateConnected) {
		
		UITableViewCell *tappedCell = [((UIView *)sender) parentTableViewCell];
		
		if (tappedCell) {
			
			NSIndexPath *indexPath = [self.tableView indexPathForCell:tappedCell];
			TableRowDescriptor *rowDescriptor = [self rowDescriptorForCellAtIndexPath:indexPath];
			NSString *characteristicUUID = rowDescriptor.modelObject;
			
			CBCharacteristic *characteristic = [self bleCharacteristicWithString:characteristicUUID];
			
			if (characteristic.value) {
				
				switch (characteristic.value.length) {
						
					case sizeof(float): {
						
						float floatValue = [[self.greenhouseValues valueForKey:characteristicUUID] floatValue];
						NSData *buffer = [NSData dataWithBytes:&floatValue length:sizeof(float)];
						
						[self.blePeripheral writeValue:buffer forCharacteristic:characteristic type:CBCharacteristicWriteWithResponse];
						
						break;
					}
						
					case sizeof(SInt16): {
						
						SInt16 intValue = [[self.greenhouseValues valueForKey:characteristicUUID] integerValue];
						NSData *buffer = [NSData dataWithBytes:&intValue length:sizeof(SInt16)];
						
						[self.blePeripheral writeValue:buffer forCharacteristic:characteristic type:CBCharacteristicWriteWithResponse];
						
						break;
					}
						
					default: NSLog(@"WARNING: Encountered unexpected data type size while preparing to transmit value"); break;
				}
			}
		}
	}
	
}

#pragma mark -

- (void)appBecameActive:(NSNotification *)notification {
	
	if (self.blePeripheral.state == CBPeripheralStateConnected) {
		
		[self setNotificationForCharacteristics:[self.blePeripheral allCharacteristics] enabled:YES];
		[self readValueForCharacteristics:[self.blePeripheral allCharacteristics]];
		
	} else if (self.bleCentral.state == CBCentralManagerStatePoweredOn) {
		
		[self didTapConnectButton:nil];
	}
	
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
	
	if (self.blePeripheral.state == CBPeripheralStateConnected) {
		
		[self setNotificationForCharacteristics:[self.blePeripheral allCharacteristics] enabled:NO];  // Stop notifying
		
		[self.bleCentral cancelPeripheralConnection:self.blePeripheral];  // Disconnect
	}
	
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

- (void)tryToEstablishConnection {
	
	NSString *UUIDString = [[NSUserDefaults standardUserDefaults] stringForKey:PreviouslyConnectedPeripheralUUID];
	
	if (UUIDString) {  // We've previously connected to a device
		
		CBUUID *peripheralUUID = [CBUUID UUIDWithString:UUIDString];
		NSArray *peripherals = [self.bleCentral retrievePeripheralsWithIdentifiers:@[peripheralUUID]];
		
		if (peripherals.count > 0) {  // iOS has a record for our device cached
			
			NSLog(@"Attempting to reconnect to a remembered peripheral");
			
			_previousPeripheralAttempt = YES;
			[self connectToPeripheral:peripherals.firstObject];
		}
		
	} else {  // No memory of previously connected devices
		
		[self scanForDevices];
	}
}

- (void)scanForDevices {
	
	if (_isScanning == NO) {  // Don't continue if we're already scanning
		
		NSLog(@"Scanning for available peripherals...");
		
		_previousPeripheralAttempt = NO;
		
		//		[self.activityIndicator startAnimating];
		self.connectButton.selected = YES;
		
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
	
	//	[self.activityIndicator stopAnimating];
	
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

- (void)readValueForCharacteristics:(NSArray *)characteristics {
	
	for (CBCharacteristic *aCharacteristic in characteristics) {
		
		if ((aCharacteristic.properties & CBCharacteristicPropertyRead) == CBCharacteristicPropertyRead) {  // Check that Characteristic supports 'Notify'
			
			[self.blePeripheral readValueForCharacteristic:aCharacteristic];
			
		}
		
		if ([aCharacteristic.UUID.UUIDString isEqualToString:DATE_TIME_CHARACTERISTIC_UUID]) {
			
			// Update the device's clock
			UInt32 unixTime = [[NSDate date] timeIntervalSince1970];
			NSData *buffer = [NSData dataWithBytes:&unixTime length:sizeof(UInt32)];
			
			[self.blePeripheral writeValue:buffer forCharacteristic:aCharacteristic type:CBCharacteristicWriteWithResponse];
			
		}
	}
	
};

- (void)setNotificationForCharacteristics:(NSArray *)characteristics enabled:(BOOL)isEnabled {
	
	for (CBCharacteristic *aCharacteristic in characteristics) {
		
		if ((aCharacteristic.properties & CBCharacteristicPropertyNotify) == CBCharacteristicPropertyNotify) {  // Check that Characteristic supports 'Notify'
			
			if (aCharacteristic.isNotifying != isEnabled) [self.blePeripheral setNotifyValue:isEnabled forCharacteristic:aCharacteristic];
		}
	}
}

#pragma mark - CBCentralManagerDelegate methods

- (void)centralManagerDidUpdateState:(CBCentralManager *)central {
	
	if (central.state == CBCentralManagerStatePoweredOn) {
		
		self.connectButton.enabled = YES;
		
		[self.connectButton setTitle:@"Connect" forState:UIControlStateNormal];
		[self.connectButton setTitle:@"Scanning..." forState:UIControlStateSelected];
		
		[self didTapConnectButton:nil];  // Auto-connect
		
	} else {
		
		self.connectButton.enabled = NO;
	}
}

/*
 - (void)centralManager:(CBCentralManager *)central willRestoreState:(NSDictionary *)dict {
 
 NSArray *peripherals = dict[CBCentralManagerRestoredStatePeripheralsKey];
 
 if (peripherals.count > 0) {
 
 NSLog(@"Attempting connection to restored CBPeripheral");
 [self connectToPeripheral:peripherals[0]];
 
 } else NSLog(@"Restored CentralManager had no peripherals");
 }
 */

- (void)centralManager:(CBCentralManager *)central didDiscoverPeripheral:(CBPeripheral *)peripheral advertisementData:(NSDictionary *)advertisementData RSSI:(NSNumber *)RSSI {
	
	[self stopScanningForDevices];
	
	[self connectToPeripheral:peripheral];
}

- (void)centralManager:(CBCentralManager *)central didConnectPeripheral:(CBPeripheral *)peripheral {
	
	self.connectButton.selected = NO;
	
	[self.connectButton setTitle:@"Disconnect" forState:UIControlStateNormal];
	[self.connectButton setTitle:@"Disconnecting..." forState:UIControlStateSelected];
	
	if (_previousPeripheralAttempt) NSLog(@"Re-connected to previous peripheral");
	else NSLog(@"Connected to peripheral found during scan");
	
	// Store the peripherials 'identifier' UUID so we can try to reconnect later
	[[NSUserDefaults standardUserDefaults] setValue:peripheral.identifier.UUIDString forKey:PreviouslyConnectedPeripheralUUID];
	[[NSUserDefaults standardUserDefaults] synchronize];
	
	
	[peripheral discoverServices:nil];  // Discover all services
	
	CBCharacteristic *dateTimeCharacteristic = [self bleCharacteristicWithString:DATE_TIME_CHARACTERISTIC_UUID];
	if (dateTimeCharacteristic) {
		
		// Update the device's clock
		UInt32 unixTime = [[NSDate date] timeIntervalSince1970];
		NSData *buffer = [NSData dataWithBytes:&unixTime length:sizeof(UInt32)];
		
		[self.blePeripheral writeValue:buffer forCharacteristic:dateTimeCharacteristic type:CBCharacteristicWriteWithResponse];
		
	}
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
	
	if (_previousPeripheralAttempt == YES) {  // Attempt to connect to a remembered peripheral failed
		
		NSLog(@"Attempt to connect to previously used peripheral failed, falling back to scanning...");
		
		[self scanForDevices];
	}
}



#pragma mark - CBPeripheralDelegate methods

- (void)peripheral:(CBPeripheral *)peripheral didDiscoverServices:(NSError *)error {
	
	for (CBService *aService in peripheral.services) {
		
		[peripheral discoverCharacteristics:nil forService:aService];  // Discover all characteristics
	}
}

- (void)peripheral:(CBPeripheral *)peripheral didDiscoverCharacteristicsForService:(CBService *)service error:(NSError *)error {
	
	[self setNotificationForCharacteristics:service.characteristics enabled:YES];
	[self readValueForCharacteristics:service.characteristics];
}

- (void)peripheral:(CBPeripheral *)peripheral didUpdateNotificationStateForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error {
	
	//	NSLog(@"NOTIFY changed: %@", characteristic);
}

- (void)peripheral:(CBPeripheral *)peripheral didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error {
	
	switch (characteristic.value.length) {
			
		case sizeof(float): {
			
			float floatValue;
			[characteristic.value getBytes:&floatValue length:characteristic.value.length];
			
			if (floatValue == UNAVAILABLE_f) [self.greenhouseValues removeObjectForKey:characteristic.UUID.UUIDString];
			else [self.greenhouseValues setValue:@(floatValue) forKey:characteristic.UUID.UUIDString];
			
			break;
		}
			
		case sizeof(UInt8): {
			
			UInt8 intValue;
			[characteristic.value getBytes:&intValue length:characteristic.value.length];
			
			[self.greenhouseValues setValue:@(intValue) forKey:characteristic.UUID.UUIDString];
			
			break;
		}
			
		case sizeof(SInt16): {
			
			SInt16 intValue;
			[characteristic.value getBytes:&intValue length:characteristic.value.length];
			
			if (intValue == UNAVAILABLE_u) [self.greenhouseValues removeObjectForKey:characteristic.UUID.UUIDString];
			else [self.greenhouseValues setValue:@(intValue) forKey:characteristic.UUID.UUIDString];
			
			break;
		}
			
		default: NSLog(@"Encountered Characteristic with value that is neither float or UInt8");
	}
	
	// Update the relevant TableView rows
	NSArray *rowDescriptors = [self rowDescriptorsWithModelObject:characteristic.UUID.UUIDString];
	NSMutableArray *indexPaths = [NSMutableArray new];
	for (TableRowDescriptor *aRowDescriptor in rowDescriptors) {
		
		NSIndexPath *indexPathToReload = [self indexPathOfCellForRowDescriptor:aRowDescriptor];
		[indexPaths addObject:indexPathToReload];
	}
	
	[self.tableView reloadRowsAtIndexPaths:indexPaths withRowAnimation:UITableViewRowAnimationNone];
	
	/*
	 if ([UIApplication sharedApplication].applicationState == UIApplicationStateBackground) {
	 
	 UILocalNotification *localNotification = [UILocalNotification new];
	 localNotification.alertBody = @"Temperture Alert!";//[NSString stringWithFormat:@"Temperature is %1.0f°", incomingPacket->temperature];
	 localNotification.soundName = UILocalNotificationDefaultSoundName;
	 
	 [[UIApplication sharedApplication] presentLocalNotificationNow:localNotification];
	 }
	 */
}

- (void)peripheral:(CBPeripheral *)peripheral didWriteValueForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error {
	
	// NOTE: Called only for WRITE WITH RESPONSE (i.e. ACK'd)
	if (error) {
		
		NSLog(@"BLE write failed: %@", error);
	}
	//	else NSLog(@"Got confirmation of WRITE");
}

@end
