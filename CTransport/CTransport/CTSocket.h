
#ifndef CTSOCKET_H
#define CTSOCKET_H

#ifdef CTRANSPORT_USE_MBED_TLS
#include "mbedtls/net.h"
#endif

//Socket Includes
#ifndef _WIN32
#include <sys/event.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h> //TCP_NODELAY sockopt
#include <arpa/inet.h>
#include <unistd.h>

//create an alias for ioctl a la Win32
#ifndef ioctlsocket
#define ioctlsocket ioctl
#endif

#else
//#include <windows.h>
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>

// #######################################################################
// ############ DEFINITIONS
// #######################################################################
#define OBJ_EXCLUSIVE           0x00000020L
#define OBJ_KERNEL_HANDLE       0x00000200L
#define FILE_NON_DIRECTORY_FILE 0x00000040

typedef LONG NTSTATUS;

typedef struct _IO_STATUS_BLOCK {
	union {
		NTSTATUS Status;
		PVOID Pointer;
	};
	ULONG_PTR Information;
} IO_STATUS_BLOCK, * PIO_STATUS_BLOCK;

typedef struct _FILE_COMPLETION_INFORMATION {
	HANDLE Port;
	PVOID  Key;
} FILE_COMPLETION_INFORMATION, * PFILE_COMPLETION_INFORMATION;

typedef enum _FILE_INFORMATION_CLASS {
	FileBasicInformation = 4,
	FileStandardInformation = 5,
	FilePositionInformation = 14,
	FileEndOfFileInformation = 20,
	FileReplaceCompletionInformation = 61,
} FILE_INFORMATION_CLASS, * PFILE_INFORMATION_CLASS;

typedef struct _FILE_BASIC_INFORMATION {
	LARGE_INTEGER CreationTime;							// Created             
	LARGE_INTEGER LastAccessTime;                       // Accessed    
	LARGE_INTEGER LastWriteTime;                        // Modifed
	LARGE_INTEGER ChangeTime;                           // Entry Modified
	ULONG FileAttributes;
} FILE_BASIC_INFORMATION, * PFILE_BASIC_INFORMATION;

typedef NTSTATUS(WINAPI* pNtQueryInformationFile)(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS);
typedef NTSTATUS(WINAPI* pNtSetInformationFile)(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG, FILE_INFORMATION_CLASS);


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#endif

//#include "ReqlCoroutine.h"
#include "CTError.h"
//Define REQL_SOCKET
#ifdef _WIN32
#define CTSocket 			SOCKET
#define CTThread			HANDLE
#define CTKernelQueue 		HANDLE
#define CTDispatchSource 	void
//#define SSLContextRef 	void*
#define CTSocketError() (WSAGetLastError())
#elif defined(__APPLE__) || defined(__FreeBSD__) //with libdispatch
#define CTSocket 	  		int //sockets are just file descriptors
#define CTThread			pthread_t
#define CTKernelQueue 		int //kqueues are just file descriptors
#define CT_INCOMING_PIPE	0
#define CT_OUTGOING_PIPE	1
#define CTDispatchSource 	dispatch_source_t
typedef void (^CTDispatchSourceHandler)(void);
#define CTSocketError() (errno)
#else
#error "Unsupported Platform"
#endif

typedef struct CTKernelQueuePipePair
{
    CTKernelQueue q;
	CTKernelQueue p[2];
}CTKernelQueuePipePair;

extern const uint32_t  CTSOCKET_MAX_TIMEOUT; //input to kqueue to indicate we want to wait forever
extern const uintptr_t CTSOCKET_EVT_TIMEOUT; //custom return value from kqueue wait to indicate we received timeout from kqueue

typedef struct CTSocketContext
{

#ifdef CTRANSPORT_USE_MBED_TLS
	union {
		CTSocket socket;
		mbedtls_net_context ctx; //TO DO: consider putting wolfssl socket context here?
	};
#else
	CTSocket socket;
#endif
	CTKernelQueue			cxQueue;		//
	CTKernelQueue			txQueue;		//an WIN32 iocp completion port
	CTKernelQueue			rxQueue;
#ifndef _WIN32
	CTKernelQueue			oxPipe[2];	//The desired kernel queue for the socket connection to post overlapped cursors to the rxQueue + thread pool
#endif

	char * host;
}CTSocketContext;
typedef CTSocketContext* CTSocketContextRef;

//#pragma mark -- CTKernelQueue Event API
CTRANSPORT_API CTRANSPORT_INLINE int kqueue_wait_with_timeout(int kqueue, struct kevent * kev, int numEvents, uint32_t timeout);

CTRANSPORT_API CTRANSPORT_INLINE CTKernelQueue CTKernelQueueCreate(void);

/***
 *  Creates rx and tx kernel queues and associates them with the socket for reading and writing
 *	(Not used in practice; only for development)
 ***/
CTRANSPORT_API CTRANSPORT_INLINE CTKernelQueue CTSocketCreateEventQueue(CTSocketContext * socketContext);

/**
 *  CTKernelQueueWait
 *
 *  Wait for socket event on provided input kqueue event_queue
 *
 *  Returns the resulting kqueue event which should match socketfd on success,
 *  timeout or out of range otherwise
 *
 *  TO DO:  Add timeout input
 ***/
CTRANSPORT_API CTRANSPORT_INLINE coroutine int CTKernelQueueWait(CTKernelQueue event_queue, int16_t eventFilter);

//#pragma mark -- CTThread API

CTRANSPORT_API CTRANSPORT_INLINE CTThread CTThreadCreate(CTKernelQueue* threadPoolQueue, LPTHREAD_START_ROUTINE threadRoutine);
CTRANSPORT_API CTRANSPORT_INLINE void CTThreadClose(CTThread* thread);

//#pragma mark -- CTSocket API

/***
 *	CTSocketInit
 *
 *	Load Socket Libraries on WIN32 platforms
 ***/
CTRANSPORT_API CTRANSPORT_INLINE void CTSocketInit(void);
CTRANSPORT_API CTRANSPORT_INLINE int  CTSocketGetError(CTSocket socketfd);

CTRANSPORT_API CTRANSPORT_INLINE CTSocket CTSocketCreate(int nonblocking);
CTRANSPORT_API CTRANSPORT_INLINE int 	  CTSocketClose(CTSocket socketfd);


#endif