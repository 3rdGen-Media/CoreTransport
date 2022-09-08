//
//  AppDelegate.h
//  HappyEyeballs
//
//  Created by Joe Moulton on 7/24/22.
//


#import <TargetConditionals.h>

#if TARGET_OS_OSX
#import <Cocoa/Cocoa.h>
#define CocoaDelegateSuperclass NSObject
#define CocoaAppDelegate        NSApplicationDelegate
#define CocoaWindow             NSWindow
#import "CTMenu.h"
#else
#define CocoaDelegateSuperclass UIResponder
#define CocoaAppDelegate        UIApplicationDelegate, UIWindowSceneDelegate
#define CocoaWindow             UIWindow
#endif

#import "CTApplication.h"

@interface CTApplicationDelegate : CocoaDelegateSuperclass <CocoaAppDelegate>

+ (id)sharedInstance;

@property (strong, nonatomic) CocoaWindow *window;

//@property (nonatomic) NSWindowStyleMask modalWindowStyleMask;
//@property (nonatomic, retain) MastryAdminWindow * window;
//@property (nonatomic, retain) MastryAdminWindow * editWindow;

@end

