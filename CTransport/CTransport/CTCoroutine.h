#ifndef CTCOROUTINE_H
#define CTCOROUTINE_H

#ifndef _WIN32
#include "dill/libdill.h"
//dill coroutine handles are actually coroutines bundles that are store as ints
#define CTRoutine int
#define CTRoutineClose hclose
#else
//map coroutine to an empty macro on platforms that don't support it
#ifndef coroutine
#define coroutine
#define go(routine) routine
#define yield()
#define CTRoutine int
#define CTRoutineClose
#endif
#endif

#ifdef __APPLE__

static const CFOptionFlags CF_RUN_LOOP_WAITING = kCFRunLoopBeforeWaiting | kCFRunLoopAfterWaiting;
static const CFOptionFlags CF_RUN_LOOP_RUNNING  = kCFRunLoopAllActivities;

static void CTRoutineRunLoopObserver(CFRunLoopObserverRef observer, CFRunLoopActivity activity, void *info)
{
    //printf("CTRunLoopObserver\n");
    //yield to any pending coroutines and wake up the run loop
    yield();
    CFRunLoopWakeUp(CFRunLoopGetCurrent());
}

static CFRunLoopObserverRef CTRoutineCreateRunLoopObserver(void)
{
    //  Coroutines rely on integration with a run loop [running on a thread] to return operation
    //  to the coroutine when it yields operation back to the calling run loop
    //
    //  For Darwin platforms that either:
    //   1)  A CFRunLoop associated with a main thread in a cocoa app [Cocoa]
    //   2)  A CFRunLoop run from a background thread in a Darwin environment [Core Foundation]
    //   3)  A custom event loop implemented with kqueue [Custom]
    CFRunLoopObserverRef runLoopObserver = NULL;
    
    //Get current run loop
    CFRunLoopRef runLoop = CFRunLoopGetCurrent();
    
    //Create a run loop observer to inject yields from the run loop back to this (and other) coroutines
    // TO DO:  It would be nice to avoid this branching
    CFOptionFlags runLoopActivities = ( runLoop == CFRunLoopGetMain() ) ? CF_RUN_LOOP_WAITING : CF_RUN_LOOP_RUNNING;
    //if ( !(runLoopObserver = CFRunLoopObserverCreate(kCFAllocatorDefault, runLoopActivities, true, 0, &ReqlRunLoopObserver, NULL)) )
    //{
    //    return ReqlRunLoopError;
    //}
    runLoopObserver = CFRunLoopObserverCreate(kCFAllocatorDefault, runLoopActivities, true, 0, &CTRoutineRunLoopObserver, NULL);
    //  Install a repeating run loop observer so we can call coroutine API yield
    if( runLoopObserver ) CFRunLoopAddObserver(runLoop, runLoopObserver, kCFRunLoopDefaultMode);
    return runLoopObserver;
}

#endif


#endif
