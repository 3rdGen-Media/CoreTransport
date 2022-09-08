//
//  AppDelegate.m
//  HappyEyeballs
//
//  Created by Joe Moulton on 7/24/22.
//

#import "CTApplicationDelegate.h"
#import "CTAppInterface.h"
#import "CTViewController.h"

const char * CT_MENU_TITLES[] =
{
    "Core Transport",
    "SubMenu 2",
    "SubMenu 3"
};

@interface CTApplicationDelegate ()

@property (nonatomic, retain) CocoaViewController * rootViewController;

@end

@implementation CTApplicationDelegate

#pragma mark -- Create Menu

-(void)setMenuTitle:(NSString*)title
{
#if TARGET_OS_OSX
    NSMenuItem* menuItem = [[NSApp mainMenu] itemAtIndex:0];
    NSMenu *menu = [[[NSApp mainMenu] itemAtIndex:0] submenu];
    //NSString *title = @"Core Render";
    // Append some invisible character to title :)
    
    //NSFont* font = menu.font;

    title = [title stringByAppendingString:@"\x1b"];
    [menu setTitle:title];
    
    
    NSFont * newFont = menu.font;//[NSFont menuBarFontOfSize:0];
    
    NSDictionary *attributes = @{
                                 NSFontAttributeName: newFont,
                                 NSForegroundColorAttributeName: [NSColor greenColor]
                                 };
    
    NSMutableAttributedString *attrTitle = [[NSMutableAttributedString alloc] initWithString:@"View" attributes:attributes];
    //[attrTitle addAttribute:NSFontAttributeName value:menu.font range:NSMakeRange(0, title.length)];
    //[menu setTitle:attrTitle];
    //[menu setTitle:title];
    [menuItem setAttributedTitle:attrTitle];
    //[menuItem.submenu changeMenuFont:newFont];
    //[[menu setTitleColor:[NSColor redColor]];
    
    //[[NSApp mainMenu] setTitle:title];
    //NSFont* newFont = [NSFont boldSystemFontOfSize:font.pointSize];
    //[submenu setFont:newFont];
#else
    
#endif

}

-(void)createMenuBar
{
#if TARGET_OS_OSX
    [NSApp setMainMenu:[CTMenu sharedInstance]];
#else
    
#endif
}



+ (id)sharedInstance
{
    static CTApplicationDelegate *appDelegate = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        appDelegate = [[self alloc] init];
    });
    return appDelegate;
}


-(void)closeCurrentWindow
{
    
    
}

#pragma mark -- Init Delegate

-(id)init
{
    if(self = [super init]) {
        
        [self createWindow];
        
        // Setup Preference Menu Action/Target on MainMenu
        [self createMenuBar];
        
        //CRView * view = [[CRView alloc] initWithFrame:self.window.frame];
        //[self.window.contentView addSubview:view];
        // Create a view
        //view = [[NSTabView alloc] initWithFrame:CGRectMake(0, 0, 700, 700)];
    }
    return self;
}

-(void)createWindow
{
    if( !self.window )
    {
#if TARGET_OS_OSX
    /*
    NSRect contentSize = NSMakeRect(500.0, 500.0, 1000.0, 1000.0);
    NSUInteger windowStyleMask = NSTitledWindowMask | NSResizableWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask;
    self.window = [[NSWindow alloc] initWithContentRect:contentSize styleMask:windowStyleMask backing:NSBackingStoreBuffered defer:YES];
    self.window.backgroundColor = [NSColor whiteColor];
    self.window.title = @"MyBareMetalApp";
    */
#else

    //on ios >= 13.0 we'll defer window creation to the scene view delegate methods
    self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    self.window.backgroundColor = [UIColor blueColor];
    
    /*
    NSBundle *frameworkBundle = [NSBundle bundleWithIdentifier:@"com.apple.GraphicsServices"];
    const char *frameworkPath = [[frameworkBundle executablePath] UTF8String];
    if (frameworkPath) {
        void *graphicsServices = dlopen(frameworkPath, RTLD_NOLOAD | RTLD_LAZY);
        if (graphicsServices) {
            BOOL (*GSFontAddFromFile)(const char *) = dlsym(graphicsServices, "GSFontAddFromFile");
            if (GSFontAddFromFile) {
                
                NSMutableDictionary *themeFonts = [[NSMutableDictionary alloc] init];
                NSString *fontFileName = nil; NSString *fontFilePath = nil;
                
                //add primary font
                //[self findNameOfFontFileWithType:PrimaryFont fontFileName:&fontFileName fontFilePath:&fontFilePath];
                //if (fontFilePath && fontFileName) {
                    //NSString *path = [NSString stringWithFormat:@"%@/%@", fontFilePath, fontFileName];
                    //newFontCount += GSFontAddFromFile([path UTF8String]);
                    //[themeFonts setObject:[fontFileName removeExtension] forKey:IFEPrimaryFontKey];
                    //[fontFilePath release];
                    //[fontFileName release];
               // }
            }
        }
    }
    */
#endif
    }
}

