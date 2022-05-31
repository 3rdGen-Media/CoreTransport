
#ifndef CTSOCKET_H
#define CTSOCKET_H

#ifdef CTRANSPORT_USE_MBED_TLS
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
#define CTSocket SOCKET
#define CTThreadQueue HANDLE
#define CTDispatchSource void
//#define SSLContextRef void*
#else
#define CTSocket int
#define CTThreadQueue dispatch_queue_t
#define CTDispatchSource dispatch_source_t

#endif

typedef struct CTSocketContext
{

#ifdef CTRANSPORT_USE_MBED_TLS
	union {
		CTSocket socket;
		mbedtls_net_context ctx;
	};
#else
	CTSocket socket;
#endif
	CTThreadQueue			cxQueue;		//
	CTThreadQueue			txQueue;		//an WIN32 iocp completion port
	CTThreadQueue			rxQueue;
	char * host;
}CTSocketContext;

typedef CTSocketContext* CTSocketContextRef;

//#pragma mark -- CTSocket API

/***
 *	CTSocketLInit
 *
 *	Load Socket Libraries on WIN32 platforms
 ***/
CTRANSPORT_API CTRANSPORT_INLINE void CTSocketInit(void);

CTRANSPORT_API CTRANSPORT_INLINE CTSocket CTSocketCreate(void);
CTRANSPORT_API CTRANSPORT_INLINE int CTSocketClose(CTSocket socketfd);

/***
 *
 *
 ***/
CTRANSPORT_API CTRANSPORT_INLINE int CTSocketCreateEventQueue(CTSocketContext * socketContext);

/**
 *  CTSocketWait
 *
 *  Wait for socket event on provided input kqueue event_queue
 *
 *  Returns the resulting kqueue event which should match socketfd on success,
 *  timeout or out of range otherwise
 *
 *  TO DO:  Add timeout input
 ***/
CTRANSPORT_API CTRANSPORT_INLINE coroutine uintptr_t CTSocketWait(CTSocket socketfd, int event_queue, int16_t eventFilter);


CTRANSPORT_API CTRANSPORT_INLINE coroutine int CTSocketGetError(CTSocket socketfd);

#endif