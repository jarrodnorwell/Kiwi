//
//  KiwiObjC.h
//
//
//  Created by Jarrod Norwell on 21/2/2024.
//

#import <Foundation/Foundation.h>

#ifdef __cplusplus
#include <cstdint>
#endif

NS_ASSUME_NONNULL_BEGIN

@interface KiwiObjC : NSObject
+(KiwiObjC *) sharedInstance NS_SWIFT_NAME(shared());
-(void) insertGame:(NSURL *)url NS_SWIFT_NAME(insert(game:));
-(void) step;

-(uint32_t*) screenFramebuffer;

-(void) virtualControllerButtonDown:(uint8_t)button;
-(void) virtualControllerButtonUp:(uint8_t)button;
@end

NS_ASSUME_NONNULL_END
