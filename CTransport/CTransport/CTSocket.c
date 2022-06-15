//#include "stdafx.h"
#include "../CTransport.h"

//Winsock Library TO DO:  move these globals elsewhere?
#ifdef _WIN32
WSADATA g_WsaData;
static int g_WsaInitialized = 0;
#else
#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif
#endif 

const uint32_t  CTSOCKET_MAX_TIMEOUT = UINT_MAX;	//input to kqueue to indicate we want to wait forever
const uintptr_t CTSOCKET_EVT_TIMEOUT = UINTPTR_MAX;	//custom return value from kqueue wait to indicate we received timeout from kqueue

#ifndef _WIN32
int kqueue_wait_with_timeout(int kqueue, struct kevent * kev, int numEvents, uint32_t timeout)
{
    struct timespec _ts;
    struct timespec *ts = NULL;
    if (timeout != UINT_MAX) {
        ts = &_ts;
        ts->tv_sec = 0;//(timeout - (timeout % 1000)) / 1000;
        ts->tv_nsec = (timeout /*% 1000*/);// * 1000;
    }
    
    //kev->filter = eventFilter;
    //struct kevent kev;
    return kevent(kqueue, NULL, 0, kev, numEvents, ts);
}
#endif

CTKernelQueue CTKernelQueueCreate(void)
{
#ifndef _WIN32
    return kqueue();
#else
    return CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, (ULONG_PTR)NULL, 0);
#endif
}


#ifdef _WIN32
//ULONG _ioCompletionKey = 0;//NULL;
ULONG_PTR dwCompletionKey = (ULONG_PTR)NULL;
#endif 
/***
 *
 *
 ***/
CTKernelQueue CTSocketCreateEventQueue(CTSocketContext * socketContext)
{
	CTKernelQueue event_queue = -1;
#if defined( __APPLE__ ) || defined(__FreeBSD__)
	//  Create a kqueue for managing socket events
    event_queue = CTKernelQueueCreate();
    
    //  Register kevents for socket connection notification
    struct kevent kev;
    void * context_ptr = NULL; //we can pass a context ptr around such as SSLContextRef or Re�MQL*, to retrieve when kevents come in on other threads
    EV_SET(&kev, socketContext->socket, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, context_ptr);
    kevent(event_queue, &kev, 1, NULL, 0, NULL);
    EV_SET(&kev, socketContext->socket, EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, context_ptr);
    kevent(event_queue, &kev, 1, NULL, 0, NULL);

	//TO DO:  Create GCD tx/rx queues
	//socketContext->txQueue = dispatch_queue_create(
	//socketContext->rxQueue = dispatch_queue_create(
#elif defined(_WIN32)

	//TO DO:  Create system event for listening to socket events

	//TO DO:  Create tx/rx queue overlapped send/recv events here

	//Create Transmit/Receive I/O Completion Ports for Windows Send/Recv Thread Queues
	socketContext->txQueue = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, dwCompletionKey, 0);
	socketContext->rxQueue = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, dwCompletionKey, 0);
	CreateIoCompletionPort((HANDLE)socketContext->socket, socketContext->rxQueue, dwCompletionKey, 1);
	//CreateIoCompletionPort((HANDLE)socketContext->socket, socketContext->txQueue, 0, 0);
	event_queue = 0;
#endif
	return event_queue;
}

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
coroutine int CTKernelQueueWait(CTKernelQueue event_queue, int16_t eventFilter)
{
#ifndef _WIN32
    //fprintf(stderr, "CTSocketWait Start\n");
    struct kevent kev = {0};
    kev.filter = eventFilter;
    //  Synchronously wait for the socket event by idling until we receive kevent from our kqueue
    int ret = kqueue_wait_with_timeout(event_queue, &kev, 1, CTSOCKET_MAX_TIMEOUT);
    fprintf(stderr, "CTConnectRoutine::CTKernelQueueWait ret (%d)\n", ret);
    if ( ret != 1 )
    {
        if( ret == 0 )
            fprintf(stderr, "CTKernelQueueWait timeout\n");
        else
            fprintf(stderr, "CTKernelQueueWait failed with error (%d)\n", errno);
    }
    return ret;
#else
	return event_queue;
#endif
	//return ret_event;
}


