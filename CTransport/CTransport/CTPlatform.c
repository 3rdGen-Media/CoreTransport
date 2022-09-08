//
//  CTPlatform.c
//  CTransport-OSX
//
//  Created by Joe Moulton on 8/27/22.
//


#include "../CTransport.h"

#pragma mark -- Global Process Handles

cr_pid_t cr_displaySyncProcess;

#pragma mark -- Init Main Thread/Run Loop References

CTThread   CTMainThread = NULL;
CTThreadID CTMainThreadID;// = 0;

#pragma mark -- Define Global Kernel Queue Events
CTKernelQueueEvent CTVBlankNotification = {0};

#pragma mark --Define Global Kernel Event Queues/Pipes
CTKernelQueue CTProcessEventQueue  = {0};
CTKernelQueue CTPlatformEventQueue = {0};
CTKernelQueue CTDisplayEventQueue  = {0};  //a global kqueue singleton for distributing display updates from a dedicated real-time thread/process

//a global read/write pipe pair singleton for sending blob messages to to the cr_displayEventQueue (allows waking cr_displayEventQueue from a separate process)
//int cr_displayEventPipe[2];
