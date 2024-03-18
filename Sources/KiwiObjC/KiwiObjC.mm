//
//  KiwiObjC.mm
//
//
//  Created by Jarrod Norwell on 21/2/2024.
//

#import "KiwiObjC.h"

#import "emulator.hpp"

std::unique_ptr<Emulator> kiwiEmulator;

@implementation KiwiObjC
+(KiwiObjC *) sharedInstance {
    static dispatch_once_t onceToken;
    static KiwiObjC *sharedInstance = NULL;
    dispatch_once(&onceToken, ^{
        sharedInstance = [[self alloc] init];
    });
    return sharedInstance;
}

-(void) insertGame:(NSURL *)url {
    kiwiEmulator = std::make_unique<Emulator>([url.path UTF8String]);
    [self reset];
}

-(void) reset {
    kiwiEmulator->reset();
}

-(void) step {
    kiwiEmulator->step();
}

-(uint32_t *) screenFramebuffer {
    return kiwiEmulator->get_screen_buffer();
}

-(void) virtualControllerButtonDown:(uint8_t)button {
    kiwiEmulator->get_controller(0)[0] |= button;
}

-(void) virtualControllerButtonUp:(uint8_t)button {
    kiwiEmulator->get_controller(0)[0] &= ~button;
}
@end