//#pragma mark -- CTSocket API

void CTSocketInit(void)
{
#ifdef _WIN32
	if( !g_WsaInitialized )
	{
		// Initialize the WinSock subsystem.
		if(WSAStartup(MAKEWORD(2, 2)/*0x0101*/, &g_WsaData) == SOCKET_ERROR) // Winsock.h
		{ fprintf(stderr, "Error %d returned by WSAStartup\n", GetLastError()); }// goto cleanup; } //
		fprintf(stderr, "----- WinSock Initialized\n");
		g_WsaInitialized = 1;
	}
	else
		fprintf(stderr, "----- WinSock Alrleady Initialized!\n");
#elif defined(__FreeBSD__)

#else //__APPLE__

#endif
}

CTSocket CTSocketCreateUDP(void)
{
    // Standard unix socket definitions
    CTSocket socketfd;
    struct linger lin;
    int iResult;
    u_long nonblockingMode = 0;
    struct timeval timeout = { 0,0 };

#ifdef _WIN32
    BOOL     bOptVal = TRUE;
    int      bOptLen = sizeof(BOOL);
    socketfd = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP , NULL, 0, WSA_FLAG_OVERLAPPED);
#elif defined(__APPLE__)

#endif

    // Create the non-blocking unix socket file descriptor for a tcp stream
    if (socketfd < 0 || socketfd == INVALID_SOCKET)
    {
        fprintf(stderr, "socket(PF_INET, SOCK_STREAM, IPPROTO_TCP) failed with error:  %d", errno);
        return CTSocketError;
    }

    // Set linger explicitly off to kill connections on close
    // Note:  linger failure does not explicilty fail the connection
    /*
    lin.l_onoff = 0;
    lin.l_linger = 0;
    if ((setsockopt(socketfd, SOL_SOCKET, SO_LINGER, (const char*)&lin, sizeof(int))) < 0)
    {
        fprintf(stderr, "setsockopt(socketfd, SOL_SOCKET, SO_LINGER,...) failed with error:  %d\n", errno);
        //return CTSocketError;
    }

    //Disable Nagle
    if ((setsockopt(socketfd, IPPROTO_TCP, TCP_NODELAY, (const char*)&bOptVal, bOptLen)) < 0)
    {
        fprintf(stderr, "setsockopt(socketfd, IPPROTO_TCP, TCP_NODELAY,...) failed with error:  %d\n", errno);
        //return CTSocketError;
    }

    //Send Keep alive messages automatically
    if ((setsockopt(socketfd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&bOptVal, bOptLen)) < 0)
    {
        fprintf(stderr, "setsockopt(socketfd, SOL_SOCKET, SO_LINGER,...) failed with error:  %d\n", errno);
        //return CTSocketError;
    }
    */

    return socketfd;
}

