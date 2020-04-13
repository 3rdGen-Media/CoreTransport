
#ifndef ReqlSocket_h
#define ReqlSocket_h

#ifdef REQL_USE_MBED_TLS
#include "mbedtls/net.h"
#endif

//Socket Includes
#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#else
//#include <windows.h>
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#endif

#include "ReqlCoroutine.h"
#include "ReqlError.h"

//Define REQL_SOCKET
#ifdef _WIN32
#define REQL_SOCKET SOCKET
#define ReqlThreadQueue HANDLE
#define ReqlDispatchSource void
//#define SSLContextRef void*
#else
#define REQL_SOCKET int
#define ReqlTheadQueue dispatch_queue_t
#define ReqlDispatchSource dispatch_source_t

#endif

typedef struct ReqlSocketContext
{

#ifdef REQL_USE_MBED_TLS
	union {
		REQL_SOCKET socket;
		mbedtls_net_context ctx;
	};
#else
	REQL_SOCKET socket;
#endif
	ReqlThreadQueue			txQueue;		//an WIN32 iocp completion port
	ReqlThreadQueue			rxQueue;
	//ReqlService * service;
	char * host;
}ReqlSocketContext;

typedef ReqlSocketContext* ReqlSocketContextRef;

#pragma mark -- ReqlSocket API

/***
 *	ReqlSocketLInit
 *
 *	Load Socket Libraries on WIN32 platforms
 ***/
REQL_API REQL_INLINE void ReqlSocketInit(void);

REQL_SOCKET ReqlSocket(void);
int ReqlCloseSocket(REQL_SOCKET socketfd);


/***
 *
 *
 ***/
int ReqlSocketCreateEventQueue(ReqlSocketContext * socketContext);

/**
 *  ReqlSocketWait
 *
 *  Wait for socket event on provided input kqueue event_queue
 *
 *  Returns the resulting kqueue event which should match socketfd on success,
 *  timeout or out of range otherwise
 *
 *  TO DO:  Add timeout input
 ***/
coroutine uintptr_t ReqlSocketWait(REQL_SOCKET socketfd, int event_queue, int16_t eventFilter);


#endif