-(void)createRootViewController
{
    
#if TARGET_OS_OSX
    //if you set the pointer to nil here and no other strong references to the camera view controller are in memory
    //the object will deallocate itself, so by using this line you can run this function over and over to recreate your root view controller
    //and navigation stack if needed
#else
    self.rootViewController = nil;
    
    //create the camera video view controller
    self.rootViewController = [[CTViewController alloc] init];
    self.rootViewController.view.backgroundColor = [UIColor purpleColor];
    self.window.rootViewController = self.rootViewController;
#endif
    
}
#pragma mark -- OSX Delegate Methods

#if TARGET_OS_OSX
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
    //Hold up NSApplication::terminate: from being called so we can notify CoreTransport to clean up and it can notify NSApplication when finished so that shutdown can proceed

    struct kevent kev;
    EV_SET(&kev, CTProcessEvent_Exit, EVFILT_USER, EV_ADD | EV_ENABLE | EV_ONESHOT, NOTE_FFCOPY|NOTE_TRIGGER|0x1, 0, NULL);
    kevent(CTProcessEventQueue.kq, &kev, 1, NULL, 0, NULL);
    
    return NSTerminateLater;
    
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    // Insert code here to initialize your application
    
    //Customize Menu Title
    //[self setMenuTitle:@"Core Transport"];

    //[[NSApplication sharedApplication] stop];
    //[NSRunLoop mainRunLoop]
    
    
}


- (void)applicationWillTerminate:(NSNotification *)aNotification {
    // Insert code here to tear down your application
    NSLog(@"ApplicationWillTerminate");
    
    
}


- (BOOL)applicationSupportsSecureRestorableState:(NSApplication *)app {
    return YES;
}

#else



- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    // Override point for customization after application launch.

    /*
     #if TARGET_OS_IOS || (TARGET_OS_IPHONE && !TARGET_OS_TV)
     // iOS-specific code
     //begin generating device orientation updates immediately at startup
     [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
     #elif TARGET_OS_TV
     // tvOS-specific code
     #endif
     */
    
    [[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];

if( [[UIDevice currentDevice].systemVersion floatValue] >= 13.0 )
{
    
}
else
{
    [self createWindow];
    [self createRootViewController];
}
    
    /*
    NSBundle *frameworkBundle = [NSBundle bundleWithIdentifier:@"com.apple.GraphicsServices"];
    const char *frameworkPath = [[frameworkBundle executablePath] UTF8String];
    if (frameworkPath) {
        void *graphicsServices = dlopen(frameworkPath, RTLD_NOLOAD | RTLD_LAZY);
        if (graphicsServices) {
            BOOL (*GSFontAddFromFile)(const char *) = dlsym(graphicsServices, "GSFontAddFromFile");
            if (GSFontAddFromFile) {
                
                NSMutableDictionary *themeFonts = [[NSMutableDictionary alloc] init];
                NSString *fontFileName = nil; NSString *fontFilePath = nil;
                
                //add primary font
                //[self findNameOfFontFileWithType:PrimaryFont fontFileName:&fontFileName fontFilePath:&fontFilePath];
                //if (fontFilePath && fontFileName) {
                    //NSString *path = [NSString stringWithFormat:@"%@/%@", fontFilePath, fontFileName];
                    //newFontCount += GSFontAddFromFile([path UTF8String]);
                    //[themeFonts setObject:[fontFileName removeExtension] forKey:IFEPrimaryFontKey];
                    //[fontFilePath release];
                    //[fontFileName release];
               // }
            }
        }
    }
    */
    self.window.rootViewController = self.rootViewController;
    [self.window makeKeyAndVisible];

    //NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    //NSLog(@"Documents DIR: %@", [paths objectAtIndex:0]);
    
    /*
    //notify our Core Render Application Event Loop that the Cocoa Touch application has finished launching
    struct kevent kev;
    EV_SET(&kev, crevent_init, EVFILT_USER, 0, NOTE_TRIGGER, 0, NULL);
    kevent(cr_appEventQueue, &kev, 1, NULL, 0, NULL);
    */
    
    return YES;
}



