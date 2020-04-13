#include "stdafx.h"
#include "../ctReQL.h"

//Winsock Library
WSADATA g_WsaData;
static int g_WsaInitialized = 0;

void ReqlSocketInit(void)
{
	if( !g_WsaInitialized )
	{
		// Initialize the WinSock subsystem.
		if(WSAStartup(MAKEWORD(2, 2)/*0x0101*/, &g_WsaData) == SOCKET_ERROR) // Winsock.h
		{ printf("Error %d returned by WSAStartup\n", GetLastError()); }// goto cleanup; } //
		printf("----- WinSock Initialized\n");
		g_WsaInitialized = 1;
	}
	else
		printf("----- WinSock Alrleady Initialized!\n");

}

//A helper function to create a socket
REQL_SOCKET ReqlSocket(void)
{
    // Standard unix socket definitions
    REQL_SOCKET socketfd;
	struct linger lin;

#ifdef _WIN32
	socketfd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
#elif defined(__APPLE__)
	socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif
	// Create the non-blocking unix socket file descriptor for a tcp stream
    if(  socketfd < 0 || socketfd == INVALID_SOCKET )
    {
        printf("socket(PF_INET, SOCK_STREAM, IPPROTO_TCP) failed with error:  %d", errno);
        return ReqlSocketError;
    }
    
    // Set linger explicitly off to kill connections on close
    // Note:  linger failure does not explicilty fail the connection

    lin.l_onoff = 0;
    lin.l_linger = 0;
    if( (setsockopt(socketfd, SOL_SOCKET, SO_LINGER, (const char *)&lin, sizeof(int))) < 0 )
    {
        printf("setsockopt(socketfd, SOL_SOCKET, SO_LINGER,...) failed with error:  %d\n", errno);
        //return ReqlSocketError;
    }
 
#ifndef _WIN32
    // Set socket to close on exec
    if( (fcntl(socketfd, F_SETFD, FD_CLOEXEC)) < 0 )
    {
        printf("fcntl(socketfd, F_SETFD, FD_CLOEXEC) failed with error:  %d\n", errno);
        return ReqlSocketError;
    }
#endif
    // Set socket tot nonblocking (important!)
    /*
    int flags = fcntl(socketfd, F_GETFL);
    if( (fcntl(socketfd, F_SETFL, flags | O_NONBLOCK)) < 0 )
    {
        printf("fcntl(socketfd, F_SETFL, flags | O_NONBLOCK) failed with error:  %d\n", errno);
        return ReqlSocketError;
    }
     */
    // Return the successfully created socket descriptor
    return socketfd;
}

int ReqlCloseSocket(REQL_SOCKET socketfd)
{
#ifdef _WIN32
	return closesocket(socketfd);
#else
    return close( socketfd );
#endif
}

	ULONG _ioCompletionKey = 0;//NULL;
	ULONG_PTR dwCompletionKey = (ULONG_PTR)NULL;

/***
 *
 *
 ***/
int ReqlSocketCreateEventQueue(ReqlSocketContext * socketContext)
{
	int event_queue = -1;
#ifdef __APPLE__
	//  Create a kqueue for managing socket events
    int event_queue = kqueue();
    
    //  Register kevents for socket connection notification
    struct kevent kev;
    void * context_ptr = NULL; //we can pass a context ptr around such as SSLContextRef or ReØMQL*, to retrieve when kevents come in on other threads
    EV_SET(&kev, socketfd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, context_ptr);
    kevent(event_queue, &kev, 1, NULL, 0, NULL);
    EV_SET(&kev, socketfd, EVFILT_WRITE, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, context_ptr);
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
 *  ReqlSocketWait
 *
 *  Wait for socket event on provided input kqueue event_queue
 *
 *  Returns the resulting kqueue event which should match socketfd on success,
 *  timeout or out of range otherwise
 *
 *  TO DO:  Add timeout input
 ***/
coroutine uintptr_t ReqlSocketWait(REQL_SOCKET socketfd, int event_queue, int16_t eventFilter)
{

#ifndef _WIN32
    //printf("ReqlSocketWait Start\n");
    struct kevent kev = {0};
    //  Synchronously wait for the socket event by idling until we receive kevent from our kqueue
    return kqueue_wait_with_timeout(event_queue, &kev, eventFilter, 10000);
#else
	return socketfd;
#endif
	//return ret_event;
}