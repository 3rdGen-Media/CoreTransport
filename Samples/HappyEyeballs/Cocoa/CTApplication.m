//
//  CTApplication.m
//  HappyEyeballs-OSX
//
//  Created by Joe Moulton on 7/25/22.
//

#import "CTApplication.h"
#import "CTAppInterface.h"
#import "CTApplicationDelegate.h"

#if TARGET_OS_OSX
#define RunLoopEvent NSEvent
#else
#define RunLoopEvent UIEvent
#endif

@implementation CTApplication

BOOL _shouldKeepRunning = YES;

+(CTApplication*)sharedInstance{
    return (CTApplication*)(CocoaApplication.sharedApplication);
}

-(id)init
{
    
    self = [super init];
    if(self)
    {
        NSLog(@"CTApplication::init");

#if TARGET_OS_OSX
        //Create Custom App Delegate Here To Avoid Having to Use .xib
        self.delegate = [CTApplicationDelegate sharedInstance];
#else
        //If we didn't start a dedicated process or thread in Core Render
        //We can optionally start a display sync update process here, like in
        //our overrided iOS/tvOS UIApplication class:
        //[self StartDisplaySyncEventPublishThread];
#endif
    }
    return self;
}


/*
-(void)stop
{
    
    self postEvent
}
*/


/*
-(void)stop
{
    //_shouldKeepRunning = NO;
    [self stop:self];
    
    NSEvent* event = [NSEvent otherEventWithType: NSEventTypeApplicationDefined
                                                         location: NSMakePoint(0,0)
                                                   modifierFlags: 0
                                                       timestamp: 0.0
                                                    windowNumber: 0
                                                         context: nil
                                                         subtype: 0
                                                           data1: 0
                                                           data2: 0];
    [NSApp postEvent: event atStart:NO];
    
     
}
*/

//If we just want to tap into the events received on the NSApplication RunLoop
//We don't need to override a custom run method, but rather
//just override the sendEvent function that gets called from that super method
-(void)sendEvent:(RunLoopEvent*)event
{
    //custom event loop tap
    //NSLog(@"NSEvent type:  %d\n", CGEventGetType(event.CGEvent));
    
    //Filter events such as keystrokes and send them to CoreTransport
    /*
    //dispatch event to Core Render C-Land Application Event Loop!
    if( event && event.CGEvent)
    {
        struct kevent kev;
        EV_SET(&kev, crevent_platform_event, EVFILT_USER, 0, NOTE_TRIGGER, 0, event.CGEvent);
        kevent(cr_appEventQueue, &kev, 1, NULL, 0, NULL);
    }
    */
    
    [super sendEvent:event];
}


- (void) mainWindowChanged:(NSNotification *) notification
{
    
#if TARGET_OS_OSX
    // [notification name] should always be @"TestNotification"
    // unless you use this method for observation of other notifications
    // as well.
    NSWindow * window = (NSWindow*)(notification.object);
    //NSLog(@"Main Window id: %d", [[self mainWindow] windowNumber]);
    ////NSLog(@"Main Window id: %d", [window windowNumber]);

    CGWindowID windowID = (int)[window windowNumber];
    //Tell the Core Render CGWindows that the Cocoa Main Window has updated
  
    //[window resignKeyWindow];
    //[window resignMainWindow];
    //[window orderBack:self];
    //[window orderOut:self];
    
    //struct kevent kev;
    //EV_SET(&kev, crevent_main_window_changed, EVFILT_USER, 0, NOTE_TRIGGER, 0, (void*)windowID);
    //kevent(cr_mainEventQueue, &kev, 1, NULL, 0, NULL);
    
    //if ([[notification name] isEqualToString:@"TestNotification"])
    //    NSLog (@"Successfully received the test notification!");
#else
    
#endif
}

#if TARGET_OS_OSX

- (void)run
{
    //create a kqueue to listen for incoming events
    CTPlatformEventQueue = CTKernelQueueCreate();

    //register for incoming kqueue events
    for (uint64_t event = CTPlatformEvent_Init; event < CTPlatformEvent_Idle; ++event) {
        struct kevent kev;
        EV_SET(&kev, event, EVFILT_USER, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, NULL);
        kevent(CTPlatformEventQueue.kq, &kev, 1, NULL, 0, NULL);
    }

    
    @autoreleasepool
    {
        //[self activateIgnoringOtherApps : YES];
        //[[NSRunningApplication currentApplication] activateWithOptions:(NSApplicationActivateAllWindows | NSApplicationActivateIgnoringOtherApps)];
        //[self preventWindowOrdering];
        
        [self finishLaunching];

        //send kevent to CoreTransport notifying it that NSApplication RunLoop is now running
        struct kevent kev;
        EV_SET(&kev, CTProcessEvent_Init, EVFILT_USER, EV_ADD | EV_ENABLE | EV_ONESHOT, NOTE_FFCOPY|NOTE_TRIGGER|0x1, 0, NULL);
        kevent(CTProcessEventQueue.kq, &kev, 1, NULL, 0, NULL);
        //monotonicTimeNanos();

        //[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(mainWindowChanged:) name:NSWindowDidBecomeMainNotification object:nil];
        
        do
        {
            //[pool release];
           // pool = [[NSAutoreleasePool alloc] init];
            @autoreleasepool
            {
                //pump event queue, sleep until next event received
                NSEvent *event = [self nextEventMatchingMask:NSUIntegerMax untilDate:[NSDate distantFuture] inMode:NSDefaultRunLoopMode dequeue:YES];
                
                struct kevent kev;
                CTPlatformEvent kev_type;
                
                //filter for the kevents we want to receive
                kev_type = (CTPlatformEvent)ct_event_queue_wait_with_timeout(CTPlatformEventQueue.kq, &kev, EVFILT_USER, CTPlatformEvent_Timeout, CTPlatformEvent_OutOfRange, CTPlatformEvent_Exit, CTPlatformEvent_Exit, 0);
                
                //NSLog(@"kev_type = %d\n", kev_type);

            
                if( kev_type == CTPlatformEvent_Exit)
                {
                    //CoreTransport notified us that it wants Cocoa to terminate
                    NSLog(@"NSEvent Loop CTPlatformEvent_Exit");
                    assert(1==0);
                }
                
                //NSLog(@"NSEvent type:  %d\n", CGEventGetType(event.CGEvent));
                
                //dispatch event to Core Transport C-Land Application/Process Event Loop!
                //if( event && event.CGEvent)
                //{
                //    struct kevent kev;
                //    EV_SET(&kev, CTProcessEvent_PlatformEvent, EVFILT_USER, 0, NOTE_TRIGGER, 0, event.CGEvent);
                //    kevent(CTProcessEventQueue.kq, &kev, 1, NULL, 0, NULL);
                //}
                
                //handle regular NSApplication run loop housekeeping
                [self sendEvent:event];
                [self updateWindows];
                
                //yield();
                //CFRunLoopWakeUp([NSRunLoop.currentRunLoop getCFRunLoop]);
                 
            }
        } while (_shouldKeepRunning);
        
    }
    
    //cleanup dispatch queue creatd in core render
    //if(_cgEventQueue)
    //    dispatch_release(_cgEventQueue);
    //_cgEventQueue = NULL;
    
    //[pool release];
}
#else

- (void)replyToApplicationShouldTerminate:(BOOL)shouldTerminate
{
    
    
}


#endif
@end
