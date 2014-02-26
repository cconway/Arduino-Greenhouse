//
//  CelciusToFahrenheitTransformer.m
//  Arduino Greenhouse
//
//  Created by Chas Conway on 2/21/14.
//  Copyright (c) 2014 Chas Conway. All rights reserved.
//

#import "CelciusToFahrenheitTransformer.h"

@implementation CelciusToFahrenheitTransformer

+ (void)initialize {
	
	NSString * const CtoFTransformerName = @"CelciusToFahrenheitTransformer";
	
	// Set the value transformer
	[NSValueTransformer setValueTransformer:[[CelciusToFahrenheitTransformer alloc] init] forName:CtoFTransformerName];
}

+ (Class)transformedValueClass {
	
	return [NSNumber class];
}

+ (BOOL)allowsReverseTransformation {
	
    return YES;
}

- (id)transformedValue:(id)value {
	
	if (value == nil) return nil;
	else {
	
		double celciusValue = [((NSNumber *)value) doubleValue];
		double fahrenheitValue = celciusValue * 1.8 + 32.0;
		
		return [NSNumber numberWithDouble:fahrenheitValue];
		
	}
}

- (id)reverseTransformedValue:(id)value {
	
	if (value == nil) return nil;
	else {
		
		double fahrenheitValue = [((NSNumber *)value) doubleValue];
		double celciusValue = (fahrenheitValue - 32) * (5.0/9.0);
		
		return [NSNumber numberWithDouble:celciusValue];
	}
}

@end