#pragma mark - UISceneSession lifecycle

- (UISceneConfiguration *)application:(UIApplication *)application configurationForConnectingSceneSession:(UISceneSession *)connectingSceneSession options:(UISceneConnectionOptions *)options  API_AVAILABLE(ios(13.0)){
    // Called when a new scene session is being created.
    // Use this method to select a configuration to create the new scene with.
    return [[UISceneConfiguration alloc] initWithName:@"Default Configuration" sessionRole:connectingSceneSession.role];
}


- (void)application:(UIApplication *)application didDiscardSceneSessions:(NSSet<UISceneSession *> *)sceneSessions  API_AVAILABLE(ios(13.0)){
    // Called when the user discards a scene session.
    // If any sessions were discarded while the application was not running, this will be called shortly after application:didFinishLaunchingWithOptions.
    // Use this method to release any resources that were specific to the discarded scenes, as they will not return.
}


#pragma mark - UISceneDelegate methods

- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)connectionOptions  API_AVAILABLE(ios(13.0)){
    // Use this method to optionally configure and attach the UIWindow `window` to the provided UIWindowScene `scene`.
    // If using a storyboard, the `window` property will automatically be initialized and attached to the scene.
    // This delegate does not imply the connecting scene or session are new (see `application:configurationForConnectingSceneSession` instead).

    UIWindowScene * windowScene = [[UIWindowScene alloc] initWithSession:session connectionOptions:connectionOptions];
    if( !windowScene ) return;
    
    //self.window = [[UIWindow alloc] initWithFrame:windowScene.coordinateSpace.bounds];
    self.window.windowScene = windowScene;
    self.window = [[UIWindow alloc] initWithWindowScene:windowScene];
    
    self.window.rootViewController = [[UIViewController alloc] init];
    self.window.rootViewController.view.backgroundColor = [UIColor purpleColor];
    [self.window makeKeyAndVisible];
}

- (void)sceneDidDisconnect:(UIScene *)scene  API_AVAILABLE(ios(13.0)){
    // Called as the scene is being released by the system.
    // This occurs shortly after the scene enters the background, or when its session is discarded.
    // Release any resources associated with this scene that can be re-created the next time the scene connects.
    // The scene may re-connect later, as its session was not necessarily discarded (see `application:didDiscardSceneSessions` instead).
}


- (void)sceneDidBecomeActive:(UIScene *)scene  API_AVAILABLE(ios(13.0)){
    // Called when the scene has moved from an inactive state to an active state.
    // Use this method to restart any tasks that were paused (or not yet started) when the scene was inactive.
}


- (void)sceneWillResignActive:(UIScene *)scene  API_AVAILABLE(ios(13.0)){
    // Called when the scene will move from an active state to an inactive state.
    // This may occur due to temporary interruptions (ex. an incoming phone call).
}


- (void)sceneWillEnterForeground:(UIScene *)scene  API_AVAILABLE(ios(13.0)){
    // Called as the scene transitions from the background to the foreground.
    // Use this method to undo the changes made on entering the background.
}


- (void)sceneDidEnterBackground:(UIScene *)scene  API_AVAILABLE(ios(13.0)){
    // Called as the scene transitions from the foreground to the background.
    // Use this method to save data, release shared resources, and store enough scene-specific state information
    // to restore the scene back to its current state.
}


#endif


@end
