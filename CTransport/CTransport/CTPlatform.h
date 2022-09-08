#ifndef CTPLATFORM_H
#define CTPLATFORM_H


#ifdef __cplusplus
extern "C" {
#endif
    
/*
#if defined(_WIN32)         //MICROSOFT WINDOWS PLATFORMS
#define CR_TARGET_WIN32     1
#elif defined( __APPLE__ )  //APPLE PLATFORMS
#if defined(TARGET_OS_IOS) && TARGET_OS_IOS
#define CR_TARGET_IOS       1
#elif defined(TARGET_OS_TV) && TARGET_OS_TV
#define CR_TARGET_TVOS      1
#elif defined(TARGET_OS_OSX) && TARGET_OS_OSX
#define CR_TARGET_OSX       1
#endif                      //APPLE PLATFORMS
#endif
    
#if defined(__APPLE__)
#include <dispatch/dispatch.h>
#if !TARGET_OS_IOS && !TARGET_OS_TV
#include "cg_private.h"
#endif
#endif
*/

//OSX Platform Threading Mechanism (Grand Central Dispatch thread pools)
 
#pragma mark -- Core Render Process Definitions

//Define an opaque process handle for supported platform process types
#ifdef _WIN32
typedef HANDLE cr_pid_t;
#else //defined(__APPLE__)
typedef pid_t cr_pid_t;
#endif

//global display sync process handle
extern cr_pid_t CTDisplaySyncProcess;

//Define Opaque Priority Class Types for Running System Processes
typedef enum CTPriorityClass
{
    CTPriorityClass_Default  = 0,
    CTPriorityClass_Realtime = 0x00000100  //Win32 equivalent
}CTPriorityClass;

typedef enum CTThreadPriority
{
    CTThreadPriority_Default,
    CTThreadPriority_Critical // = THREAD_PRIORITY_TIME_CRITICAL
}CTThreadPriority;

#pragma mark -- Core Render Platform Kernel Message Definitions

//Define Opaque OS Platform Kernel Message Type Provided by the OS (ie Mach Messages for Darwin)
//#ifdef CR_TARGET_WIN32
//what to put here?
//#elif defined(__APPLE__)
//typedef mach_msg cr_kernel_msg;
//#endif

#pragma mark -- Core Render Platform Run Loop Event Message Type Definitions

//Define Opaque OS Platform Event Loop Message Data Type Provided by the OS Windowing API (ie GDI for Win32 or Core Graphics for Cocoa)
#ifdef CR_TARGET_WIN32
typedef MSG CTPlatformEventMsg;            //an OS defined container for wrapping a system genereated kernel message delivered to an application's main event loop or async window event loop
#elif defined(__APPLE__) && defined(TARGET_OS_OSX) && TARGET_OS_OSX
//typedef CGEventRef CTPlatformEventMsg;        //an OS defined container for wrapping a system genereated kernel message delivered to an application's main event loop or async window event loop
#elif defined(__APPLE__) && defined(TARGET_OS_IOS) && TARGET_OS_IOS
typedef uintptr_t CTPlatformEventMsg;
#endif

//Extend the OS Platform Defined Event Loop Message Enumerations (for defining/handling custom messages platform provided event queue on the main event loop)
#ifndef WM_APP
#define WM_APP 0x8000
#endif
#define CT_PLATFORM_EVENT_MSG_BASE_ID WM_APP
#define CT_PLATFORM_EVENT_MSG_LOOP_QUIT            CR_PLATFORM_EVENT_MSG_BASE_ID + 1

// user defined window messages the render thread uses to communicate
//crgc_view render thread messages
/*
#define CR_PLATFORM_WINDOW_EVENT_MSG_BASE_ID        CR_PLATFORM_EVENT_MSG_LOOP_QUIT + 1
#define CR_PLATFORM_WINDOW_EVENT_MSG_PAUSE          CR_PLATFORM_WINDOW_EVENT_MSG_BASE_ID
#define CR_PLATFORM_WINDOW_EVENT_MSG_RESIZE         CR_PLATFORM_WINDOW_EVENT_MSG_BASE_ID + 1
#define CR_PLATFORM_WINDOW_EVENT_MSG_SHOW           CR_PLATFORM_WINDOW_EVENT_MSG_BASE_ID + 2
#define CR_PLATFORM_WINDOW_EVENT_MSG_RECREATE       CR_PLATFORM_WINDOW_EVENT_MSG_BASE_ID + 3
#define CR_PLATFORM_WINDOW_EVENT_MSG_CLOSE          CR_PLATFORM_WINDOW_EVENT_MSG_BASE_ID + 4
*/
//Make "CR_MAIN_EVENT_LOOP" substring synonomous with "CR_PLATFORM_EVENT_LOOP"
#define CT_MAIN_EVENT_LOOP CT_PLATFORM_EVENT_LOOP

#pragma mark -- Core Render Kernel Event Queue

/***
 *  "Kernel Event Queues" are useful for sending/receiving and waiting on events between threads and processes when standard provided event/message queue mechanisms provided by the OS to the application dont cut the mustard)
 *
 *    Core Transport defines the notion of a kernel event queue as some implementation that allows handling of associated "Kernel Events" for the purposes of interprocess and interthread communication without incurring the cost of a User Space->Kernel Space->User Space memory transfer.
 *    Core Transpoort defines the notion of a "Kernel Event" as an event that a process thread may idle on and "woken up" by the kernel to receive the memory associated with the event directly from the kernel.
 *
 *    Note that some platforms may not have a dedicated "Kernel Event Queue" data type or handle exposed but still have "Kernel Event" type that a process thread may idle on.  For example, Win32 thread handles and their associated event queues are one in the same (see comment below).
 *    When no sufficient "Kernel Event" mechanism can be provided by or implemented on top of the OS, a run loop must fall back to polling for events which is not suitable for real-time operation :(
 ***/

#pragma mark -- Core Render Kernel Event Queue Event Enumerations

//example application event loop kernel queue events

typedef enum CTProcessEvent {
    CTProcessEvent_Init,
    CTProcessEvent_Exit,                  //we received a message to shutdown the applicaition
    CTProcessEvent_Menu,                  //we received an event as a result of user clicking the menu
    //CTProcessEvent_RegisterView,        //register a platform created view/layer backed by an accelerated gl context to render with Core Render
    //CTProcessEvent_MainWindowchanged,   //the application event loop was notifi
    CTProcessEvent_PlatformEvent,         //the application event loop received a platform event from the platform main event loop (or one of the window control threads, less likely)
    //CTProcessEvent_Graphics,
    CTProcessEvent_Idle,
    CTProcessEvent_Timeout,
    CTProcessEvent_OutOfRange
}CTProcessEvent;

typedef enum CTPlatformEvent {
    CTPlatformEvent_Init,
    CTPlatformEvent_Exit,                  //we received a message to shutdown the applicaition
    CTPlatformEvent_Menu,                  //we received an event as a result of user clicking the menu
    //CTProcessEvent_RegisterView,        //register a platform created view/layer backed by an accelerated gl context to render with Core Render
    //CTProcessEvent_MainWindowchanged,   //the application event loop was notifi
    CTPlatformEvent_ProcessEvent,         //the application event loop received a platform event from the platform main event loop (or one of the window control threads, less likely)
    //CTProcessEvent_Graphics,
    CTPlatformEvent_Idle,
    CTPlatformEvent_Timeout,
    CTPlatformEvent_OutOfRange
}CTPlatformEvent;
    
//example display sync process/thread kernel queue events
typedef enum CTDisplayEvent {
    CTDisplayEvent_RunningStart,
    CTDisplayEvent_VSync,                 //definitely vblank
    CRDisplayEvent_VBlank_Notification, //might be vblank, might be a premediated vblank notification, depending on the platform
    CRDisplayEvent_NextFrameTime,
    CRDisplayEvent_Idle,
    CRDisplayEvent_Timeout,
    CRDisplayEvent_OutOfRange
}CTDisplayEvent;

typedef enum CTMenuEvent
{
    CTMenuEvent_Quit,
    CTMenuEvent_CloseWindow,
    CTMenuEvent_ToggleFullscreen
}CTMenuEvent;


#pragma mark -- Declare Main Thread Run Loop References

extern CTThread   CTMainThread;
extern CTThreadID CTMainThreadID;

#pragma mark -- Define Global Kernel Queues and corresponding Events (That live in crPlatform memory space)

//define a global vertical retrace notification event suitable for use on all platforms
extern CTKernelQueueEvent CTVBlankNotification;       //a native windows event associated with the display vertical retrace that can be waited on for thread synchronization (A HANDLE is just a void*, a kqueue is just an int file descriptor)

#pragma mark -- Declare Global Event Queues and Pipes (for responding/handling events sent to a threads queue or sending messages to queues between process, respectively)

//#define CR_INCOMING_PIPE 0        //always use the same indexes for reading/writing to a pipe pair for posterity
//#define CR_OUTGOING_PIPE 1        //always use the same indexes for reading/writing to a pipe pair for posterity

//a kqueue singleton designed to that lives in crPlatform shared lib
//so it can be shared and used for kernel level communication between
//threads, processes andn most importantly shared between application frameworks
//such as Cocoa and Core Render

static const char * kCTProcessEventQueueID  = "com.3rdGen.CTransport.ProcessEventQueue";     //an identifier that can be used with cr_mainEventQueue
static const char * kCTPlatformEventQueueID = "com.3rdGen.CTransport.PlatformEventQueue";    //an identifier that can be used with cr_platformEventQueue
static const char * kCTDisplayEventQueueID  = "com.3rdGen.CTransport.DisplayEventQueue";     //an identifier that can be used with cr_DisplayEventQueue
static const char * kCTCoroutineEventQueueID  = "com.3rdGen.CTransport.CoroutineEventQueue";     //an identifier that can be used with cr_DisplayEventQueue

extern CTKernelQueue CTProcessEventQueue;   //a global kqueue singleton for the CoreTransport client application event loop
extern CTKernelQueue CTPlatformEventQueue;  //a global kqueue singleton for injecting events into the platform event loop [if one exists, and it should]
extern CTKernelQueue CTDisplayEventQueue;   //a global kqueue singleton for distributing display updates from a dedicated real-time thread

//a global read/write pipe pair singleton for sending blob messages to to the cr_displayEventQueue (allows waking kqueue from a separate process)
//extern int cr_displayEventPipe[2];
    
#ifdef __cplusplus
}
#endif


#endif //CTPLATFORM_H
