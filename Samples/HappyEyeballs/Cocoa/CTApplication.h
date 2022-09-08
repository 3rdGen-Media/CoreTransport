//
//  CTApplication.h
//  HappyEyeballs-OSX
//
//  Created by Joe Moulton on 7/25/22.
//

#import <TargetConditionals.h>

#if TARGET_OS_OSX
#import <Cocoa/Cocoa.h>
//#import <AppKit/AppKit.h>
#define CocoaApplication NSApplication
#else
#import <UIKit/UIKit.h>
#define CocoaApplication UIApplication
#endif

NS_ASSUME_NONNULL_BEGIN
@interface CTApplication : CocoaApplication

+(CTApplication*)sharedInstance;
-(void)replyToApplicationShouldTerminate:(BOOL)shouldTerminate;

@end
NS_ASSUME_NONNULL_END