//A helper function to create a socket
CTSocket CTSocketCreate(int nonblocking)
{
    // Standard unix socket definitions
    CTSocket socketfd;
	struct linger lin;
	int iResult;

    //CTransport always uses non-blocking sockets, yuh
    u_long nonblockingMode = (u_long)nonblocking;
    struct timeval timeout = { 0,0 };

#ifdef _WIN32
    BOOL     bOptVal = TRUE;
    int      bOptLen = sizeof(BOOL);
	socketfd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
#else //defined(__APPLE__)
    bool     bOptVal = true;
    int      bOptLen = sizeof(bool);
	socketfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif
	// Create the non-blocking unix socket file descriptor for a tcp stream
    if(  socketfd < 0 || socketfd == INVALID_SOCKET )
    {
        fprintf(stderr, "socket(PF_INET, SOCK_STREAM, IPPROTO_TCP) [fd=%d] failed with error:  %d", socketfd, errno);
        return CTSocketError;
    }

    //-------------------------
    // Set the socket I/O mode: In this case FIONBIO
    // enables or disables the blocking mode for the 
    // socket based on the numerical value of iMode.
    // If iMode = 0, blocking is enabled; 
    // If iMode != 0, non-blocking mode is enabled.

    iResult = ioctlsocket(socketfd, FIONBIO, &nonblockingMode);
    if (iResult != 0)
        fprintf(stderr, "ioctlsocket failed with error: %d\n", iResult);

    // Set linger explicitly off to kill connections on close
    // Note:  linger failure does not explicilty fail the connection

    lin.l_onoff = 0;
    lin.l_linger = 0;
    if( (setsockopt(socketfd, SOL_SOCKET, SO_LINGER, (const char *)&lin, sizeof(int))) < 0 )
    {
        fprintf(stderr, "setsockopt(socketfd, SOL_SOCKET, SO_LINGER,...) failed with error:  %d\n", errno);
        //return CTSocketError;
    }

    //Disable Nagle
    if ((setsockopt(socketfd, IPPROTO_TCP, TCP_NODELAY, (const char*)&bOptVal, bOptLen)) < 0)
    {
        fprintf(stderr, "setsockopt(socketfd, IPPROTO_TCP, TCP_NODELAY,...) failed with error:  %d\n", errno);
        //return CTSocketError;
    }

    //Send Keep alive messages automatically
    if ((setsockopt(socketfd, SOL_SOCKET, SO_KEEPALIVE, (const char*)&bOptVal, bOptLen)) < 0)
    {
        fprintf(stderr, "setsockopt(socketfd, SOL_SOCKET, SO_LINGER,...) failed with error:  %d\n", errno);
        //return CTSocketError;
    }


   // timeout.tv_sec = 0;
   // timeout.tv_usec = 0;
	/*
    if (setsockopt (socketfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        fprintf(stderr, "setsockopt(socketfd, SOL_SOCKET, SO_RCVTIMEO,...) failed with error:  %d\n", errno);

    if (setsockopt (socketfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        fprintf(stderr, "setsockopt(socketfd, SOL_SOCKET, SO_SNDTIMEO,...) failed with error:  %d\n", errno);
	*/
	/*
	 iResult = ioctlsocket(socketfd, FIONBIO, &nonblockingMode );
	if (iResult != NO_ERROR)
		fprintf(stderr, "ioctlsocket failed with error: %ld\n", iResult);
  */
#ifndef _WIN32
    // Set socket to close on exec
    /*
    if( (fcntl(socketfd, F_SETFD, FD_CLOEXEC)) < 0 )
    {
        fprintf(stderr, "fcntl(socketfd, F_SETFD, FD_CLOEXEC) failed with error:  %d\n", errno);
        return CTSocketError;
    }
    */
#endif
    // Set socket tot nonblocking (important!)
    /*
    int flags = fcntl(socketfd, F_GETFL);
    if( (fcntl(socketfd, F_SETFL, flags | O_NONBLOCK)) < 0 )
    {
        fprintf(stderr, "fcntl(socketfd, F_SETFL, flags | O_NONBLOCK) failed with error:  %d\n", errno);
        return CTSocketError;
    }
     */
    // Return the successfully created socket descriptor
    return socketfd;
}

#ifndef socklen_t
typedef unsigned int socklen_t;
#endif
/**
 *  CTSocketGetError
 *
 *  Wait for socket event on provided input kqueue event_queue
 ***/
int CTSocketGetError(CTSocket socketfd)
{
    //fprintf(stderr, "ReqlGetSocketError\n");
    int result = 0;
    //get the result of the socket connection
    socklen_t result_len = sizeof(result);
    if ( getsockopt(socketfd, SOL_SOCKET, SO_ERROR, (char*)&result, &result_len) != 0 ) 
	{
        // error, fail somehow, close socket
        fprintf(stderr, "getsockopt(socketfd, SOL_SOCKET, SO_ERROR,...) failed with error %d\n", errno);
        return result;//ReqlSysCallError;
    }
    return result;
}

int CTSocketClose(CTSocket socketfd)
{
#ifdef _WIN32
	return closesocket(socketfd);
#else
    return close( socketfd );
#endif
}


