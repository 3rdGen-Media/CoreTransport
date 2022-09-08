//
//  CTApp.h
//  CRViewer
//
//  Created by Joe Moulton on 1/26/19.
//  Copyright © 2019 Abstract Embedded. All rights reserved.
//

#ifndef CTAppInterface_h
#define CTAppInterface_h

//----------------------------------------------------PREPROCESSOR----------------------------------------------------------------------//

//#ifndef __BLOCKS__
//#error must be compiled with -fblocks option enabled
//#endif

//------------------------------------------------------HEADERS------------------------------------------------------------------------//

#pragma mark -- Platform C Headers

#ifdef _WIN32

#ifdef _DEBUG
#include <vld.h>              //Visual Leak Detector
#endif

#include "stdio.h"
#include "stdlib.h"
#include <process.h>          // threading routines for the CRT
#include "assert.h"

#pragma comment(lib, "crypt32.lib") //link BCRYPT API
#pragma comment(lib, "secur32.lib") //link SCHANNEL API

#elif !defined(__APPLE__)

#include <Block.h>      //Clang Blocks Runtime

#include <sys/thr.h>    //FreeBSD threads
#include "/usr/include/sys/thr.h"

#include <pthread.h>    //Unix pthreads
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#elif defined(__APPLE__)
#include <TargetConditionals.h>

#include <CoreFoundation/CoreFoundation.h>           //Core Foundation
#include <objc/runtime.h>                            //objective-c runtime
#include <objc/message.h>                            //objective-c runtime message

#if TARGET_OS_OSX
#include <ApplicationServices/ApplicationServices.h> //Cocoa
#endif
#endif


#pragma mark -- Platform Obj-C Headers

#if defined(__APPLE__) && defined(__OBJC__)  //APPLE w/ Objective-C Main
#if TARGET_OS_OSX           //MAC OSX
#import  <AppKit/AppKit.h>
#else                       //IOS + TVOS
#import  <UIKit/UIKit.h>
#endif
#endif

#pragma mark -- Standard C Headers

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "float.h"
#include "limits.h"


#pragma mark -- Application Obj-C Headers

#if defined(__APPLE__) && defined(__OBJC__)  //APPLE w/ Objective-C Main
#import "CTApplication.h"
#endif

#pragma mark -- Application 3rd Party Headers

#ifdef __cplusplus //|| defined(__OBJC__)
//CoreTransport CXURL and CXReQL C++ APIs use CTransport (CTConnection) API internally
#include <CoreTransport/CXURL.h>
#include <CoreTransport/CXReQL.h>
#elif defined(__OBJC__)
#include <CoreTransport/NSTURL.h>
#include <CoreTransport/NSTReQL.h>
#else
#include "CoreTransport/CTransport.h"
#endif

//------------------------------------------------------Linker Defines--------------------------------------------------------------------//

#pragma mark -- Obj-C Runtime Externs

#if defined(__APPLE__) && !defined(__OBJC__)
//define the NSApplicationMain objective-c runtime call to suppress warnings before linking
#if TARGET_OS_OSX
//extern int NSApplicationMain(int argc, const char *__nonnull argv[__nonnull]);
extern int NSApplicationMain(int argc, const char *_Nonnull argv[_Nonnull]);
//extern int NSApplicationMain(int argc, const char *__nonnull argv[__nonnull], id principalClassName, id delegateClassName);
#else// defined(CR_TARGET_IOS) || defined(CR_TARGET_TVOS)
//define the UIApplicationMain objective-c runtime call or linking will fail
extern int UIApplicationMain(int argc, char * _Nullable argv[_Nonnull], id _Nullable principalClassName, id delegateClassName);
//extern int UIApplicationMain(int argc, char * _Nullable argv[_Nonnull], NSString * _Nullable principalClassName, NSString * _Nullable delegateClassName);
#endif
#endif

//------------------------------------------------------App Definitions--------------------------------------------------------------------//

#pragma mark -- Application Globals

//Class Names of the Custom NSApplication/UIApplication and NSAppDelegate/UIAppDelegate Cocoa App Singleton Objective-C Objects
static const char * _Nonnull CocoaAppClassName         = "CTApplication";
static const char * _Nonnull CocoaAppDelegateClassName = "CTApplicationDelegate";

#pragma mark -- Process Names


#pragma mark -- Appication Events

/*
//example application event loop kernel queue events
typedef enum ctevent_type {
    ctevent_init,
    ctevent_exit,                   //we received a message to shutdown the applicaition
    ctevent_menu,                   //we received an event as a result of user clicking the menu
    ctevent_register_view,          //register a platform created view/layer backed by an accelerated gl context to render with Core Render
    ctevent_main_window_changed,    //the application event loop was notifi
    ctevent_platform_event,         //the application event loop received a platform event from the platform main event loop (or one of the window control threads, less likely)
    ctevent_graphics,
    ctevent_idle,
    ctevent_timeout,
    ctevent_out_of_range
}ctevent_type;
    
typedef enum ctmenu_event
{
    ctmenu_quit,
    ctmenu_close_window,
    ctmenu_toggle_fullscreen
}ctmenu_event;
*/

