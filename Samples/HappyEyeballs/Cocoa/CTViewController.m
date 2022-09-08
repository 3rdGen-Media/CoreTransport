//
//  ViewController.m
//  HappyEyeballs
//
//  Created by Joe Moulton on 7/24/22.
//

#import "CTViewController.h"

@implementation CTViewController

- (void)viewDidLoad {
    [super viewDidLoad];

    // Do any additional setup after loading the view.
}


#if TARGET_OS_OSX
- (void)setRepresentedObject:(id)representedObject {
    [super setRepresentedObject:representedObject];

    // Update the view, if already loaded.
}
#else

#endif

@end
