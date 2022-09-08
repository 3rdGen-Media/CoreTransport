//
//  CTViewController.h
//  HappyEyeballs
//
//  Created by Joe Moulton on 7/24/22.
//

#import <TargetConditionals.h>

#if TARGET_OS_OSX
#import <Cocoa/Cocoa.h>
#define CocoaViewController NSViewController
#else
#import <UIKit/UIKit.h>
#define CocoaViewController UIViewController
#endif

@interface CTViewController : CocoaViewController

@end