#pragma mark -- Application Thread Handles

//These are provided for you for convenience in crPlatform lib
//cr_platform_thread cr_mainThread;
//cr_platform_thread_id cr_mainThreadID;

#pragma mark -- Event Queue Handles


#pragma mark -- Event Handles

//This event gets created by the application, but it's handle lives in crPlatform lib
//so that it can be shared between Cocoa and Core Render, or any two processes that load the same crPlatform lib
//static cr_kernel_queue_event cr_vBlankNotification;

#pragma mark -- Custom Dispatch Queues
//define strings for dispatch queues
//static const char kCRWindowEventQueue[] = "cr.3rdgen.windowEventQueue";
//static dispatch_queue_t g_windowEventQueue[NUM_VIEWS];

#pragma mark -- Custom Run Loop Modes
//define strings for custom run loop modes
//A global event queue for receiving event updates in Core Render C-Land from
//define a custom queue that will act as the interface between
//static const CFStringRef kCFCGEventReceiveRunLoopMode = CFSTR("CGEventReceiveRunLoopMode");

#pragma mark -- Event Handlers

//break out the functions that handle OS main run loop and platform window system provided event messages
//into a separate header, as these functions can get rather large
//#include "PlatformEventHandlers.h"

#pragma mark -- Render Handlers

//break out the functions that handle rendering to the crgc_view platform window
//into a separate header per Graphics API
//#include "RenderGL.h"

#pragma mark -- Define DisplaySync Callback Struct
//define a displaySync object that can be used to slave rendering to platform kernel display updates each view
//we declare globablly so the object reference can persist when its pointer is passed through the callback mechanism
//CRDisplaySyncCallbackRef g_dispSyncCallbackRef[NUM_VIEWS];

#pragma mark -- Timing Globals
//static uint64_t timeA, timeB;

#pragma mark -- CoreTransport CTTarget Credentials

static const char*          http_server  = "3rdgen-sandbox-html-resources.s3.us-west-1.amazonaws.com";//"example.com";// "vtransport - assets.s3.us - west - 1.amazonaws.com";//"example.com";//"mineralism.s3 - us - west - 2.amazonaws.com";
static const unsigned short http_port    = 443;

//Proxies use a prefix to specify the proxy protocol, defaulting to HTTP Proxy
static const char*          proxy_server = "http://172.20.10.1";//"54.241.100.168";
static const unsigned short proxy_port   = 443;

//Define a CTransport API ReqlService (ie CTTarget) C style struct to initiate a RethinkDB TLS connection with a CTransport API CTConnection
static const char*          rdb_server   = "rdb.server.net";
static const unsigned short rdb_port     = 28015;
static const char*          rdb_user     = "rdb_username";
static const char*          rdb_pass     = "rdb_password";

//These are only relevant for custom DNS resolution libdill, loading the data from these files has not yet been implemented for WIN32 platforms
static char resolvConfPath[1024]         = "../Samples/HappyEyeballs/Resources/Keys/resolv.conf";
static char nsswitchConfPath[1024]       = "../Samples/HappyEyeballs/Resources/Keys/nsswitch.conf";

//Define SSL Certificate for ReqlService construction:
//    a)  For xplatform MBEDTLS support:    The SSL certificate can be defined in a buffer OR a file AS a .cer in PEM format
//    b)  For Win32 SCHANNEL support:       The SSL certificate must be pre-installed in a trusted root store on the system using MS .cer format
//    c)  For Darwin SecureTransport:       The SSL certificate must be defined in a .der file with ... format
//    d)  For xplatform WolfSSL support: ...
static char certPath[1024]               = "./certificate.der";
//const char * url_cert                  = "URL_CERT_PATH";
//const char * rdb_cert                  = "RDB_CERT_APTH";

#pragma mark -- Define CTConnections for CTApp to use

#ifdef __cplusplus
using namespace CoreTransport;

//Define pointers to keep reference to our secure HTTPS and RethinkDB connections, respectively
static CXConnection* _httpConn = NULL;
static CXConnection* _reqlConn = NULL;
#elif defined(__OBJC__)
static NSTConnection* _httpConn = NULL;
static NSTConnection* _reqlConn = NULL;
#else
static CTConnection   _httpConn;
static CTConnection   _reqlConn;
#endif

//Define Application Wide CoreTransport Connection and Cursors Pools
#define CT_APP_MAX_INFLIGHT_CONNECTIONS 1024
#define CT_APP_MAX_INFLIGHT_CURSORS 1024
static CTConnection CT_APP_CONNECTIONS[CT_APP_MAX_INFLIGHT_CONNECTIONS];
static CTCursor     CT_APP_CURSORS[CT_APP_MAX_INFLIGHT_CONNECTIONS];

#endif /* CTAppInterface_h */
