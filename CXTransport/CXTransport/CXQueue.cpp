#include "../CXTransport.h"

#ifndef _WIN32
#include <assert.h>


void* __stdcall CX_Dequeue_Resolve_Connect_Handshake(LPVOID lpParameter)
{
	int i;
	OSStatus status;
	uint64_t queryToken = 0;
	
	CTCursor* cursor = NULL;

	CTOverlappedTarget  pendingTarget = {0};
	CTOverlappedTarget* overlappedTarget = NULL;
	//CTOverlappedResponse* overlappedResponse = NULL;

	unsigned long NumBytesTransferred = 0;
	unsigned long ioctlBytes = 0;
	//unsigned long recvMsgLength = 0;

	CTSSLStatus scRet = 0;
	CTClientError ctError = CTSuccess;

	//we passed the completion port handle as the lp parameter
	CTKernelQueue * cq = ((CTKernelQueue*)lpParameter);
	CTKernelQueueType cxQueue = cq->kq;
	CTKernelQueueType cxPipe[2] = {cq->pq[0], cq->pq[1]};//*((CTKernelQueue*)lpParameter);

	CTConnection* conn = NULL;
	unsigned long cBufferSize = ct_system_allocation_granularity();

	CTCursor* closeCursor = NULL;
	CTError err = { (CTErrorClass)0,0,NULL };

	fprintf(stderr, "CT_Dequeue_Resolve_Connect_Handshake Begin\n");

	//Create a kqueue to listen for incoming pipe eventss
    CTKernelQueueType pendingTargetQueue = kqueue();//CTKernelQueueCreate();

    /* Set the kqueue to start listening for messages on the incoming pipe */
    struct kevent overlappedEvent = {0};
    EV_SET(&overlappedEvent, cxPipe[CT_INCOMING_PIPE], EVFILT_READ, EV_ADD, 0, 0, NULL);
	kevent(pendingTargetQueue, &overlappedEvent, 1, NULL, 0, NULL);

	//TO DO:  Optimize number of packets dequeued at once
	while(1)
	{    
		int ret = 0;
		struct kevent kev[2] = {{0}, {0}};
		memset(&overlappedEvent, 0, sizeof(struct kevent));
		//kev[0].filter = EVFILT_USER;
		//kev[0].ident = CTKQUEUE_OVERLAPPED_EVENT;
		//kev[1].filter = EVFILT_READ | EV_EOF;
		//kev[1].udata = &
		//kev[1].ident = CTKQUEUE_OVERLAPPED_EVENT;
		overlappedTarget = NULL;

		fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::Waiting for cursor event\n");
	
		//wait for active target and read events with zero timeout 
		if( (ret = kqueue_wait_with_timeout(cxQueue, &(kev[0]), 2, 0)) < 0 )
		{
			fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::kqueue_wait_with_timeout 1 failed with ret (%d) and errno (%d).\n", ret, errno);
			assert(1==0);
		}
		else if(ret == 1)
		{
			//make sure we got the active target event
			assert( kev[0].filter == EVFILT_USER);

		}
		else if(ret == 2)
		{
			if( kev[0].filter == EVFILT_READ && kev[1].filter == EVFILT_USER)
			{
				struct kevent tmp = kev[0];
				kev[0] = kev[1];
				kev[1] = tmp;
			}
			//make sure kev[0] is the active cursor event
			assert( kev[0].filter == EVFILT_USER);
			assert( kev[1].filter == EVFILT_READ);
		}

		if( kev[0].filter == EVFILT_USER )
		{
			//assert(kev[0].filter == EVFILT_USER );//&& kev[0].ident == CTKQUEUE_OVERLAPPED_EVENT);

			//Get the overlapped struct associated with the asychronous IOCP call
			overlappedTarget = (CTOverlappedTarget*)(kev[0].udata);
			assert(overlappedTarget);

			cursor = overlappedTarget->cursor;
			cursor->target = overlappedTarget->target;
			//cursor->target->ctx = overlappedTarget;
			//overlappedTarget->cursor->overlappedResponse.buf += NumBytesTransferred;
			
			fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::Read overlappedTarget cursor (%llu) from cxQueue\n", cursor->queryToken);

			//if( overlappedTarget->stage == CT_OVERLAPPED_EXECUTE) //the active target is finishing an async connect
			//	assert(1==0);
			
			if( overlappedTarget->stage != CT_OVERLAPPED_RECV_FROM && overlappedTarget->stage != CT_OVERLAPPED_EXECUTE) //skip over this for async recv_from completion
			{
				
				fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::Waiting for read event\n");
				assert(1==0);
				//Wait for kqueue notification that bytes are available for reading on rxQueue
				if( kev[1].filter != EVFILT_READ && (ret = kqueue_wait_with_timeout(cxQueue, &(kev[1]), 1, CTSOCKET_MAX_TIMEOUT)) != 1 )
				{
					fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::kqueue_wait_with_timeout cxQueue read failed with ret (%d).\n", ret);
					assert(1==0);
				}
				//fprintf(stderr, "\nCT_Dequeue_Resolve_Connect_Handshake::kqueue_wait_with_timeout 2 ret = (%hd) errno = %d.\n", kev[1].filter, errno);

				fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::kqueue_wait_with_timeout cxQueue read filter = (%hd) errno = %ld.\n", kev[1].filter, kev[1].data);

				//assert(ret==1);
				if( kev[1].filter == EV_ERROR)
				{
					//assert(kev[1].filter == EVFILT_USER && kev[1].ident == CTKQUEUE_OVERLAPPED_EVENT);
					assert(1==0);
				}
				else if( kev[1].flags & EV_EOF)
				{
					assert(1==0);
				}
			
				assert( kev[1].filter == EVFILT_READ);
				assert( kev[1].ident == cursor->target->socket );
				assert((kev[1].udata));	
				assert(cxPipe[CT_INCOMING_PIPE] == (CTKernelQueueType)(uint64_t)(kev[1].udata) );
				NumBytesTransferred = kev[1].data;

				//Determine how many bytes are available on the socket for reading
				fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::Bytes Available\n");

				int ioRet = ioctlsocket(cursor->target->socket, FIONREAD, &ioctlBytes);
				assert(NumBytesTransferred > 0);
				//if( NumBytesRecv == 0) NumBytesRecv = 32768;

				fprintf(stderr, "**** nCX_Dequeue_Resolve_Connect_Handshake::NumBytesTransferred C status = %lu\n", NumBytesTransferred);
				fprintf(stderr, "**** nCX_Dequeue_Resolve_Connect_Handshake::ictlsocket C status = %d\n", ioRet);
				fprintf(stderr, "**** nCX_Dequeue_Resolve_Connect_Handshake::ictlsocket C = %lu\n", ioctlBytes);
				
				NumBytesTransferred = ioctlBytes;

				assert( kev[0].ident != CTSOCKET_EVT_TIMEOUT );
			}
		}
		else if( kev[0].filter == EVFILT_WRITE ) //there was only one event from cxQueue and it was a write event
		{
			assert(1==0);
		}
		else if( kev[0].filter == EVFILT_READ) //there was only one event cxQueue and it was a read event
		{
			assert(1==0);
		}
		else //there was no event
		{
			fprintf(stderr, "\nnCX_Dequeue_Resolve_Connect_Handshake::Waiting for overlapped pipe\n");
			//assert(kev[0].filter == EVFILT_READ);

			/* Grab any events written to the pipe from the crTimeServer */
			if( (ret = kqueue_wait_with_timeout(pendingTargetQueue, &kev[0], 1, CTSOCKET_MAX_TIMEOUT)) < 1 )
			{
				fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::kqueue_wait_with_timeout 0 failed with ret (%d).\n", ret);
				assert(1==0);
			}

			assert( kev[0].ident == cxPipe[CT_INCOMING_PIPE]);		
			ssize_t bytes = read((int)kev[0].ident, &pendingTarget, sizeof(CTOverlappedTarget));
			if( bytes == -1 )
			{
				fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::Read from overlapped pipe failed with error %d\n", errno);
				assert(1==0);
			}
			//fprintf(stderr, "\nCT_Dequeue_Recv_Decrypt::Read %d bytes from overlapped pipe \n", (int)bytes);
			assert(bytes == sizeof(CTOverlappedTarget));
			overlappedTarget = &pendingTarget;
			//assert(overlappedResponse);

			assert(overlappedTarget);
			assert(overlappedTarget->target);
			//cursor = overlappedTarget->cursor;
			//cursor->target = overlappedTarget->target;
			//cursor->target->ctx = overlappedTarget;
			//	overlappedTarget->cursor->overlappedResponse.buf += NumBytesTransferred;
				
			fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::Read connection target (%s) from cxPipe\n", overlappedTarget->target->url.host);
		}
	

		if (overlappedTarget->stage == CT_OVERLAPPED_SCHEDULE)
		{
			fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::CT_OVERLAPPED_SCHEDULE\n");
			//This will resolve the host asynchronously and kick off the async connection returning to this loop for the various completion stage
			//overlappedTarget->target->ctx = overlappedTarget; // store the overlapped for subsquent async connection execution
			CTTargetResolveHost(overlappedTarget->target, overlappedTarget->target->callback);
		}
		else if (overlappedTarget->stage == CT_OVERLAPPED_RECV_FROM)
		{
			fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::CT_OVERLAPPED_RECV_FROM\n");
			//assert(1 == 0);
			cursor = overlappedTarget->cursor;
			cursor->target = overlappedTarget->target;
			/*
			//cursor->target->ctx = overlappedTarget;
			overlappedTarget->cursor->overlappedResponse.buf += NumBytesTransferred;
			if (cursor->headerLength == 0)
			{
				//search for end of protocol header (and set cxCursor headerLength property
				char* endOfHeader = cursor->headerLengthCallback(cursor, cursor->file.buffer, NumBytesTransferred);

				//calculate size of header
				if (endOfHeader)
				{
					cursor->headerLength = endOfHeader - (cursor->file.buffer);

					memcpy(cursor->requestBuffer, cursor->file.buffer, cursor->headerLength);

					fprintf(stderr, "CTDecryptResponseCallbacK::Header = \n\n%.*s\n\n", cursor->headerLength, cursor->requestBuffer);

					//copy body to start of buffer to overwrite header 
					memcpy(cursor->file.buffer, endOfHeader, NumBytesTransferred - cursor->headerLength);
					//overlappedResponse->buf += endDecryptedBytes - endOfHeader;

					overlappedTarget->cursor->overlappedResponse.buf -= cursor->headerLength;
				}

			}
			assert(cursor->contentLength > 0);

			if (cursor->contentLength <= overlappedTarget->cursor->overlappedResponse.buf - cursor->file.buffer)
			{
				closeCursor = cursor;
				//increment the connection response count
				//closeCursor->conn->rxCursorQueueCount--;
				//closeCursor->conn->responseCount++;

#ifdef _DEBUG
				assert(closeCursor->responseCallback);
#endif
				//issue the response to the client caller
				closeCursor->responseCallback(&err, closeCursor);
				closeCursor = NULL;	//set to NULL for posterity

			}
			*/

			struct CTError error = { (CTErrorClass)0,0,0 };    //Reql API client functions will generally return ints as errors

			//char* target_host = cursor->target->proxy.host ? cursor->target->proxy.host : cursor->target->url.host;
			short target_port = cursor->target->proxy.host  ? cursor->target->proxy.port : cursor->target->url.port;

			//struct dill_ipaddr addr[1];
			//struct dns_addrinfo* ai = (struct dns_addrinfo*)(cursor->target->ctx);

			int numResolvedAddresses = 0;
			if ((numResolvedAddresses = dill_ipaddr_dns_query_wait_ai(cursor->target->dns.ai, (struct dill_ipaddr *)&(cursor->target->url.addr), 1, target_port, cursor->target->url.addr.ss_family == AF_INET6 ? DILL_IPADDR_IPV6 : DILL_IPADDR_IPV4, -1)) < 1)
			{
				fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::dill_ipaddr_dns_query_wait_ai failed to resolve any IPV4 addresses!\n");
				error.errClass = CTDriverErrorClass;
				error.id = CTDNSError;
				goto CONN_CALLBACK;

			}

			fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::DNS resolved %d ip address(es)\n", numResolvedAddresses);
			//struct sockaddr_storage* ptr = (struct sockaddr_storage*)addr;//(addr[0]);
			//cursor->target->url.addr = *ptr;//(struct sockaddr_in*)addresses;

			closeCursor = cursor;
			//increment the connection response count
			//closeCursor->conn->rxCursorQueueCount--;
			//closeCursor->conn->responseCount++;

#ifdef _DEBUG
			assert(closeCursor->responseCallback);
#endif
			//issue the response to the client caller
			closeCursor->responseCallback(&err, closeCursor);
			closeCursor = NULL;	//set to NULL for posterity
			continue;
		}
		else if (overlappedTarget->stage == CT_OVERLAPPED_EXECUTE)
		{
			fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::CT_OVERLAPPED_EXECUTE\n");

			cursor = overlappedTarget->cursor;
			cursor->target = overlappedTarget->target;

			//wait for write to be available to indicate connection is complete
			if( kev[1].filter != EVFILT_WRITE && (ret = kqueue_wait_with_timeout(cxQueue, &(kev[1]), 1, CTSOCKET_MAX_TIMEOUT)) != 1 )
			{
				fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::kqueue_wait_with_timeout cxQueue write failed with ret (%d).\n", ret);
				assert(1==0);
			}

			fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::kqueue_wait_with_timeout cxQueue write filter = (%hd) errno = %ld.\n", kev[1].filter, kev[1].data);

			//assert(ret==1);
			if( kev[1].filter == EV_ERROR)
			{
				//assert(kev[1].filter == EVFILT_USER && kev[1].ident == CTKQUEUE_OVERLAPPED_EVENT);
				assert(1==0);
			}
			else if( kev[1].flags & EV_EOF)
			{
				assert(1==0);
			}
		
			assert( kev[1].filter == EVFILT_WRITE);
			assert( kev[1].ident == cursor->target->socket );
			//assert((kev[1].udata));	
			//assert(cxPipe[CT_INCOMING_PIPE] == (CTKernelQueueType)(kev[1].udata) );

			//This will get the result of the async connection and perform ssl handshake
			fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::ConnectEx complete error = %d\n\n", CTSocketError());
			//struct CTConnection conn = { 0 };
			//get a pointer to a new connection object memory
			//struct CTError error = { (CTErrorClass)0,0,0 };    //Reql API client functions will generally return ints as errors

			//Check for socket error to ensure that the connection succeeded
			if ((err.id = CTSocketGetError(overlappedTarget->target->socket)) != 0)
			{
				// connection failed; error code is in 'result'
				fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::socket async connect failed with result: %d", err.id);
				CTSocketClose(overlappedTarget->target->socket);
				err.id = CTSocketConnectError;
				goto CONN_CALLBACK;
			}

			fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::After CTSocketGetError\n\n");

			/* Make the socket more well-behaved.
			int rc = setsockopt(overlappedTarget->target->socket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
			if (rc != 0) {
				fprintf(stderr, "SO_UPDATE_CONNECT_CONTEXT failed: %d\n", WSAGetLastError());
				return 1;
			}
			*/

			conn = CTGetNextPoolConnection();
			memset(conn, 0, sizeof(CTConnection));

			conn->socket = overlappedTarget->target->socket;
			conn->socketContext.socket = overlappedTarget->target->socket;
			conn->socketContext.host = overlappedTarget->target->url.host;

#ifdef _WIN32
			//For Loading
			IO_STATUS_BLOCK iostatus;
			pNtSetInformationFile NtSetInformationFile = NULL;

			//For Executing
			FILE_COMPLETION_INFORMATION  socketCompletionInfo = { NULL, NULL };// { overlappedTarget->target->cxQueue, dwCompletionKey };
			ULONG fcpLen = sizeof(FILE_COMPLETION_INFORMATION);

			//Load library if needed...
			//	ntdll = LoadLibrary("ntdll.dll");
			//	if (ntdll == NULL) {
			//		return 0;
			//	}

			fprintf(stderr, "\nBefore GetProcAddress\n\n");

			/*
			HINSTANCE hDll = LoadLibrary("ntdll.dll");
			if (!hDll) {
				fprintf(stderr, "ntdll.dll failed to load!\n");
				assert(1 == 0);
			}
			fprintf(stderr, "ntdll.dll loaded successfully...\n");
			*/

			//LoadNtSetInformation function ptr from ntdll.lib

			NtSetInformationFile = (pNtSetInformationFile)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtSetInformationFile");
			if (NtSetInformationFile == NULL) {
				assert(1 == 0);
				return 0;
			}

			//remove the existing completion port attache to socket handle used for connection
			if (NtSetInformationFile(conn->socket, &iostatus, &socketCompletionInfo, sizeof(FILE_COMPLETION_INFORMATION), FileReplaceCompletionInformation) < 0) {
				assert(1 == 0);
				return 0;
			}
#else
			//unsubscribe from socket events on the cxQueue
			struct kevent kev;
			EV_SET(&kev, conn->socket, EVFILT_READ, EV_DELETE, 0, 0, NULL);
			kevent(overlappedTarget->target->cxQueue, &kev, 1, NULL, 0, NULL);
			EV_SET(&kev, conn->socket, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
			kevent(overlappedTarget->target->cxQueue, &kev, 1, NULL, 0, NULL);
		
#ifdef CTRANSPORT_SANS_AIO
            //kqueue sans aio path
			//subscribe to socket read events on rxQueue
			EV_SET(&kev, conn->socket, EVFILT_READ, EV_ADD | EV_ENABLE , 0, 0, (void*)(uint64_t)(overlappedTarget->target->rxPipe[CT_INCOMING_PIPE]));
			kevent(overlappedTarget->target->rxQueue, &kev, 1, NULL, 0, NULL);
#endif

#endif
			//Copy tx, rx completion port queue from target to connection and attach them to the socket handle for async writing and async reading, respectively
			//We assume that since we are executing on a cxQueue placed on the target by the user, they will have also specified their own txQueue, rxQueue as well
			conn->socketContext.cq = overlappedTarget->target->cq;
			conn->socketContext.tq = overlappedTarget->target->tq;// CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, dwCompletionKey, 0);
			conn->socketContext.rq = overlappedTarget->target->rq;// CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, dwCompletionKey, 0);
			//CreateIoCompletionPort((HANDLE)conn->socket, conn->socketContext.rxQueue, dwCompletionKey, 1);
			fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::After CreateIoCompletionPort\n\n");

			//There is some short order work that would be good to parallelize regarding loading the cert and saving it to keychain
			//However, the SSLHandshake loop has already been optimized with yields for every asynchronous call

#ifdef _DEBUG
			fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::Before CTSSLRoutine Host = %s\n", conn->socketContext.host);
			fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::Before CTSSLRoutine Host = %s\n", overlappedTarget->target->url.host);
#endif

			//put the connection callback on the service so we can ride it through the async handshake

			conn->target = overlappedTarget->target;

			if (conn->target->proxy.host)
			{
				//Initate the Async SSL Handshake scheduling from this thread (ie cxQueue) to rxQueue and txQueue threads
				if ((status = CTProxyHandshake(conn)) != 0)
				{
					fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::CTSSLRoutine failed with error: %d\n", (int)status);
					err.id = CTSSLHandshakeError;
					goto CONN_CALLBACK;
					//return status;
				}

				//if we are running the handshake asynchronously on a queue, don't return connection to client yet
				if (conn->socketContext.txQueue)
					continue;
			}
			else if (conn->target->ssl.method > CTSSL_NONE)
			{
				//Initate the Async SSL Handshake scheduling from this thread (ie cxQueue) to rxQueue and txQueue threads
				if ((status = CTSSLRoutine(conn, conn->socketContext.host, overlappedTarget->target->ssl.ca)) != 0)
				{
					fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::CTSSLRoutine failed with error: %d\n", (int)status);
					err.id = CTSSLHandshakeError;
					goto CONN_CALLBACK;
					//return status;
				}

				//if we are running the handshake asynchronously on a queue, don't return connection to client yet
				if (conn->socketContext.txQueue)
					continue;
			}

			/*
			int iResult;

			u_long iMode = 1;

			//-------------------------
			// Set the socket I/O mode: In this case FIONBIO
			// enables or disables the blocking mode for the
			// socket based on the numerical value of iMode.
			// If iMode = 0, blocking is enabled;
			// If iMode != 0, non-blocking mode is enabled.

			iResult = ioctlsocket(conn.socket, FIONBIO, &iMode);
			if (iResult != NO_ERROR)
				fprintf(stderr, "ioctlsocket failed with error: %ld\n", iResult);
			*/

			//The connection completed successfully, allocate memory to return to the client
			err.id = CTSuccess;

		CONN_CALLBACK:
			//fprintf(stderr, "before callback!\n");
			conn->responseCount = 0;  //we incremented this for the handshake, it is critical to reset this for the client before returning the connection
			conn->queryCount = 0;
			CTSetCursorPoolIndex(0);
			overlappedTarget->target->callback(&err, conn);
			//fprintf(stderr, "After callback!\n");

			/*
			//Remove observer that yields to coroutines
			CFRunLoopRemoveObserver(runLoop, runLoopObserver, kCFRunLoopDefaultMode);
			//Remove dummy source that keeps run loop alive
			CFRunLoopRemoveSource(runLoop, runLoopSource, kCFRunLoopDefaultMode);

			//release runloop object memory
			CFRelease(runLoopObserver);
			CFRelease(runLoopSource);
			*/

			//return the error id
			return 0;//error.id;

		}
		else
		{
			fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::UNHANDLED_STAGE (%d)\n", overlappedTarget->stage);
			fflush(stderr);
		}
		/*
		if (NumBytesSent == 0)
		{
			fprintf(stderr, "CTConnection::CTConnectThread::Server Disconnected!!!\n");
		}
		else
		{

			CTTargetResolveHost(overlappedTarget->target, overlappedTarget->callback);

		}
		*/
	

	}

	fprintf(stderr, "\nCX_Dequeue_Resolve_Connect_Handshake::End\n");

	//ExitThread(0);
	return 0;

}


void* __stdcall CX_Dequeue_Recv_Decrypt(LPVOID lpParameter)
{
	uintptr_t event;
	int i;
	
	//CTQueryMessageHeader * header = NULL;
	CTOverlappedResponse overlappedCursor = {0};
	CTOverlappedResponse nextOverlappedCursor = {0};

	CTOverlappedResponse* overlappedResponse = NULL;
	CTOverlappedResponse* nextOverlappedResponse = NULL;

	struct aiocb* aioEvent = NULL;

	unsigned long NumBytesRecv = 0;
	unsigned long ioctlBytes = 0;

	unsigned long NumBytesRemaining = 0;
	char* extraBuffer = NULL;

	unsigned long PrevBytesRecvd = 0;
	//unsigned long TotalBytesToDecrypt = 0;
	//unsigned long NumBytesRecvMore = 0;
	unsigned long extraBufferOffset = 0;
	CTSSLStatus scRet = 0;
	CTClientError ctError = CTSuccess;

	//struct timespec ts = {0};

	CXConnection* cxConn = NULL;// (CXConnection*)lpParameter;
	CTKernelQueue * rq = ((CTKernelQueue*)lpParameter);
	CTKernelQueueType rxQueue = rq->kq;
	CTKernelQueueType rxPipe[2] = {rq->pq[0], rq->pq[1]};//*((CTKernelQueue*)lpParameter);

	//CTKernelQueuePipePair *qPair = (CTKernelQueuePipePair*)lpParameter;
	//CTKernelQueue rxQueue = qPair->q;//*((CTKernelQueue*)lpParameter);
	//CTKernelQueue oxPipe[2] = {qPair->p[0], qPair->p[1]};//*((CTKernelQueue*)lpParameter);

	unsigned long cBufferSize = ct_system_allocation_granularity();

	//unsigned long headerLength;
	//unsigned long contentLength;
	CTCursor* cursor = NULL;
	CTCursor* nextCursor = NULL;
	CTCursor* closeCursor = NULL;

	struct kevent nextCursorMsg = { 0 };
	CTError err = { (CTErrorClass)0,0,NULL };

	//unsigned long currentThreadID = GetCurrentThreadId();

	//Create a kqueue to listen for incoming pipe eventss
    //CTKernelQueue activeCursorQueue = CTKernelQueueCreate();
    CTKernelQueueType pendingCursorQueue = kqueue();//CTKernelQueueCreate();

    /* Set the kqueue to start listening for messages on the incoming pipe */
    struct kevent overlappedEvent = {0};
    EV_SET(&overlappedEvent, rxPipe[CT_INCOMING_PIPE], EVFILT_READ, EV_ADD, 0, 0, NULL);
	kevent(pendingCursorQueue, &overlappedEvent, 1, NULL, 0, NULL);

	fprintf(stderr, "\nCX_Dequeue_Recv_Decrypt::Begin\n");

	while(1)
	{    
		int ret = 0;
		ssize_t aioBytes = -1;
		
		struct aiocb aio;
		bzero(&aio, sizeof(aio));
		aioEvent = &aio;
		//aio.aio_sigevent.sigev_notify_kqueue = 13;
		//aio.aio_sigevent.sigev_value.sival_ptr = overlappedResponse;
		//aio.aio_sigevent.sigev_notify = SIGEV_KEVENT;//SIGEV_SIGNAL;

		struct kevent kev[2] = {{0}, {0}};
		memset(&overlappedEvent, 0, sizeof(struct kevent));
		//kev[0].filter = EVFILT_USER;
		//kev[0].ident = CTKQUEUE_OVERLAPPED_EVENT;
		//kev[1].filter = EVFILT_READ | EV_EOF;
		//kev[1].udata = &
		//kev[1].ident = CTKQUEUE_OVERLAPPED_EVENT;
		overlappedResponse = NULL;



		fprintf(stderr, "\nCX_Dequeue_Recv_Decrypt::Waiting for cursor event\n");

#ifndef CTRANSPORT_SANS_AIO
		//wait for lio_listio/aio_read completion event with zero timeout 
		
		/*	
		if( (aioBytes = aio_waitcomplete(&aioEvent, NULL)) == -1)
		{
			assert(1==0);
		}
		//fprintf(stderr, "\nCT_Dequeue_Recv_Decrypt::After aio_waitcomplete\n");
		*/
		
		if( (ret = kqueue_wait_with_timeout(rxQueue, &(kev[0]), 1, CTSOCKET_MAX_TIMEOUT)) < 0 )
		{
			fprintf(stderr, "\nCT_Dequeue_Recv_Decrypt::kqueue_wait_with_timeout aio rxQueue failed with ret (%d) and errno (%d).\n", ret, errno);
			assert(1==0);
		}

		if(ret == 1)
		{
			//make sure we got the active cursor event
			if( kev[0].filter == EVFILT_AIO )
			{
				overlappedResponse = (CTOverlappedResponse*)(kev[0].udata);
				struct aiocb * aioEventPtr = &(overlappedResponse->Overlapped.hAIO); 
				assert( kev[0].ident == (uintptr_t)aioEventPtr );
				//assert( aiocbPtr->aio_nbytes == 0 );
			}
#ifdef __FreeBSD__
			else if( kev[0].filter == EVFILT_LIO )
			{
				assert(1==0);
			}
#endif
			else
				assert(1==0);
		}
		else
			assert(1==0);
		
		//Get the overlapped struct associated with the asychronous AIO call
		overlappedResponse = (CTOverlappedResponse*)(kev[0].udata);
		//overlappedResponse = (CTOverlappedResponse*)(aioEvent->aio_sigevent.sigev_value.sival_ptr);

		//Get the cursor/connection that has been pinned on the overlapped
		cursor = (CTCursor*)overlappedResponse->cursor;
		cxConn = (CXConnection*)(cursor->conn->object_wrapper);

		//Retrieve the number of bytes provided from the socket buffer by IOCP
#ifdef AIO_KEVENT_FLAG_REAP
		NumBytesRecv = (unsigned long)(kev[0].data);
#else		
		if( (aioBytes = aio_return( &(overlappedResponse->Overlapped.hAIO))) < 0 ) assert( aioBytes >= 0 );
		NumBytesRecv = (unsigned long)aioBytes;
#endif
		assert(overlappedResponse);
		assert(cursor);

		fprintf(stderr, "\nCX_Dequeue_Recv_Decrypt::Read cursor (%lu) from rxQueue\n", cursor->queryToken);
#else
        //wait for active cursor and read events with zero timeout
        if( (ret = kqueue_wait_with_timeout(rxQueue, &(kev[0]), 2, 0)) < 0 )
        {
            fprintf(stderr, "\nCX_Dequeue_Recv_Decrypt::kqueue_wait_with_timeout 1 failed with ret (%d) and errno (%d).\n", ret, errno);
            assert(1==0);
        }
        else if(ret == 1)
        {
            //make sure we got the active cursor event
            if( kev[0].filter != EVFILT_USER )
            {
                assert( kev[0].filter == EVFILT_READ);
                kev[1] = kev[0];
                memset(&kev[0], 0, sizeof(struct kevent));
            }
            else
                assert( kev[0].filter == EVFILT_USER);
        }
        else if(ret == 2)
        {
            if( kev[0].filter == EVFILT_READ && kev[1].filter == EVFILT_USER)
            {
                struct kevent tmp = kev[0];
                kev[0] = kev[1];
                kev[1] = tmp;
            }
            //make sure kev[0] is the active cursor event
            assert( kev[0].filter == EVFILT_USER);
            assert( kev[1].filter == EVFILT_READ);
        }


        if( kev[0].filter == EVFILT_USER ) {
        
            //Get the overlapped struct associated with the asychronous IOCP call
            overlappedResponse = (CTOverlappedResponse*)(kev[0].udata);
            //overlappedResponse = (CTOverlappedReponse*)aioEvent->aio_sigevent.sigev_value.sival_ptr;

            //Get the cursor/connection that has been pinned on the overlapped
            cursor = (CTCursor*)overlappedResponse->cursor;
            cxConn = (CXConnection*)(cursor->conn->object_wrapper);

            assert(overlappedResponse);
            assert(cursor);

            fprintf(stderr, "\nCX_Dequeue_Recv_Decrypt::Read cursor (%llu) from rxQueue\n", cursor->queryToken);
        }
        else
        {
            fprintf(stderr, "\nCX_Dequeue_Recv_Decrypt::Waiting for overlapped pipe\n");

            // Grab any events written to the pipe from the txQueue
            if( (ret = kqueue_wait_with_timeout(pendingCursorQueue, &kev[0], 1, CTSOCKET_MAX_TIMEOUT)) < 1 )
            {
                fprintf(stderr, "\nCX_Dequeue_Recv_Decrypt::kqueue_wait_with_timeout 0 failed with ret (%d).\n", ret);
                assert(1==0);
            }

            assert( kev[0].ident == rxPipe[CT_INCOMING_PIPE]);
            ssize_t bytes = read((int)kev[0].ident, &overlappedCursor, sizeof(CTOverlappedResponse));
            if( bytes == -1 )
            {
                fprintf(stderr, "\nCX_Dequeue_Recv_Decrypt::Read from overlapped pipe failed with error %d\n", errno);
                assert(1==0);
            }
            //fprintf(stderr, "\nCT_Dequeue_Recv_Decrypt::Read %d bytes from overlapped pipe \n", (int)bytes);
            assert(bytes == sizeof(CTOverlappedResponse));
            overlappedResponse = &overlappedCursor;
            assert(overlappedResponse);
            cursor = (CTCursor*)overlappedResponse->cursor;
            cxConn = (CXConnection*)(cursor->conn->object_wrapper);
            assert(cursor);
            //assert(cursor->queryToken == 0);
            fprintf(stderr, "\nCX_Dequeue_Recv_Decrypt::Read cursor (%llu) from rxPipe\n", cursor->queryToken);
        }

        fprintf(stderr, "\nCX_Dequeue_Recv_Decrypt::Waiting for read event\n");

        //Wait for kqueue notification that bytes are available for reading on rxQueue
        if( kev[1].filter != EVFILT_READ && (ret = kqueue_wait_with_timeout(rxQueue, &(kev[1]), 1, CTSOCKET_MAX_TIMEOUT)) != 1 )
        {
            fprintf(stderr, "\nCX_Dequeue_Recv_Decrypt::kqueue_wait_with_timeout rxQueue read failed with ret (%d).\n", ret);
            assert(1==0);
        }
        //fprintf(stderr, "\nCT_Dequeue_Recv_Decrypt::kqueue_wait_with_timeout 2 ret = (%hd) errno = %d.\n", kev[1].filter, errno);

        fprintf(stderr, "\nCX_Dequeue_Recv_Decrypt::kqueue_wait_with_timeout rxQueue read filter = (%hd) errno = %ld.\n", kev[1].filter, kev[1].data);

        //assert(ret==1);
        if( kev[1].filter == EV_ERROR)
        {
            //assert(kev[1].filter == EVFILT_USER && kev[1].ident == CTKQUEUE_OVERLAPPED_EVENT);
            assert(1==0);
        }
        else if( kev[1].flags & EV_EOF)
        {
            assert(1==0);
        }
    
        assert( kev[1].filter == EVFILT_READ);
        assert( kev[1].ident == cursor->conn->socket );
        assert((kev[1].udata));
        assert(rxPipe[CT_INCOMING_PIPE] == (CTKernelQueueType)(uint64_t)(kev[1].udata) );
        NumBytesRecv = kev[1].data;
#endif

#ifdef CTRANSPORT_SANS_AIO
        if( kev[0].ident != CTSOCKET_EVT_TIMEOUT ) {
#else
        if( overlappedResponse ) {
#endif
            //Cursor requests are always queued onto the connection's decrypt thread (ie this one) to be scheduled
			//so every overlapped sent to this decrypt thread is handled here first.
			//There are two possible scenarios:
			//	1)  There are currently previous cursor requests still being processed and the cursor request is scheduled
			//		for pickup later from this thread's message queue
			//	2)  The are currently no previous cursor requests still being processed and so we may initiate an async socket recv
			//		for the cursor's response
			
			fprintf(stderr, "CX_Dequeue_Recv_Decrypt::cursor = (%d)\n", (int)cursor->queryToken);
			fprintf(stderr, "CX_Dequeue_Recv_Decrypt::overlappedResponse->stage = (%d)\n", (int)overlappedResponse->stage);

			if (overlappedResponse->stage > CT_OVERLAPPED_STAGE_NULL && overlappedResponse->stage < CT_OVERLAPPED_SCHEDULE_RECV_FROM)
			{
				fprintf(stderr, "CX_Dequeue_Recv_Decrypt::Scheduling Cursor (%d) via ...", (int)cursor->queryToken);

				//set the overlapped message type to indicate that scheduling for the current
				//overlapped/cursor has already been processed
				overlappedResponse->stage = (CTOverlappedStage) ((int)overlappedResponse->stage + 4);// CT_OVERLAPPED_EXECUTE;
				int64_t zero = 0;
				if (cursor->conn->responseCount > zero)// < cursor->queryToken)
				{
					fprintf(stderr, "PostThreadMessage (%lld)\n", cursor->conn->responseCount);
					//assert(1==0);
					/*
					if (PostThreadMessage(currentThreadID, WM_USER + cursor->queryToken, 0, (LPARAM)cursor) < 1)
					{
						//cursor->conn->rxCursorQueueCount++;
						fprintf(stderr, "CTDecryptResponseCallback::PostThreadMessage failed with error: %d", GetLastError());
					}
					*/

#ifndef CTRANSPORT_SANS_AIO
                    //write to pipe being read by thread running rxQueue
                    ssize_t ret = 0;
                    if( (ret = write(cursor->conn->socketContext.rxPipe[CT_OUTGOING_PIPE], overlappedResponse, sizeof(CTOverlappedResponse))) == -1)
                    {
                        fprintf(stderr, "CT_Dequeue_Recv_Decrypt::write to pipe failed with error (%d)\n\n", errno);
                        assert(1==0);
                    }
#endif
				}
				else
				{
					fprintf(stderr, "CTCursorAsyncRecv\n");
#ifndef CTRANSPORT_SANS_AIO
                    int ioRet = ioctlsocket(cursor->conn->socket, FIONREAD, &ioctlBytes);
                    assert(NumBytesRecv == 0);
                    //if( NumBytesRecv == 0) NumBytesRecv = 32768;

                    fprintf(stderr, "**** CX_Dequeue_Recv_Decrypt::NumBytesRecv C status = %lu\n", NumBytesRecv);
                    fprintf(stderr, "**** CX_Dequeue_Recv_Decrypt::ictlsocket C status = %d\n", ioRet);
                    fprintf(stderr, "**** CX_Dequeue_Recv_Decrypt::ictlsocket C = %lu\n", ioctlBytes);
                    
                    NumBytesRecv = cBufferSize;
                    
                    if ((ctError = CTCursorAsyncRecv(&overlappedResponse, (void*)(cursor->file.buffer), 0, &NumBytesRecv)) != CTSocketIOPending)
                    {
                        //the async operation returned immediately
                        if (ctError == CTSuccess)
                        {
                            //cursor->conn->rxCursorQueueCount++;
                            fprintf(stderr, "CX_Dequeue_Recv_Decrypt::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRecv);
                        }
                        else //failure
                        {
                            fprintf(stderr, "CX_Dequeue_Recv_Decrypt::CTAsyncRecv failed with CTDriverError = %d\n", ctError);
                        }
                    }
#else
                    cursor->conn->responseCount++;
#endif
                }
#ifndef CTRANSPORT_SANS_AIO
                cursor->conn->responseCount++;
                continue;
#endif
			}

#ifdef CTRANSPORT_SANS_AIO
            //Determine how many bytes are available on the socket for reading
            //fprintf(stderr, "\nCT_Dequeue_Recv_Decrypt::Bytes Available\n");

            int ioRet = ioctlsocket(cursor->conn->socket, FIONREAD, &ioctlBytes);
            assert(NumBytesRecv > 0);
            //if( NumBytesRecv == 0) NumBytesRecv = 32768;

            fprintf(stderr, "**** CX_Dequeue_Recv_Decrypt::NumBytesRecv C status = %lu\n", NumBytesRecv);
            fprintf(stderr, "**** CX_Dequeue_Recv_Decrypt::ictlsocket C status = %d\n", ioRet);
            fprintf(stderr, "**** CX_Dequeue_Recv_Decrypt::ictlsocket C = %lu\n", ioctlBytes);
            
            NumBytesRecv = ioctlBytes;

            // Sync Wait for query response on ReqlConnection socket
            unsigned long recv_length = NumBytesRecv;
            char * responsePtr = (char*)CTRecvBytes( cursor->conn, overlappedResponse->wsaBuf.buf, &recv_length );
            if( !responsePtr ) responsePtr = (char*)CTRecvBytes( cursor->conn, overlappedResponse->wsaBuf.buf, &recv_length );

            assert(responsePtr);
            assert(recv_length > 0);
            //fprintf(stderr, "**** CT_Dequeue_Recv_Decrypt::CTRecv bytes = %lu\n", recv_length);
            //assert(recv_length == NumBytesRecv);

            //assert(recv_length == NumByte)
            NumBytesRecv = recv_length;
#endif


			NumBytesRecv += overlappedResponse->wsaBuf.buf - overlappedResponse->buf;
			//overlappedResponse->buf[NumBytesRecv] = '\0';

			fprintf(stderr, "Cursor Response Count (%llu) NumBytesRecv (%lu)\n\n", cursor->conn->responseCount, NumBytesRecv);

			PrevBytesRecvd = NumBytesRecv;	 //store the recvd bytes value from the overlapped
			NumBytesRemaining = cBufferSize; //we can never decrypt more bytes than the size of our buffer, and likely schannel will still separate that into chunks

			extraBuffer = overlappedResponse->buf;
			NumBytesRemaining = NumBytesRecv;
			scRet = SEC_E_OK;

			if (overlappedResponse->stage == CT_OVERLAPPED_RECV)
			{
				//we advance the buffer as if it were processed, because in the non-TLS case it is already processed
				overlappedResponse->buf += NumBytesRecv;

				while (NumBytesRemaining > 0 && scRet == SEC_E_OK)
				{

					if (cursor->headerLength == 0)
					{
						//search for end of protocol header (and set cxCursor headerLength property
						char* endOfHeader = NULL;

						if (cxConn)
						{
							std::shared_ptr<CXCursor> cxCursor = cxConn->getRequestCursorForKey(cursor->queryToken);
							endOfHeader = cxCursor->ProcessResponseHeader(cursor, cursor->file.buffer, NumBytesRecv);
						}
						else
						{
							endOfHeader = cursor->headerLengthCallback(cursor, cursor->file.buffer, NumBytesRecv);
						}

						//calculate size of header
						if (endOfHeader)
						{
							cursor->headerLength = endOfHeader - (cursor->file.buffer);

							fprintf(stderr, "CX_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV Header = \n\n%.*s\n\n", (int)cursor->headerLength, cursor->file.buffer);

							if (cursor->requestBuffer != cursor->file.buffer && cursor->headerLength > 0)
								memcpy(cursor->requestBuffer, cursor->file.buffer, cursor->headerLength);

							//fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV Header (%llu) = \n\n%.*s\n\n", cursor->headerLength, cursor->requestBuffer);

							//copy body to start of buffer to overwrite header 
							if (endOfHeader != cursor->file.buffer)
							{
								if (NumBytesRecv == cursor->headerLength)
									memset(cursor->file.buffer, 0, cursor->headerLength);
								else
								{
									//memcpy(cursor->file.buffer, endOfHeader, NumBytesRecv - cursor->headerLength);
									memmove( cursor->file.buffer, endOfHeader, NumBytesRecv - cursor->headerLength); 
									//fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV Body (%lu) = \n\n%.*s\n\n", cursor->contentLength, (int)cursor->contentLength, cursor->file.buffer);
									fprintf(stderr, "CX_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV Body (%lu)\n\n", cursor->contentLength);

								}
							}
							overlappedResponse->buf -= cursor->headerLength;
							NumBytesRemaining -= cursor->headerLength;
						}
					}
					//assert(cursor->contentLength > 0);

					fprintf(stderr, "CX_Deque_Recv_Decrypt::Check cursor->contentLength (%d, %d) \n\n", (int)cursor->contentLength, (int)(overlappedResponse->buf - cursor->file.buffer));
					if (cursor->contentLength > 0 && ((int)cursor->contentLength) <= (int)(overlappedResponse->buf - cursor->file.buffer))
					{
						extraBufferOffset = 0;
						closeCursor = cursor;
						fprintf(stderr, "Copy closeCursor->responseCount = %lld\n", closeCursor->conn->responseCount);
						NumBytesRemaining = 0;// (cursor->contentLength + cursor->headerLength);// -NumBytesRecv;
						nextCursor = NULL;
						nextOverlappedResponse = NULL;
						
						memset(&nextCursorMsg, 0, sizeof(struct kevent));
						//nextCursorMsg.filter = EVFILT_USER;
						//nextCursorMsg.ident = (uintptr_t )(cursor->queryToken + 1);

						fprintf(stderr, "\nCX_Dequeue_Recv_Decrypt::Waiting for nextCursor event on pipe\n");

						/*
						//Synchronously wait for a cursor  event by idling until we receive kevent from our kqueue
						if( (ret = kqueue_wait_with_timeout(oxQueue, &nextCursorMsg, 1, 0)) < 0 )
						{
							fprintf(stderr, "\nCT_Dequeue_Recv_Decrypt::kqueue_wait_with_timeout nextCursorMsg failed with ret (%d).\n", ret);
							assert(1==0);
						}
						*/


						/* Grab any events written to the pipe from the crTimeServer */
						if( (ret = kqueue_wait_with_timeout(pendingCursorQueue, &nextCursorMsg, 1, 0)) < 0 )
						{
							fprintf(stderr, "\nCT_Dequeue_Recv_Decrypt::kqueue_wait_with_timeout nextCursorMsg failed with ret (%d).\n", ret);
							assert(1==0);
						}

				
						//if (PeekMessage(&nextCursorMsg, NULL, WM_USER, WM_USER + cursor->conn->queryCount - 1, PM_REMOVE)) //(nextCursor)
						if( ret == 1)
						{
							//assert(1==0);

							assert( nextCursorMsg.ident == rxPipe[CT_INCOMING_PIPE]);		
							ssize_t bytes = read((int)nextCursorMsg.ident, &nextOverlappedCursor, sizeof(CTOverlappedResponse));
							if( bytes == -1 )
							{
								fprintf(stderr, "\nCT_Dequeue_Recv_Decrypt::Read from overlapped pipe (nextCursorMsg) failed with error %d\n", errno);
								assert(1==0);
							}
							//fprintf(stderr, "\nCT_Dequeue_Recv_Decrypt::Read %d bytes from overlapped pipe \n", (int)bytes);
							assert(bytes == sizeof(CTOverlappedResponse));
							//overlappedResponse = &overlappedCursor;
							//assert(overlappedResponse);
							//cursor = (CTCursor*)overlappedResponse->cursor;
							//assert(cursor);
							//fprintf(stderr, "\nCT_Dequeue_Recv_Decrypt::Read cursor (%lu) from oxPipe\n", cursor->queryToken);


							//assert(nextCursorMsg.filter == EVFILT_USER && nextCursorMsg.ident == cursor->queryToken + 1);

							//Get the overlapped struct associated with the asychronous IOCP call
							nextOverlappedResponse = &nextOverlappedCursor;//(CTOverlappedResponse*)(nextCursorMsg.udata);
							assert(nextOverlappedResponse);
							//Get the cursor/connection that has been pinned on the overlapped
							//nextCursor = (CTCursor*)(nextCursorMsg.lParam);
							nextCursor = (CTCursor*)nextOverlappedResponse->cursor;

							

//#ifdef _DEBUG
							//assert(nextCursorMsg.message == WM_USER + (UINT)cursor->queryToken + 1);
							assert(nextCursor);
//#endif					
							fprintf(stderr, "CX_Dequeue_Recv_Decrypt::cursor = (%lu)\n", (unsigned long)cursor->queryToken);
							fprintf(stderr, "CX_Dequeue_Recv_Decrypt::nextCursor = (%lu)\n", (unsigned long)nextCursor->queryToken);
							
#ifdef CTRANSPORT_SANS_AIO
                                nextCursor->conn->responseCount++;
#endif
                            if (cursor->contentLength < overlappedResponse->buf - cursor->file.buffer)
							{
								fprintf(stderr, "CX_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV::vagina (%ld)\n", (overlappedResponse->buf - cursor->file.buffer));

								cursor->conn->response_overlap_buffer_length = (size_t)overlappedResponse->buf - (size_t)(cursor->file.buffer + cursor->contentLength);
								assert(cursor->conn->response_overlap_buffer_length > 0);

								fprintf(stderr, "CX_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV::buffer overlap (%d)\n", (int)cursor->conn->response_overlap_buffer_length);

								if (nextCursor && cursor->conn->response_overlap_buffer_length > 0)
								{
									NumBytesRemaining = cursor->conn->response_overlap_buffer_length;

									//copy overlapping cursor's response from previous cursor's buffer to next cursor's buffer
									memcpy(nextCursor->file.buffer, cursor->file.buffer + cursor->contentLength, cursor->conn->response_overlap_buffer_length);

									//read the new cursor's header
									assert(nextCursor->headerLength == 0);
									if (nextCursor->headerLength == 0)
									{
										//search for end of protocol header (and set cxCursor headerLength property
										assert(cursor->file.buffer != nextCursor->file.buffer);
										//assert(((ReqlQueryMessageHeader*)nextCursor->file.buffer)->token == nextCursor->queryToken);

										char* endOfHeader = NULL;
										if (cxConn)
										{
											std::shared_ptr<CXCursor> cxNextCursor = cxConn->getRequestCursorForKey(nextCursor->queryToken);
											endOfHeader = cxNextCursor->ProcessResponseHeader(nextCursor, nextCursor->file.buffer, cursor->conn->response_overlap_buffer_length);
										}
										else
											endOfHeader = nextCursor->headerLengthCallback(nextCursor, nextCursor->file.buffer, cursor->conn->response_overlap_buffer_length);

										//calculate size of header
										if (endOfHeader)
										{
											nextCursor->headerLength = endOfHeader - (nextCursor->file.buffer);

											if (nextCursor->headerLength > 0)
											{

												if (cursor->conn->response_overlap_buffer_length < nextCursor->headerLength)
												{
													//leave the incomplete header in place and go around again
													nextCursor->headerLength = 0;
													//NumBytesRemaining = 32768;
												}
												else
												{
													if (nextCursor->requestBuffer != nextCursor->file.buffer )//&& cursor->headerLength > 0)
														memcpy(nextCursor->requestBuffer, nextCursor->file.buffer, nextCursor->headerLength);

													fprintf(stderr, "CX_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV nextCursor->Header = \n\n%.*s\n\n", (int)nextCursor->headerLength, nextCursor->requestBuffer);

													//copy body to start of buffer to overwrite header 
													if (cursor->conn->response_overlap_buffer_length > nextCursor->headerLength && endOfHeader != nextCursor->file.buffer)
													{
														//memcpy(nextCursor->file.buffer, endOfHeader, cursor->conn->response_overlap_buffer_length - nextCursor->headerLength);
														memmove(nextCursor->file.buffer, endOfHeader, cursor->conn->response_overlap_buffer_length - nextCursor->headerLength);
														
														//fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV nextCursor->Body (%lu) = \n\n%.*s\n\n", cursor->conn->response_overlap_buffer_length, (int)nextCursor->contentLength, nextCursor->file.buffer);
														fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV nextCursor->Body (%lu)\n\n", cursor->conn->response_overlap_buffer_length);

													}
													//else if( cursor->conn->response_overlap_buffer_length == nextCursor->headerLength)
													//	NumBytesRemaining = 
													//overlappedResponse->buf += endDecryptedBytes - endOfHeader;

													//fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV Body = \n\n%.*s\n\n", nextCursor->contentLength, nextCursor->file.buffer);


													cursor->conn->response_overlap_buffer_length -= nextCursor->headerLength;
													//extraBufferOffset = cursor->conn->response_overlap_buffer_length; //not used for non-TLS path
													NumBytesRemaining = cursor->conn->response_overlap_buffer_length;
												}
												
												nextCursor->overlappedResponse.buf = nextCursor->file.buffer + cursor->conn->response_overlap_buffer_length;

											}
										}

										//extraBufferOffset = cursor->conn->response_overlap_buffer_length;
										//NumBytesRemaining -= cursor->headerLength;
										//if (nextCursor->queryToken == 99) assert(1 == 0);
										fprintf(stderr, "CX_Deque_Recv_Decrypt::Advance nextCursor->overlappedResponse.buf\n");
									}

								}
							}
							//else
							//	nextCursor = NULL;

							if (nextCursor && NumBytesRemaining == 0)
							{
								fprintf(stderr, "CX_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV::Penis\n");
								//fprintf(stderr, "TO DO: implement nextCursor read more path...\n");
								//assert(1==0);

								assert(nextCursor);
								NumBytesRecv = cBufferSize;;
								overlappedResponse = &(nextCursor->overlappedResponse);
								
								
								fprintf(stderr, "\n\n\n\n\n\n\n\n\n\n (next cursor) READ MORE PATH NumBytesRecv(%lu) NumBytesRemaning (%lu)...\n\n\n\n\n\n\n\n\n\n", NumBytesRecv, NumBytesRemaining);

								//if ((scRet = CTCursorAsyncRecv(&(overlappedResponse), (void*)(nextCursor->file.buffer), 0, &NumBytesRecv)) != CTSocketIOPending)
								if ((scRet = CTCursorAsyncRecv(&(overlappedResponse), (void*)(nextCursor->overlappedResponse.buf/* + cursor->conn->response_overlap_buffer_length - nextCursor->headerLength*/), 0, &NumBytesRecv)) != CTSocketIOPending)
								{
									//the async operation returned immediately
									if (scRet == CTSuccess)
									{
										fprintf(stderr, "CX_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV::CTAsyncRecv (next cursor) returned immediate with %lu bytes\n", NumBytesRecv);
									}
									else //failure
									{
										fprintf(stderr, "CX_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV::CTAsyncRecv (next cursor) failed with CTDriverError = %ld\n", scRet);
										assert(1 == 0);
									}
								}

								nextCursor = NULL;
								//NumBytesRemaining = 0;
								//break;
							}

							else if( nextCursor )
							{
								fprintf(stderr, "CX_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV::Scrotum\n");

//#ifdef _DEBUG
								assert(nextCursor);
								assert(nextCursor->file.buffer);
								//assert(extraBuffer);
//#endif
								//since extra buffer may be pointing to current cursor buffer memory we have to copy it to the next
								//cursor BEFORE closing the current cursor's mapping
								//memcpy(nextCursor->file.buffer + extraBufferOffset, extraBuffer, NumBytesRemaining);
								//extraBuffer = nextCursor->file.buffer + extraBufferOffset;


								overlappedResponse = &(nextCursor->overlappedResponse);
								//overlappedResponse->buf = nextCursor->file.buffer + nextCursor->headerLength;// cursor->conn->response_overlap_buffer_length;// +extraBufferOffset;

								cursor = nextCursor;
								nextCursor = NULL;

							}

						}
						
						//increment the connection response count
						//closeCursor->conn->rxCursorQueueCount--;
						closeCursor->conn->responseCount--;

						//unsubscribe rxQueue from closeCursor's event (only really needed for kqueue sans aio path)
						EV_SET(&kev[0], (uintptr_t )(closeCursor->queryToken), EVFILT_USER, EV_DELETE, 0, 0, NULL);
						kevent(rxQueue, &kev[0], 1, NULL, 0, NULL);

						fprintf(stderr, "close closeCursor->responseCount = %lld\n", closeCursor->conn->responseCount);
						assert(closeCursor->conn->responseCount > -1);

#ifdef _DEBUG
						//assert(closeCursor->responseCallback);
#endif
						//issue the response to the client caller
						if (cxConn) cxConn->distributeResponseWithCursorForToken(closeCursor->queryToken);
						else closeCursor->responseCallback(&err, closeCursor);
						//if (closeCursor->queryToken == 99) assert(1 == 0);
						closeCursor = NULL;	//set to NULL for posterity
					}
					else
					{
						//assert(1==0);
						//NumBytesRecv = NumBytesRemaining;
						//NumBytesRemaining = cBufferSize - NumBytesRecv;
						
						fprintf(stderr, "CX_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV::wanker\n");
						fprintf(stderr, "\n\n\n\n\n\n\n\n\n\n READ MORE PATH NumBytesRecv(%lu) NumBytesRemaning (%lu)...\n\n\n\n\n\n\n\n\n\n", NumBytesRecv, NumBytesRemaining);

						//NumBytesRecv = NumBytesRemaining;

						//if ((scRet = CTCursorAsyncRecv(&overlappedResponse, cursor->file.buffer, NumBytesRecv, &NumBytesRemaining)) != CTSocketIOPending)
						NumBytesRemaining = cBufferSize;// - NumBytesRemaining;
						if ((scRet = CTCursorAsyncRecv(&overlappedResponse, overlappedResponse->buf, 0, &NumBytesRemaining)) != CTSocketIOPending)
						{
							//the async operation returned immediately
							if (scRet == CTSuccess)
							{
	 							fprintf(stderr, "CX_Deque_Recv_Decrypt::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRemaining);
								//scRet = CTSSLDecryptMessage(overlappedResponse->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);
								//continue;
							}
							else //failure
							{
								fprintf(stderr, "CX_Deque_Recv_Decrypt::CTAsyncRecv failed with CTDriverError = %ld\n", scRet);
								assert(1 == 0);
							}
						}
						
						//NumBytesRemaining = 0;
						break;
					}
					//continue;
				}
			}
			else if (overlappedResponse->stage == CT_OVERLAPPED_EXECUTE)//CT_OVERLAPPED_RECV_DECRYPT)
			{
				//assert(1==0);
				//Issue a blocking call to decrypt the message (again, only if using non-zero read path, which is exclusively used with MBEDTLS at current)

				/*
				PrevBytesRecvd = NumBytesRecv;	 //store the recvd bytes value from the overlapped
				NumBytesRemaining = cBufferSize; //we can never decrypt more bytes than the size of our buffer, and likely schannel will still separate that into chunks

				extraBuffer = overlappedResponse->buf;
				NumBytesRemaining = NumBytesRecv;
				scRet = SEC_E_OK;
				*/

				//while their is extra data to decrypt keep calling decrypt
				while (NumBytesRemaining > 0 && scRet == SEC_E_OK)
				{
					//move the extra buffer to the front of our operating buffer
					overlappedResponse->wsaBuf.buf = extraBuffer;
					extraBuffer = NULL;
					PrevBytesRecvd = NumBytesRecv;		//store the previous amount of decrypted bytes
					NumBytesRecv = NumBytesRemaining;	//set up for the next decryption

					scRet = CTSSLDecryptMessage2(cursor->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv, &extraBuffer, &NumBytesRemaining);
					fprintf(stderr, "CX_Deque_Recv_Decrypt::CT_OVERLAPPED_EXECUTE C::Decrypted data: %lu bytes recvd, %lu bytes remaining, scRet = 0x%lx\n", NumBytesRecv, NumBytesRemaining, scRet); //PrintText(NumBytesRecv, (PBYTE)(overlappedResponse->wsaBuf.buf + overlappedResponse->conn->sslContext->Sizes.cbHeader));

					if (scRet == SEC_E_OK)
					{
						//push past cbHeader and copy decrypted message bytes
#if !defined(__APPLE__)
                        memmove((char*)overlappedResponse->buf, (char*)overlappedResponse->wsaBuf.buf + cursor->conn->sslContext->Sizes.cbHeader, NumBytesRecv);
#else
                        memmove((char*)overlappedResponse->buf, (char*)overlappedResponse->wsaBuf.buf, NumBytesRecv);
#endif
						overlappedResponse->buf += NumBytesRecv;//+ overlappedResponse->conn->sslContext->Sizes.cbHeader;// + overlappedResponse->conn->sslContext->Sizes.cbTrailer;

						/* WIN32 WAY
						if (cursor->headerLength == 0)
						{
							//search for end of protocol header (and set cxCursor headerLength property
							char* endOfHeader = cursor->headerLengthCallback(cursor, cursor->file.buffer, NumBytesRecv);

							//calculate size of header
							if (endOfHeader)
							{
								cursor->headerLength = endOfHeader - (cursor->file.buffer);

								memcpy(cursor->requestBuffer, cursor->file.buffer, cursor->headerLength);

								//fprintf(stderr, "CTDecryptResponseCallbacK::Header = \n\n%.*s\n\n", cursor->headerLength, cursor->requestBuffer);

								//copy body to start of buffer to overwrite header 
								memcpy(cursor->file.buffer, endOfHeader, NumBytesRecv - cursor->headerLength);
								//overlappedResponse->buf += endDecryptedBytes - endOfHeader;

								overlappedResponse->buf -= cursor->headerLength;
							}
						}
						*/

						if (cursor->headerLength == 0)
						{
							//search for end of protocol header (and set cxCursor headerLength property
							char* endOfHeader = NULL;

							if (cxConn)
							{
								std::shared_ptr<CXCursor> cxCursor = cxConn->getRequestCursorForKey(cursor->queryToken);
								endOfHeader = cxCursor->ProcessResponseHeader(cursor, cursor->file.buffer, NumBytesRecv);
							}
							else
							{
								endOfHeader = cursor->headerLengthCallback(cursor, cursor->file.buffer, NumBytesRecv);
							}
							
							//calculate size of header
							if (endOfHeader)
							{
								cursor->headerLength = endOfHeader - (cursor->file.buffer);

								fprintf(stderr, "CX_Deque_Recv_Decrypt::CT_OVERLAPPED_EXECUTE Header = \n\n%.*s\n\n", (int)cursor->headerLength, cursor->file.buffer);

								if (cursor->requestBuffer != cursor->file.buffer && cursor->headerLength > 0)
									memcpy(cursor->requestBuffer, cursor->file.buffer, cursor->headerLength);

								//fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV Header (%llu) = \n\n%.*s\n\n", cursor->headerLength, cursor->requestBuffer);

								//copy body to start of buffer to overwrite header 
								if (endOfHeader != cursor->file.buffer)
								{
									if (NumBytesRecv == cursor->headerLength)
										memset(cursor->file.buffer, 0, cursor->headerLength);
									else
									{
										//memcpy(cursor->file.buffer, endOfHeader, NumBytesRecv - cursor->headerLength);
										memmove( cursor->file.buffer, endOfHeader, NumBytesRecv - cursor->headerLength); 
										//fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV Body (%lu) = \n\n%.*s\n\n", cursor->contentLength, (int)cursor->contentLength, cursor->file.buffer);
										fprintf(stderr, "CX_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV Body (%lu)\n\n", cursor->contentLength);

									}
								}
								overlappedResponse->buf -= cursor->headerLength;
								//NumBytesRemaining -= cursor->headerLength;			//this line is only relevant for non-TLS path
							}
						}

						if (cursor->headerLength >  0) assert(cursor->contentLength > 0);

						fprintf(stderr, "CX_Deque_Recv_Decrypt::Check cursor->contentLength (%d, %d) \n\n", (int)cursor->contentLength, (int)(overlappedResponse->buf - cursor->file.buffer));
						if (cursor->contentLength > 0 && ((int)cursor->contentLength) <= (int)(overlappedResponse->buf - cursor->file.buffer))
						{
							extraBufferOffset = 0;
							closeCursor = cursor;
							fprintf(stderr, "Copy closeCursor->responseCount = %lld\n", closeCursor->conn->responseCount);
							//NumBytesRemaining = 0; //this line is different from win32 implementation
							nextCursor = NULL;
							nextOverlappedResponse = NULL;
							
							memset(&nextCursorMsg, 0, sizeof(struct kevent));
							//nextCursorMsg.filter = EVFILT_USER;
							//nextCursorMsg.ident = (uintptr_t )(cursor->queryToken + 1);

							fprintf(stderr, "\nCX_Dequeue_Recv_Decrypt::Waiting for nextCursor event on pipe\n");

							/*
							//Synchronously wait for a cursor  event by idling until we receive kevent from our kqueue
							if( (ret = kqueue_wait_with_timeout(oxQueue, &nextCursorMsg, 1, 0)) < 0 )
							{
								fprintf(stderr, "\nCT_Dequeue_Recv_Decrypt::kqueue_wait_with_timeout nextCursorMsg failed with ret (%d).\n", ret);
								assert(1==0);
							}
							*/

							/* Grab any events written to the pipe from the crTimeServer */
							if( (ret = kqueue_wait_with_timeout(pendingCursorQueue, &nextCursorMsg, 1, 0)) < 0 )
							{
								fprintf(stderr, "\nCX_Dequeue_Recv_Decrypt::kqueue_wait_with_timeout nextCursorMsg failed with ret (%d).\n", ret);
								assert(1==0);
							}
							//if (PeekMessage(&nextCursorMsg, NULL, WM_USER, WM_USER + cursor->conn->queryCount - 1, PM_REMOVE)) //(nextCursor)
							if( ret == 1)
							{
								//assert(1==0);

								assert( nextCursorMsg.ident == rxPipe[CT_INCOMING_PIPE]);		
								ssize_t bytes = read((int)nextCursorMsg.ident, &nextOverlappedCursor, sizeof(CTOverlappedResponse));
								if( bytes == -1 )
								{
									fprintf(stderr, "\nCX_Dequeue_Recv_Decrypt::Read from overlapped pipe (nextCursorMsg) failed with error %d\n", errno);
									assert(1==0);
								}
								//fprintf(stderr, "\nCT_Dequeue_Recv_Decrypt::Read %d bytes from overlapped pipe \n", (int)bytes);
								assert(bytes == sizeof(CTOverlappedResponse));
								//overlappedResponse = &overlappedCursor;
								//assert(overlappedResponse);
								//cursor = (CTCursor*)overlappedResponse->cursor;
								//assert(cursor);
								//fprintf(stderr, "\nCT_Dequeue_Recv_Decrypt::Read cursor (%lu) from oxPipe\n", cursor->queryToken);


								//assert(nextCursorMsg.filter == EVFILT_USER && nextCursorMsg.ident == cursor->queryToken + 1);

								//Get the overlapped struct associated with the asychronous IOCP call
								nextOverlappedResponse = &nextOverlappedCursor;//(CTOverlappedResponse*)(nextCursorMsg.udata);
								assert(nextOverlappedResponse);
								//Get the cursor/connection that has been pinned on the overlapped
								//nextCursor = (CTCursor*)(nextCursorMsg.lParam);
								nextCursor = (CTCursor*)nextOverlappedResponse->cursor;

								//assert(nextCursorMsg.message == WM_USER + (UINT)cursor->queryToken + 1);
								assert(nextCursor);

								fprintf(stderr, "CX_Dequeue_Recv_Decrypt::cursor = (%lu)\n", (unsigned long)cursor->queryToken);
								fprintf(stderr, "CX_Dequeue_Recv_Decrypt::nextCursor = (%lu)\n", (unsigned long)nextCursor->queryToken);

#ifdef CTRANSPORT_SANS_AIO
                                nextCursor->conn->responseCount++;
#endif
                                if (cursor->contentLength < overlappedResponse->buf - cursor->file.buffer)
								{
									fprintf(stderr, "CX_Deque_Recv_Decrypt::CT_OVERLAPPED_EXECUTE::vagina (%ld)\n", (overlappedResponse->buf - cursor->file.buffer));

									cursor->conn->response_overlap_buffer_length = (size_t)overlappedResponse->buf - (size_t)(cursor->file.buffer + cursor->contentLength);
									assert(cursor->conn->response_overlap_buffer_length > 0);

									fprintf(stderr, "CX_Deque_Recv_Decrypt::CT_OVERLAPPED_EXECUTE::buffer overlap (%d)\n", (int)cursor->conn->response_overlap_buffer_length);

									if (nextCursor && cursor->conn->response_overlap_buffer_length > 0)
									{
										//NumBytesRemaining = cursor->conn->response_overlap_buffer_length; //this line only exists in non-TLS implementation

										//copy overlapping cursor's response from previous cursor's buffer to next cursor's buffer
										memcpy(nextCursor->file.buffer, cursor->file.buffer + cursor->contentLength, cursor->conn->response_overlap_buffer_length);

										//read the new cursor's header
										assert(nextCursor->headerLength == 0); //different from WIN32
										if (nextCursor->headerLength == 0)
										{
											//search for end of protocol header (and set cxCursor headerLength property
											assert(cursor->file.buffer != nextCursor->file.buffer);
											//assert(((ReqlQueryMessageHeader*)nextCursor->file.buffer)->token == nextCursor->queryToken);

											char* endOfHeader = NULL;
											if (cxConn)
											{
												std::shared_ptr<CXCursor> cxNextCursor = cxConn->getRequestCursorForKey(nextCursor->queryToken);
												endOfHeader = cxNextCursor->ProcessResponseHeader(nextCursor, nextCursor->file.buffer, cursor->conn->response_overlap_buffer_length);
											}
											else
												endOfHeader = nextCursor->headerLengthCallback(nextCursor, nextCursor->file.buffer, cursor->conn->response_overlap_buffer_length);

											//calculate size of header
											if (endOfHeader)
											{
												nextCursor->headerLength = endOfHeader - (nextCursor->file.buffer);

												if (nextCursor->headerLength > 0)
												{

													if (cursor->conn->response_overlap_buffer_length < nextCursor->headerLength)
													{
														//leave the incomplete header in place and go around again
														nextCursor->headerLength = 0;
														//NumBytesRemaining = 32768;
													}
													else
													{
														//copy header to cursor's request buffer
														if (nextCursor->requestBuffer != nextCursor->file.buffer )//&& cursor->headerLength > 0)
															memcpy(nextCursor->requestBuffer, nextCursor->file.buffer, nextCursor->headerLength);

														fprintf(stderr, "CX_Deque_Recv_Decrypt::CT_OVERLAPPED_EXECUTE nextCursor->Header = \n\n%.*s\n\n", (int)nextCursor->headerLength, nextCursor->requestBuffer);

														//copy body to start of buffer to overwrite header 
														if (cursor->conn->response_overlap_buffer_length > nextCursor->headerLength && endOfHeader != nextCursor->file.buffer)
														{
															//memcpy(nextCursor->file.buffer, endOfHeader, cursor->conn->response_overlap_buffer_length - nextCursor->headerLength);
															memmove(nextCursor->file.buffer, endOfHeader, cursor->conn->response_overlap_buffer_length - nextCursor->headerLength);
															
															//fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV nextCursor->Body (%lu) = \n\n%.*s\n\n", cursor->conn->response_overlap_buffer_length, (int)nextCursor->contentLength, nextCursor->file.buffer);
															fprintf(stderr, "CX_Deque_Recv_Decrypt::CT_OVERLAPPED_EXECUTE nextCursor->Body (%lu)\n\n", cursor->conn->response_overlap_buffer_length);

														}
														//else if( cursor->conn->response_overlap_buffer_length == nextCursor->headerLength)
														//	NumBytesRemaining = 
														//overlappedResponse->buf += endDecryptedBytes - endOfHeader;

														//fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV Body = \n\n%.*s\n\n", nextCursor->contentLength, nextCursor->file.buffer);


														cursor->conn->response_overlap_buffer_length -= nextCursor->headerLength;
														extraBufferOffset = cursor->conn->response_overlap_buffer_length;	//this line is only relevant for TLS path
														//NumBytesRemaining = cursor->conn->response_overlap_buffer_length; //this line is only relevant for non-TLS path

													}
													
													//nextCursor->overlappedResponse.buf = nextCursor->file.buffer + cursor->conn->response_overlap_buffer_length; //this line is only relevant for non-TLS path
												}
											}

											//extraBufferOffset = cursor->conn->response_overlap_buffer_length;
											//NumBytesRemaining -= cursor->headerLength;
											//if (nextCursor->queryToken == 99) assert(1 == 0);
											fprintf(stderr, "CX_Deque_Recv_Decrypt::Advance nextCursor->overlappedResponse.buf\n");


										}

									}
								}

								if (nextCursor && NumBytesRemaining == 0)
								{
									fprintf(stderr, "CX_Deque_Recv_Decrypt::CT_OVERLAPPED_EXECUTE::Penis\n");
									//assert(1==0);

									assert(nextCursor);
									NumBytesRecv = cBufferSize;;
									overlappedResponse = &(nextCursor->overlappedResponse);

									fprintf(stderr, "\n\n\n\n\n\n\n\n\n\n (next cursor) READ MORE PATH NumBytesRecv(%lu) NumBytesRemaning (%lu)...\n\n\n\n\n\n\n\n\n\n", NumBytesRecv, NumBytesRemaining);

									//if ((scRet = CTCursorAsyncRecv(&(overlappedResponse), (void*)(nextCursor->file.buffer), 0, &NumBytesRecv)) != CTSocketIOPending)
									if ((scRet = CTCursorAsyncRecv(&(overlappedResponse), (void*)(nextCursor->overlappedResponse.buf/* + cursor->conn->response_overlap_buffer_length - nextCursor->headerLength*/), 0, &NumBytesRecv)) != CTSocketIOPending)
									{
										//the async operation returned immediately
										if (scRet == CTSuccess)
										{
											fprintf(stderr, "CX_Deque_Recv_Decrypt::CT_OVERLAPPED_EXECUTE::CTAsyncRecv (next cursor) returned immediate with %lu bytes\n", NumBytesRecv);
										}
										else //failure
										{
											fprintf(stderr, "CX_Deque_Recv_Decrypt::CT_OVERLAPPED_EXECUTE::CTAsyncRecv (next cursor) failed with CTDriverError = %ld\n", scRet);
											assert(1 == 0);
										}
									}

									nextCursor = NULL;
									//NumBytesRemaining = 0;
									//break;
								}
								else if( nextCursor )
								{
									fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_EXECUTE::Scrotum\n");

//#ifdef _DEBUG
									assert(nextCursor);
									assert(nextCursor->file.buffer);
									assert(extraBuffer);
//#endif
									//since extra buffer may be pointing to current cursor buffer memory we have to copy it to the next
									//cursor BEFORE closing the current cursor's mapping
									memcpy(nextCursor->file.buffer + extraBufferOffset, extraBuffer, NumBytesRemaining);
									extraBuffer = nextCursor->file.buffer + extraBufferOffset;

									overlappedResponse = &(nextCursor->overlappedResponse);
									overlappedResponse->buf = nextCursor->file.buffer + extraBufferOffset;

									cursor = nextCursor;
									nextCursor = NULL;
								}

							}

							//increment the connection response count
							//closeCursor->conn->rxCursorQueueCount--;
							closeCursor->conn->responseCount--;

							//unsubscribe rxQueue from closeCursor's event (only really needed for kqueue sans aio path)
							EV_SET(&kev[0], (uintptr_t )(closeCursor->queryToken), EVFILT_USER, EV_DELETE, 0, 0, NULL);
							kevent(rxQueue, &kev[0], 1, NULL, 0, NULL);

							fprintf(stderr, "close closeCursor->responseCount = %lld\n", closeCursor->conn->responseCount);
							assert(closeCursor->conn->responseCount > -1);

#ifdef _DEBUG
							//assert(closeCursor->responseCallback);
#endif
							//issue the response to the client caller
							if (cxConn) cxConn->distributeResponseWithCursorForToken(closeCursor->queryToken);
							else closeCursor->responseCallback(&err, closeCursor);							
							//if (closeCursor->queryToken == 99) assert(1 == 0);
							closeCursor = NULL;	//set to NULL for posterity
						}
						else if (NumBytesRemaining == 0)
						{
							//assert(1==0);
							//NumBytesRecv = NumBytesRemaining;
							//NumBytesRemaining = cBufferSize - NumBytesRecv;
							
							fprintf(stderr, "CX_Deque_Recv_Decrypt::CT_OVERLAPPED_EXECUTE::wanker\n");
							fprintf(stderr, "\n\n\n\n\n\n\n\n\n\n READ MORE PATH NumBytesRecv(%lu) NumBytesRemaning (%lu)...\n\n\n\n\n\n\n\n\n\n", NumBytesRecv, NumBytesRemaining);

							//NumBytesRecv = NumBytesRemaining;

							//if ((scRet = CTCursorAsyncRecv(&overlappedResponse, cursor->file.buffer, NumBytesRecv, &NumBytesRemaining)) != CTSocketIOPending)
							NumBytesRemaining = cBufferSize;// - NumBytesRemaining;
							if ((scRet = CTCursorAsyncRecv(&overlappedResponse, overlappedResponse->buf, 0, &NumBytesRemaining)) != CTSocketIOPending)
							{
								//the async operation returned immediately
								if (scRet == CTSuccess)
								{
									fprintf(stderr, "CX_Deque_Recv_Decrypt::CT_OVERLAPPED_EXECUTE::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRemaining);
									//scRet = CTSSLDecryptMessage(overlappedResponse->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);
									//continue;
								}
								else //failure
								{
									fprintf(stderr, "CX_Deque_Recv_Decrypt::CT_OVERLAPPED_EXECUTE::CTAsyncRecv failed with CTDriverError = %ld\n", scRet);
									assert(1 == 0);
								}
							}
							
							//NumBytesRemaining = 0;
							break;
						}
						//continue;
					}
				}

				if (scRet == SEC_E_INCOMPLETE_MESSAGE)
				{
					//assert(1==0);
					fprintf(stderr, "CX_Deque_Recv_Decrypt::SEC_E_INCOMPLETE_MESSAGE %lu prev bytes recvd, %lu bytes recv, %lu bytes remaining\n", PrevBytesRecvd, NumBytesRecv, NumBytesRemaining); //PrintText(NumBytesRecv, (PBYTE)(overlappedResponse->wsaBuf.buf + overlappedResponse->conn->sslContext->Sizes.cbHeader));

					//
					//	Handle INCOMPLETE ENCRYPTED MESSAGE AS FOLLOWS:
					// 
					//	1)  copy the remaining encrypted bytes to the start of our buffer so we can 
					//		append to them when receiving from socket on the next async iocp iteration
					//  
					//	2)  use NumBytesRecv var as an offset input parameter CTCursorAsyncRecv
					//
					//	3)  use NumByteRemaining var as input to CTCursorAsyncRecv to request num bytes to recv from socket
					//		(PrevBytesRecvd will be subtracted from NumBytesRemaining internal to CTAsyncRecv

					//For SCHANNEL:  CTSSLDecryptMessage2 will point extraBuffer at the input msg buffer
					//For WolfSSL:   CTSSLDecryptMessage2 will point extraBuffer at the input msg buffer + the size of the ssl header wolfssl already consumed
					memmove((char*)overlappedResponse->buf, extraBuffer, NumBytesRecv);
					NumBytesRemaining = cBufferSize;   //request exactly the amount to complete the message NumBytesRemaining 
													   //populated by CTSSLDecryptMessage2 above or request more if desired

					if ((scRet = CTCursorAsyncRecv(&overlappedResponse, overlappedResponse->buf, NumBytesRecv, &NumBytesRemaining)) != CTSocketIOPending)
					{
						//the async operation returned immediately
						if (scRet == CTSuccess)
						{
							//fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRemaining);
							//scRet = CTSSLDecryptMessage(overlappedResponse->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);
						}
						else //failure
						{
							fprintf(stderr, "CXConnection::DecryptResponseCallback::CTAsyncRecv failed with CTDriverError = %ld\n", scRet);
							break;
						}
					}
				}

			}

		}
		else
			assert(1==0);//timeout
		


	}


	fprintf(stderr, "\nCX_Dequeue_Recv_Decrypt::End\n");
	
	//returning will automatically call pthread_exit with return value
	//pthread_exit(0);
	return lpParameter;

}



void* __stdcall CX_Dequeue_Encrypt_Send(LPVOID lpParameter)
{
	uint64_t queryToken;
	//uintptr_t event;
	CTCursor* cursor = NULL;
	CTOverlappedResponse* overlappedRequest = NULL;
	CTOverlappedResponse* overlappedResponse = NULL;
	

	unsigned long NumBytesSent = 0;
	unsigned long recvMsgLength = 0;

	CTSSLStatus scRet = 0;
	CTClientError ctError = CTSuccess;
	CTError err = { (CTErrorClass)0,0,NULL };
	//struct timespec _timespec = {0,0};

	//we passed the kqueue handle as the lp parameter
	CTKernelQueue *tq = ((CTKernelQueue*)lpParameter);
	CTKernelQueueType txQueue = tq->kq;
	//CTKernelQueue oxQueue = 0;//*((CTKernelQueue*)lpParameter);

	unsigned long cBufferSize = ct_system_allocation_granularity();

	fprintf(stderr, "\nCT_Dequeue_Encrypt_Send(kqueue)::Begin\n");

	while( 1)
	{
		struct kevent kev = {0};
		//EV_SET( &kev, 0, EVFILT_USER, 0, 0, 0, NULL );

		fprintf(stderr, "\nCX_Dequeue_Encrypt_Send(kqueue)::Waiting for cursor\n");

		//kev.filter = EVFILT_USER;
    	//kev.ident = CTKQUEUE_OVERLAPPED_EVENT;
		//Synchronously wait for the socket cursor event by idling until we receive kevent from our kqueue
    	kqueue_wait_with_timeout(txQueue, &kev, 1, CTSOCKET_MAX_TIMEOUT);
		//event = CTKernelQueueWait(rxQueue, EVFILT_READ);

		if( kev.ident != CTSOCKET_EVT_TIMEOUT  )
		{
			if( kev.filter != EVFILT_USER)
			{
				fprintf(stderr, "\nCX_Dequeue_Encrypt_Send(kqueue) continue\n");
				continue;
			}
			
			overlappedRequest = (CTOverlappedResponse*)(kev.udata);
			assert(overlappedRequest);
			//NumBytesSent = ovl_entry[i].dwNumberOfBytesTransferred;
			NumBytesSent = overlappedRequest->len;

			int schedule_stage = CT_OVERLAPPED_SCHEDULE;
			if (!overlappedRequest)
			{
				fprintf(stderr, "\nCX_Dequeue_Encrypt_Send(kqueue) continue\n");
				assert(1==0);
			}

			if (NumBytesSent == 0) //TO DO:  what does this if statement even mean when not the result of a send?
			{
				fprintf(stderr, "\nCX_Dequeue_Encrypt_Send(kqueue)::Server Disconnected!!!\n");
				assert(1==0);
			}
			else
			{
				//Get the cursor/connection
				//Get a the CTCursor from the overlappedRequest struct, retrieve the overlapped response struct memory, and store reference to the CTCursor in the about-to-be-queued overlappedResponse
				cursor = (CTCursor*)(overlappedRequest->cursor);
	
#ifdef _DEBUG
				assert(cursor);
				assert(cursor->conn);
#endif
				//unsubscribe txQueue from event as soon as possible
				EV_SET(&kev, (uintptr_t )(cursor->queryToken), EVFILT_USER, EV_DELETE, 0, 0, NULL);
				kevent(cursor->conn->socketContext.txQueue, &kev, 1, NULL, 0, NULL);

				NumBytesSent = overlappedRequest->len;
				//fprintf(stderr, "CTConnection::EncryptQueryCallback::NumBytesSent = %d\n", (int)NumBytesSent);
				//fprintf(stderr, "CTConnection::EncryptQueryCallback::msg = %s\n", overlappedRequest->buf + sizeof(ReqlQueryMessageHeader));

				//Use the ReQLC API to synchronously
				//a)	encrypt the query message in the query buffer
				//b)	send the encrypted query data over the socket
				if (overlappedRequest->stage != CT_OVERLAPPED_SEND)
					scRet = CTSSLWrite(cursor->conn->socket, cursor->conn->sslContext, overlappedRequest->buf, &NumBytesSent);
				else
				{
					//TO DO:  create a send callback cursor attachment to remove post send TLS handshake logic here					
					//assert(1 == 0);
					ssize_t cbData = send(cursor->conn->socket, (char*)overlappedRequest->buf, overlappedRequest->len, 0);

#ifdef _WIN32
					if (cbData == SOCKET_ERROR || cbData == 0)
#else
					if (cbData <= 0)
#endif
					{
						fprintf(stderr, "**** CX_Dequeue_Encrypt_Send(kqueue)::CT_OVERLAPPED_SCHEDULE_SEND Error %d sending data to server (%ld)\n", CTSocketError(), cbData);
						assert(1==0);
					}

                    fprintf(stderr, "CX_Dequeue_Encrypt_Send::%ld bytes of handshake data sent\n", cbData);

					//we haven't populated the cursor's overlappedResponse for recv'ing yet,
					//so use it here to pass the overlappedRequest->buf to the queryCallback (for instance, bc SCHANNEL needs to free handshake memory)
					cursor->overlappedResponse.buf = overlappedRequest->buf;
					err.id = (int)cbData;
					if (cursor->queryCallback)
						cursor->queryCallback(&err, cursor);

					cursor->overlappedResponse.buf = NULL;
					overlappedRequest->buf = NULL; //should this stay here or in the queryCallback?
					schedule_stage = CT_OVERLAPPED_SCHEDULE_RECV;

					/*
					//non-blocking send
					//int cbData = send(cursor->conn->socket, (char*)overlappedRequest->buf, overlappedRequest->len, 0);
					unsigned long send_length = overlappedRequest->len;
					int cbData = CTSend(cursor->conn, (char*)(overlappedRequest->buf), &send_length);
					if (cbData != 0)
					{
						fprintf(stderr, "**** CT_Dequeue_Encrypt_Send(kqueue)::CT_OVERLAPPED_SCHEDULE_SEND Error %d sending data to server (%d)\n", CTSocketError(), cbData);
						//assert(1==0);
					}
					
					assert(send_length == overlappedRequest->len);

					//we haven't populated the cursor's overlappedResponse for recv'ing yet,
					//so use it here to pass the overlappedRequest->buf to the queryCallback (for instance, bc SCHANNEL needs to free handshake memory)
					cursor->overlappedResponse.buf = overlappedRequest->buf;
					err.id = cbData;
					if (cursor->queryCallback)
						cursor->queryCallback(&err, cursor);

					fprintf(stderr, "%lu bytes of data sent\n", send_length);
					cursor->overlappedResponse.buf = NULL;
					overlappedRequest->buf = NULL; //should this stay here or in the queryCallback?
					schedule_stage = CT_OVERLAPPED_SCHEDULE_RECV;
					*/
				}

				//However, the zero buffer read doesn't seem to scale very well if Schannel is encrypting/decrypting for us 
				recvMsgLength = cBufferSize;
				queryToken = cursor->queryToken;
				//fprintf(stderr, "CTConnection::EncryptQueryCallback::queryTOken = %llu\n", queryToken);

				//Issue a blocking read for debugging purposes
				//CTRecv(overlappedQuery->conn, (void*)&(overlappedQuery->conn->response_buffers[ (queryToken%REQL_MAX_INFLIGHT_QUERIES) * REQL_RESPONSE_BUFF_SIZE]), &recvMsgLength);

				//schedule the overlapped response cursor for reading on the rxQueue
				overlappedResponse = &(cursor->overlappedResponse);
				overlappedResponse->cursor = (void*)cursor;
				overlappedResponse->len = cBufferSize;
				overlappedResponse->stage = (CTOverlappedStage)schedule_stage;
				
				if( cursor->conn->object_wrapper || cursor->headerLengthCallback != 0 )
					CTCursorRecvOnQueue(&overlappedResponse, (void*)(cursor->file.buffer), 0, &recvMsgLength);
                
                //TO DO:  How and when to break out of this loop?

			}
			
		}
		else
			assert(1==0);//timeout
		
	}

	fprintf(stderr, "\nCX_Dequeue_Encrypt_Send::End\n");
	
	//returning will automatically call pthread_exit with return value
	//pthread_exit(0);
	return lpParameter;

}


#else

unsigned long __stdcall CX_Dequeue_Resolve_Connect_Handshake(LPVOID lpParameter)
{
	int i;
	OSStatus status;
	ULONG_PTR CompletionKey;	//when to use completion key?
	OVERLAPPED_ENTRY ovl_entry[CX_MAX_INFLIGHT_CONNECT_PACKETS];
	ULONG nPaquetsDequeued;

	ULONG_PTR dwCompletionKey = (ULONG_PTR)NULL;

	CTCursor* cursor = NULL;
	CTOverlappedTarget* overlappedTarget = NULL;
	//CTOverlappedResponse* overlappedResponse = NULL;

	unsigned long NumBytesTransferred = 0;

	//we passed the completion port handle as the lp parameter
	CTKernelQueueType hCompletionPort = *(CTKernelQueueType*)lpParameter;
	CTConnection* conn = NULL;
	//unsigned long cBufferSize = ct_system_allocation_granularity();

	CTCursor* closeCursor = NULL;
	CTError err = { (CTErrorClass)0,0,NULL };

	fprintf(stderr, "CX_Dequeue_Resolve_Connect_Handshake Begin\n");

	//TO DO:  Optimize number of packets dequeued at once
	while (1)
	{
		BOOL result = GetQueuedCompletionStatusEx(hCompletionPort, ovl_entry, CX_MAX_INFLIGHT_CONNECT_PACKETS, &nPaquetsDequeued, INFINITE, TRUE);
		//fprintf(stderr, "CTConnection::EncryptQueryCallback::nPaquetsDequeued = %d\n", (int)nPaquetsDequeued);
		for (i = 0; i < nPaquetsDequeued; i++)
		{
			CompletionKey = (ULONG_PTR)ovl_entry[i].lpCompletionKey;
			NumBytesTransferred = ovl_entry[i].dwNumberOfBytesTransferred;
			overlappedTarget = (CTOverlappedTarget*)ovl_entry[i].lpOverlapped;

			if (!overlappedTarget)
			{
				fprintf(stderr, "CTConnection::CTConnectThread::GetQueuedCompletionStatus continue\n");
				continue;
			}

			fprintf(stderr, "CTConnection::CTConnectThread::Overlapped Target Packet Dequeued!!!\n");

			if (overlappedTarget->stage == CT_OVERLAPPED_SCHEDULE)
			{
				//This will resolve the host and kick off the async connection
				//TO DO: make the resolve async
				//overlappedTarget->target->ctx = overlappedTarget; // store the overlapped for subsquent async connection execution
				fprintf(stderr, "CTConnection::CTConnectThread::CT_OVERLAPPED_SCHEDULE\n");
				fflush(stderr);
				CTTargetResolveHost(overlappedTarget->target, overlappedTarget->target->callback);
				fprintf(stderr, "CTConnection::CTConnectThread::End CTTargetResolveHost\n");
				fflush(stderr);
			}
			else if (overlappedTarget->stage == CT_OVERLAPPED_RECV_FROM)
			{
				fprintf(stderr, "CTConnection::CTConnectThread::CT_OVERLAPPED_RECV_FROM\n");
				fflush(stderr);

				//assert(1 == 0);
				cursor = overlappedTarget->cursor;
				cursor->target = overlappedTarget->target;
				//cursor->target->ctx = overlappedTarget;
				overlappedTarget->cursor->overlappedResponse.buf += NumBytesTransferred;
				if (cursor->headerLength == 0)
				{
					//search for end of protocol header (and set cxCursor headerLength property
					char* endOfHeader = cursor->headerLengthCallback(cursor, cursor->file.buffer, NumBytesTransferred);

					//calculate size of header
					if (endOfHeader)
					{
						cursor->headerLength = endOfHeader - (cursor->file.buffer);

						memcpy(cursor->requestBuffer, cursor->file.buffer, cursor->headerLength);

						fprintf(stderr, "CTDecryptResponseCallback::Header = \n\n%.*s\n\n", (int)cursor->headerLength, cursor->requestBuffer);

						//copy body to start of buffer to overwrite header 
						memcpy(cursor->file.buffer, endOfHeader, NumBytesTransferred - cursor->headerLength);
						//overlappedResponse->buf += endDecryptedBytes - endOfHeader;

						overlappedTarget->cursor->overlappedResponse.buf -= cursor->headerLength;
					}
				}
				assert(cursor->contentLength > 0);

				if (cursor->contentLength <= overlappedTarget->cursor->overlappedResponse.buf - cursor->file.buffer)
				{
					closeCursor = cursor;
					//increment the connection response count
							//closeCursor->conn->rxCursorQueueCount--;
					//closeCursor->conn->responseCount++;

#ifdef _DEBUG
					assert(closeCursor->responseCallback);
#endif
					//issue the response to the client caller
					closeCursor->responseCallback(&err, closeCursor);
					closeCursor = NULL;	//set to NULL for posterity

				}

				continue;
			}
			else if (overlappedTarget->stage == CT_OVERLAPPED_EXECUTE)
			{
				//assert(result == TRUE);
				//This will get the result of the async connection and perform ssl hanshake

				fprintf(stderr, "ConnectEx complete error = %d\n\n", WSAGetLastError());
				//struct CTConnection conn = { 0 };
				//get a pointer to a new connection object memory
				struct CTError error = { (CTErrorClass)0,0,0 };    //Reql API client functions will generally return ints as errors

				//Check for socket error to ensure that the connection succeeded
				if ((error.id = CTSocketGetError(overlappedTarget->target->socket)) != 0)
				{
					// connection failed; error code is in 'result'
					fprintf(stderr, "CTConnection::CTConnectThread::socket async connect failed with result: %d", error.id);
					CTSocketClose(overlappedTarget->target->socket);
					error.id = CTSocketConnectError;
					goto CONN_CALLBACK;
				}

				/* Make the socket more well-behaved.
			   int rc = setsockopt(overlappedTarget->target->socket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
			   if (rc != 0) {
				   fprintf(stderr, "SO_UPDATE_CONNECT_CONTEXT failed: %d\n", WSAGetLastError());
				   return 1;
			   }
			   */

				conn = CTGetNextPoolConnection();
				memset(conn, 0, sizeof(CTConnection));

				conn->socket = overlappedTarget->target->socket;
				conn->socketContext.socket = overlappedTarget->target->socket;
				conn->socketContext.host = overlappedTarget->target->url.host;






				//For Loading
				IO_STATUS_BLOCK iostatus;
				pNtSetInformationFile NtSetInformationFile = NULL;

				//For Executing
				FILE_COMPLETION_INFORMATION  socketCompletionInfo = { NULL, NULL };// { overlappedTarget->target->cxQueue, dwCompletionKey };
				//ULONG fcpLen = sizeof(FILE_COMPLETION_INFORMATION);

				//Load library if needed...
				//	ntdll = LoadLibrary("ntdll.dll");
				//	if (ntdll == NULL) {
				//		return 0;
				//	}

				//LoadNtSetInformation function ptr from ntdll.lib
				NtSetInformationFile = (pNtSetInformationFile)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtSetInformationFile");
				if (NtSetInformationFile == NULL) {
					return 0;
				}

				//remove the existing completion port attache to socket handle used for connection
				if (NtSetInformationFile((HANDLE)(conn->socket), &iostatus, &socketCompletionInfo, sizeof(FILE_COMPLETION_INFORMATION), FileReplaceCompletionInformation) < 0) {
					return 0;
				}

				//Copy cx, tx, rx completion port queue from target to connection and attach them to the socket handle for async writing and async reading, respectively
				//We assume that since we are executing on a cxQueue placed on the target by the user, they will have also specified their own txQueue, rxQueue as well
				conn->socketContext.cq = overlappedTarget->target->cq; //CTSSLRoutine needs to see a connection queue in order to enter async vs sync handshake pipeline
				conn->socketContext.tq = overlappedTarget->target->tq;
				conn->socketContext.rq = overlappedTarget->target->rq;

				//Associate the socket with the dedicated read queue
				CreateIoCompletionPort((HANDLE)conn->socket, conn->socketContext.rxQueue, dwCompletionKey, 1);


				//There is some short order work that would be good to parallelize regarding loading the cert and saving it to keychain
				//However, the SSLHandshake loop has already been optimized with yields for every asynchronous call

#ifdef DEBUG
				fprintf(stderr, "Before CTSSLRoutine Host = %s\n", conn.socketContext.host);
				fprintf(stderr, "Before CTSSLRoutine Host = %s\n", overlappedTarget->target->host);
#endif

				//put the connection callback on the service so we can ride it through the async handshake
				conn->target = overlappedTarget->target;

				if (conn->target->proxy.host)
				{
					//Initate the Proxy Handshake scheduling from this thread (ie cxQueue) to rxQueue and txQueue threads
					if ((status = CTProxyHandshake(conn)) != 0)
					{
						fprintf(stderr, "CTConnection::CTConnectThread::CTSSLRoutine failed with error: %d\n", (int)status);
						error.id = CTSSLHandshakeError;
						goto CONN_CALLBACK;
						//return status;
					}

					//if we are running the handshake asynchronously on a queue, don't return connection to client yet
					if (conn->socketContext.txQueue)
						continue;
				}
				else if (conn->target->ssl.method > CTSSL_NONE)
				{

					//Initate the Async SSL Handshake scheduling from this thread (ie cxQueue) to rxQueue and txQueue threads
					if ((status = CTSSLRoutine(conn, conn->socketContext.host, overlappedTarget->target->ssl.ca)) != 0)
					{
						fprintf(stderr, "CTConnection::CTConnectThread::CTSSLRoutine failed with error: %d\n", (int)status);
						error.id = CTSSLHandshakeError;
						goto CONN_CALLBACK;
						//return status;
					}

					//if we are running the handshake asynchronously on a queue, don't return connection to client yet
					if (conn->socketContext.txQueue)
						continue;
				}
				/*
				int iResult;

				u_long iMode = 1;

				//-------------------------
				// Set the socket I/O mode: In this case FIONBIO
				// enables or disables the blocking mode for the
				// socket based on the numerical value of iMode.
				// If iMode = 0, blocking is enabled;
				// If iMode != 0, non-blocking mode is enabled.

				iResult = ioctlsocket(conn.socket, FIONBIO, &iMode);
				if (iResult != NO_ERROR)
					fprintf(stderr, "ioctlsocket failed with error: %ld\n", iResult);
				*/

				//The connection completed successfully, allocate memory to return to the client
				error.id = CTSuccess;

			CONN_CALLBACK:
				//fprintf(stderr, "before callback!\n");
				conn->responseCount = 0;  //we incremented this for the handshake, it is critical to reset this for the client before returning the connection
				conn->queryCount = 0;
				//CTSetCursorPoolIndex(0);
				overlappedTarget->target->callback(&error, conn);
				//fprintf(stderr, "After callback!\n");

				/*
				//Remove observer that yields to coroutines
				CFRunLoopRemoveObserver(runLoop, runLoopObserver, kCFRunLoopDefaultMode);
				//Remove dummy source that keeps run loop alive
				CFRunLoopRemoveSource(runLoop, runLoopSource, kCFRunLoopDefaultMode);

				//release runloop object memory
				CFRelease(runLoopObserver);
				CFRelease(runLoopSource);
				*/

				//return the error id
				//return error.id;

			}
			else
			{
				fprintf(stderr, "CTConnection::CTConnectThread::UNHANDLED_STAGE (%d)\n", overlappedTarget->stage);
				fflush(stderr);
			}
			/*
			if (NumBytesSent == 0)
			{
				fprintf(stderr, "CTConnection::CTConnectThread::Server Disconnected!!!\n");
			}
			else
			{

				CTTargetResolveHost(overlappedTarget->target, overlappedTarget->callback);

			}
			*/
		}

	}

	fprintf(stderr, "\nCTConnection::CTConnectThread::End\n");
	ExitThread(0);
	return 0;

}


unsigned long __stdcall CX_Dequeue_Recv_Decrypt(LPVOID lpParameter)
{
	int i;
	//ULONG CompletionKey;

	OVERLAPPED_ENTRY ovl_entry[CX_MAX_INFLIGHT_DECRYPT_PACKETS];
	ULONG nPaquetsDequeued;

	CTOverlappedResponse* overlappedResponse = NULL;

	unsigned long NumBytesRecv = 0;
	unsigned long NumBytesRemaining = 0;
	char* extraBuffer = NULL;

	unsigned long PrevBytesRecvd = 0;
	unsigned long extraBufferOffset = 0;

	CTSSLStatus scRet = 0;
	CTClientError ctError = CTSuccess;

	CXConnection* cxConn = NULL;// (CXConnection*)lpParameter;
	
	//CTKernelQueueType hCompletionPort = *(CTKernelQueueType*)lpParameter;
	CTKernelQueue* rq = ((CTKernelQueue*)lpParameter);
	CTKernelQueueType rxQueue = rq->kq;
	//CTKernelQueueType rxPipe[2] = { rq->pq[0], rq->pq[1] };//*((CTKernelQueue*)lpParameter);

	//rq->inflightCursors = 0;
	(*(intptr_t*)(rq->pnInflightCursors)) = 0;

	CTCursor* cursor = NULL;
	CTCursor* nextCursor = NULL;
	CTCursor* nextCursorOnConn = NULL;
	CTCursor* closeCursor = NULL;

	unsigned long cBufferSize = ct_system_allocation_granularity();

	MSG nextCursorMsg = { 0 };
	CTError err = { (CTErrorClass)0,0,NULL };
	unsigned long currentThreadID = GetCurrentThreadId();

#ifdef _DEBUG
	fprintf(stderr, "CXConnection::DecryptResponseCallback Begin\n");
#endif

	//Create a Win32 Platform Message Queue for this thread by calling PeekMessage 
	//(there should not be any messages currently in the queue because it hasn't been created until now)
	PeekMessage(&nextCursorMsg, NULL, WM_USER, WM_USER, PM_REMOVE);

	//TO DO:  Optimize number of packets dequeued at once
	while (GetQueuedCompletionStatusEx(rxQueue, ovl_entry, CX_MAX_INFLIGHT_DECRYPT_PACKETS, &nPaquetsDequeued, INFINITE, TRUE))
	{
#ifdef _DEBUG
		fprintf(stderr, "CXConnection::CTDecryptResponseCallback::nPaquetsDequeued = %d\n", (int)nPaquetsDequeued);
#endif
		for (i = 0; i < nPaquetsDequeued; i++)
		{
			//When to use CompletionKey?
			//CompletionKey = *(ovl_entry[i].lpCompletionKey);

			//Retrieve the number of bytes provided from the socket buffer by IOCP
			NumBytesRecv = ovl_entry[i].dwNumberOfBytesTransferred;//% (cBufferSize - sizeof(CTOverlappedResponse));

			//Get the overlapped struct associated with the asychronous IOCP call
			overlappedResponse = (CTOverlappedResponse*)ovl_entry[i].lpOverlapped;

			//Get the cursor/connection that has been pinned on the overlapped
			cursor = (CTCursor*)overlappedResponse->cursor;
			cxConn = (CXConnection*)(cursor->conn->object_wrapper);
			//assert(cxConn);
#ifdef _DEBUG
			if (!overlappedResponse)
			{
				fprintf(stderr, "CXConnection::CXDecryptResponseCallback::overlappedResponse is NULL!\n");
				assert(1 == 0);
			}
#endif

			//Cursor requests are always queued onto the connection's decrypt thread (ie this one) to be scheduled
			//so every overlapped sent to this decrypt thread is handled here first.
			//There are two possible scenarios:
			//	1)  There are currently previous cursor requests still being processed and the cursor request is scheduled
			//		for pickup later from this thread's message queue
			//	2)  The are currently no previous cursor requests still being processed and so we may initiate an async socket recv
			//		for the cursor's response
			if (overlappedResponse->stage > -1 && overlappedResponse->stage < 3)
			{
				fprintf(stderr, "CX_Dequeue_Recv_Decrypt::Scheduling Cursor (%llu) via ...", cursor->queryToken);

				//set the overlapped message type to indicate that scheduling for the current
				//overlapped/cursor has already been processed
				overlappedResponse->stage = (CTOverlappedStage) ((int)overlappedResponse->stage + 4);// CT_OVERLAPPED_EXECUTE;
				if ( (*(intptr_t*)(rq->pnInflightCursors)) > 0 )// || cursor->conn->responseCount > 0)//< cursor->queryToken)
				{
					fprintf(stderr, "PostThreadMessage\n");
					//if (PostThreadMessage(currentThreadID, WM_USER + cursor->queryToken, 0, (LPARAM)cursor) < 1)
					if (PostMessage(NULL, WM_USER + cursor->conn->socket, 0, (LPARAM)cursor) < 1)
					{
						//cursor->conn->rxCursorQueueCount++;
						fprintf(stderr, "CTDecryptResponseCallback::PostThreadMessage failed with error: %lu", GetLastError());
					}
				}
				else
				{
					fprintf(stderr, "CTCursorAsyncRecv\n");
					if ((ctError = CTCursorAsyncRecv(&overlappedResponse, (void*)(cursor->file.buffer), 0, &NumBytesRecv)) != CTSocketIOPending)
					{
						//the async operation returned immediately
						if (ctError == CTSuccess)
						{
							//cursor->conn->rxCursorQueueCount++;
							fprintf(stderr, "CTConnection::CTDecryptResponseCallback::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRecv);
						}
						else //failure
						{
							fprintf(stderr, "CTConnection::CTDecryptResponseCallback::CTAsyncRecv failed with CTDriverError = %d\n", ctError);
						}
					}
					//else cursor->conn->rxCursorQueueCount++;
				}

				cursor->conn->responseCount++;
				(*(intptr_t*)(rq->pnInflightCursors))++;
				inflightResponseCount++;
				continue;
			}

			//1 If specifically requested zero bytes using WSARecv, then IOCP is notifying us that it has bytes available in the socket buffer (MBEDTLS is deprecated; this path should never be used)
			//2 If we requested anything more than zero bytes using WSARecv, then IOCP is telling us that it was unable to read any bytes at all from the socket buffer (SCHANNEL/WolfSSL)
			//if (NumBytesRecv == 0)
			//{
			//	fprintf(stderr, "CXConnection::CXDecryptResponseCallback::SCHANNEL IOCP::NumBytesRecv = ZERO\n");
			//}
			//else //IOCP already read the data from the socket into the buffer we provided to Overlapped, we have to decrypt it
			//{
				//Get the cursor/connection
				//cursor = (CTCursor*)overlappedResponse->cursor;
			NumBytesRecv += overlappedResponse->wsaBuf.buf - overlappedResponse->buf;
			fprintf(stderr, "Cursor Response Count (%lld) NumBytesRecv (%lu)\n\n", cursor->conn->responseCount, NumBytesRecv);

			PrevBytesRecvd = NumBytesRecv;	 //store the recvd bytes value from the overlapped
			NumBytesRemaining = cBufferSize; //we can never decrypt more bytes than the size of our buffer, and likely schannel will still separate that into chunks

			extraBuffer = overlappedResponse->buf;
			NumBytesRemaining = NumBytesRecv;
			scRet = SEC_E_OK;

#ifdef _DEBUG
			assert(cursor);
			assert(cursor->conn);
			//assert(cursor->headerLengthCallback);
			assert(overlappedResponse->buf);
			assert(overlappedResponse->wsaBuf.buf);
			//fprintf(stderr, "CTConnection::DecryptResponseCallback B::NumBytesRecv = %d\n", (int)NumBytesRecv);
#endif

			//while their is extra data to decrypt keep calling decrypt

			if (overlappedResponse->stage == CT_OVERLAPPED_RECV)
			{
				overlappedResponse->buf += NumBytesRecv;

				while (NumBytesRemaining > 0 && scRet == SEC_E_OK)
				{

					if (cursor->headerLength == 0)
					{
						//search for end of protocol header (and set cxCursor headerLength property
						char* endOfHeader = NULL;

						if (cxConn)
						{
							std::shared_ptr<CXCursor> cxCursor = cxConn->getRequestCursorForKey(cursor->queryToken);
							endOfHeader = cxCursor->ProcessResponseHeader(cursor, cursor->file.buffer, NumBytesRecv);
						}
						else
						{
							endOfHeader = cursor->headerLengthCallback(cursor, cursor->file.buffer, NumBytesRecv);
						}

						//calculate size of header
						if (endOfHeader)
						{
							cursor->headerLength = endOfHeader - (cursor->file.buffer);

							fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV Header (%llu) = \n\n%.*s\n\n", ((ReqlQueryMessageHeader*)(cursor->file.buffer))->token, (int)cursor->headerLength, cursor->file.buffer);

							if (cursor->headerLength > 0)
								memcpy(cursor->requestBuffer, cursor->file.buffer, cursor->headerLength);

							//fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV Header (%llu) = \n\n%.*s\n\n", cursor->headerLength, cursor->requestBuffer);

							//copy body to start of buffer to overwrite header 
							if (endOfHeader != cursor->file.buffer)
							{
								if (NumBytesRecv == cursor->headerLength)
									memset(cursor->file.buffer, 0, cursor->headerLength);
								else
								{
									memcpy(cursor->file.buffer, endOfHeader, NumBytesRecv - cursor->headerLength);
									fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV Body (%lu) = \n\n%.*s\n\n", cursor->contentLength, (int)cursor->contentLength, cursor->file.buffer);
								}
							}
							overlappedResponse->buf -= cursor->headerLength;
							NumBytesRemaining -= cursor->headerLength;
						}
					}
					//assert(cursor->contentLength > 0);

					if (cursor->contentLength > 0 && cursor->contentLength <= overlappedResponse->buf - cursor->file.buffer)
					{
						extraBufferOffset = 0;
						closeCursor = cursor;
						NumBytesRemaining = 0;// (cursor->contentLength + cursor->headerLength);// -NumBytesRecv;

						memset(&nextCursorMsg, 0, sizeof(MSG));
						nextCursor = NULL;
						//if (PeekMessage(&nextCursorMsg, NULL, WM_USER, WM_USER + cursor->conn->queryCount - 1, PM_REMOVE)) //(nextCursor)
						if (PeekMessage(&nextCursorMsg, (HWND)-1, 0, 0, PM_REMOVE))
						{
							nextCursor = (CTCursor*)(nextCursorMsg.lParam);
							nextCursorOnConn = (CTCursor*)(nextCursorMsg.lParam);

							if (nextCursor->conn != closeCursor->conn)
							{
								memset(&nextCursorMsg, 0, sizeof(MSG));
								PeekMessage(&nextCursorMsg, (HWND)-1, WM_USER + closeCursor->conn->socket, WM_USER + closeCursor->conn->socket, PM_NOREMOVE);
								nextCursorOnConn = (CTCursor*)(nextCursorMsg.lParam);

								if (!nextCursorOnConn) nextCursorOnConn = nextCursor;
							}
#ifdef _DEBUG
							//assert(nextCursorMsg.message == WM_USER + (UINT)cursor->queryToken + 1);
							assert(nextCursor);
							assert(nextCursorOnConn);
#endif									
							if (cursor->contentLength < overlappedResponse->buf - cursor->file.buffer)
							{
								fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV::vagina");

								cursor->conn->response_overlap_buffer_length = (size_t)overlappedResponse->buf - (size_t)(cursor->file.buffer + cursor->contentLength);
								assert(cursor->conn->response_overlap_buffer_length > 0);

								if (nextCursorOnConn && cursor->conn->response_overlap_buffer_length > 0)
								{
									NumBytesRemaining = cursor->conn->response_overlap_buffer_length;

									//copy overlapping cursor's response from previous cursor's buffer to next cursor's buffer
									memcpy(nextCursorOnConn->file.buffer, cursor->file.buffer + cursor->contentLength, cursor->conn->response_overlap_buffer_length);

									//read the new cursor's header
									if (nextCursorOnConn->headerLength == 0)
									{
										//search for end of protocol header (and set cxCursor headerLength property
										assert(cursor->file.buffer != nextCursorOnConn->file.buffer);
										//assert(((ReqlQueryMessageHeader*)nextCursor->file.buffer)->token == nextCursor->queryToken);

										char* endOfHeader = NULL;
										if (cxConn)
										{
											std::shared_ptr<CXCursor> cxNextCursor = cxConn->getRequestCursorForKey(nextCursorOnConn->queryToken);
											endOfHeader = cxNextCursor->ProcessResponseHeader(nextCursorOnConn, nextCursorOnConn->file.buffer, cursor->conn->response_overlap_buffer_length);
										}
										else
											endOfHeader = nextCursorOnConn->headerLengthCallback(nextCursorOnConn, nextCursorOnConn->file.buffer, cursor->conn->response_overlap_buffer_length);

										//calculate size of header
										if (endOfHeader)
										{
											nextCursor->headerLength = endOfHeader - (nextCursorOnConn->file.buffer);

											if (nextCursorOnConn->headerLength > 0)
											{

												if (cursor->conn->response_overlap_buffer_length < nextCursorOnConn->headerLength)
												{
													//leave the incomplete header in place and go around again
													nextCursorOnConn->headerLength = 0;
													//NumBytesRemaining = 32768;
												}
												else
												{

													memcpy(nextCursorOnConn->requestBuffer, nextCursorOnConn->file.buffer, nextCursorOnConn->headerLength);

													fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV nextCursor->Header (%llu) = \n\n%.*s\n\n", ((ReqlQueryMessageHeader*)(nextCursorOnConn->file.buffer))->token, (int)nextCursorOnConn->headerLength, nextCursorOnConn->requestBuffer);

													//copy body to start of buffer to overwrite header 
													if (cursor->conn->response_overlap_buffer_length > nextCursorOnConn->headerLength && endOfHeader != nextCursorOnConn->file.buffer)
													{
														memcpy(nextCursorOnConn->file.buffer, endOfHeader, cursor->conn->response_overlap_buffer_length - nextCursorOnConn->headerLength);
														fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV nextCursor->Body (%llu) = \n\n%.*s\n\n", cursor->conn->response_overlap_buffer_length, (int)nextCursorOnConn->contentLength, nextCursorOnConn->file.buffer);

													}
													//else if( cursor->conn->response_overlap_buffer_length == nextCursor->headerLength)
													//	NumBytesRemaining = 
													//overlappedResponse->buf += endDecryptedBytes - endOfHeader;

													//fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV Body = \n\n%.*s\n\n", nextCursor->contentLength, nextCursor->file.buffer);


													cursor->conn->response_overlap_buffer_length -= nextCursorOnConn->headerLength;
													NumBytesRemaining = cursor->conn->response_overlap_buffer_length;

													//NumBytesRemaining = 32768;//


												}
												nextCursorOnConn->overlappedResponse.buf = nextCursorOnConn->file.buffer + cursor->conn->response_overlap_buffer_length;

											}
										}

										//extraBufferOffset = cursor->conn->response_overlap_buffer_length;
										//NumBytesRemaining -= cursor->headerLength;
										//nextCursor->overlappedResponse.buf = nextCursor->file.buffer + cursor->conn->response_overlap_buffer_length;
										//if (nextCursor->queryToken == 99) assert(1 == 0);


									}

								}
							}
							//else
							//	nextCursor = NULL;

							if (nextCursor && NumBytesRemaining == 0)
							{
								fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV::Penis\n");

								assert(nextCursor);
								NumBytesRecv = cBufferSize;;
								overlappedResponse = &(nextCursor->overlappedResponse);
								//overlappedResponse->buf = nextCursor->file.buffer;
								//if ((scRet = CTCursorAsyncRecv(&(overlappedResponse), (void*)(nextCursor->file.buffer  /* + cursor->conn->response_overlap_buffer_length - nextCursor->headerLength*/), 0, &NumBytesRecv)) != CTSocketIOPending)
								if ((scRet = CTCursorAsyncRecv(&(overlappedResponse), (void*)(overlappedResponse->buf), 0, &NumBytesRecv)) != CTSocketIOPending)
								{
									//the async operation returned immediately
									if (scRet == CTSuccess)
									{
										fprintf(stderr, "CTConnection::CT_OVERLAPPED_RECV::CTAsyncRecv (new cursor) returned immediate with %lu bytes\n", NumBytesRecv);

									}
									else //failure
									{
										fprintf(stderr, "CTConnection::CT_OVERLAPPED_RECV::CTAsyncRecv (new cursor) failed with CTDriverError = %ld\n", scRet);
									}
								}
								nextCursor = NULL;
								//NumBytesRemaining = 0;
								//break;
							}

							else if (nextCursor)
							{
								fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV::Scrotum\n");

#ifdef _DEBUG
								assert(nextCursor);
								assert(nextCursor->file.buffer);
								//assert(extraBuffer);
#endif
								overlappedResponse = &(nextCursor->overlappedResponse);
								//overlappedResponse->buf = nextCursor->file.buffer + nextCursor->headerLength;// cursor->conn->response_overlap_buffer_length;// +extraBufferOffset;
								if (nextCursor->conn != closeCursor->conn)
								{
									//memset(&nextCursorMsg, 0, sizeof(MSG));
									//PeekMessage(&nextCursorMsg, (HWND)-1, WM_USER + closeCursor->conn->socket, WM_USER + closeCursor->conn->socket, PM_NOREMOVE);
									//PostMessage(NULL, WM_USER + nextCursor->conn->socket, 0, (LPARAM)nextCursor);
									//nextCursor = (CTCursor*)(nextCursorMsg.lParam);
									//CTCursor* nextCursorOnConn = (CTCursor*)(nextCursorMsg.lParam);

									assert(nextCursorOnConn->conn == closeCursor->conn);
									assert(nextCursorOnConn->conn->socket == closeCursor->conn->socket);

									//memcpy(nextCursorOnConn->file.buffer + extraBufferOffset, extraBuffer, NumBytesRemaining);
									//extraBuffer = nextCursorOnConn->file.buffer + extraBufferOffset;

									//nextCursorOnConn->overlappedResponse.buf = nextCursorOnConn->file.buffer + extraBufferOffset;
									//nextCursorOnConn->overlappedResponse.wsaBuf.buf = nextCursorOnConn->file.buffer + extraBufferOffset + NumBytesRemaining;

									nextCursorOnConn->overlappedResponse.stage = CT_OVERLAPPED_RECV;
									//nextCursor = nextCursorOnConn;
									assert(nextCursor);

									//overlappedResponse = &(nextCursor->overlappedResponse);
									//overlappedResponse->buf = nextCursor->file.buffer;// +extraBufferOffset;
									overlappedResponse->stage = CT_OVERLAPPED_RECV;
									NumBytesRemaining = cBufferSize;
									NumBytesRecv = 0;


									if ((scRet = CTCursorAsyncRecv(&overlappedResponse, overlappedResponse->buf, 0, &NumBytesRemaining)) != CTSocketIOPending)
									{
										//the async operation returned immediately
										if (scRet == CTSuccess)
										{
											fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRemaining);
											//scRet = CTSSLDecryptMessage(overlappedResponse->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);
										}
										else //failure
										{
											fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv failed with CTDriverError = %ld\n", scRet);
											break;
										}
									}

									NumBytesRemaining = 0;
									extraBuffer = NULL;
									scRet = SEC_E_OK;
								}
								else
								{
									//since extra buffer may be pointing to current cursor buffer memory we have to copy it to the next
									//cursor BEFORE closing the current cursor's mapping
									//memcpy(nextCursor->file.buffer + extraBufferOffset, extraBuffer, NumBytesRemaining);
									//extraBuffer = nextCursor->file.buffer + extraBufferOffset;
									//overlappedResponse = &(nextCursor->overlappedResponse);
									//overlappedResponse->buf = nextCursor->file.buffer + extraBufferOffset;
								}

								//since extra buffer may be pointing to current cursor buffer memory we have to copy it to the next
								//cursor BEFORE closing the current cursor's mapping
								//memcpy(nextCursor->file.buffer + extraBufferOffset, extraBuffer, NumBytesRemaining);
								//extraBuffer = nextCursor->file.buffer + extraBufferOffset;

								cursor = nextCursor;
								nextCursor = NULL;
							}

						}

						//increment the connection response count
						//closeCursor->conn->rxCursorQueueCount--;
						closeCursor->conn->responseCount--;
						(*(intptr_t*)(rq->pnInflightCursors))--;
						inflightResponseCount--;
#ifdef _DEBUG
						//assert(closeCursor->responseCallback);
#endif
						//issue the response to the client caller
						if (cxConn) cxConn->distributeResponseWithCursorForToken(closeCursor->queryToken);
						else closeCursor->responseCallback(&err, closeCursor);
						closeCursor = NULL;	//set to NULL for posterity
					}
					else
					{
						//NumBytesRecv = NumBytesRemaining;
						//NumBytesRemaining = cBufferSize - NumBytesRecv;
						//if ((scRet = CTCursorAsyncRecv(&overlappedResponse, cursor->file.buffer, NumBytesRecv, &NumBytesRemaining)) != CTSocketIOPending)

						//TO DO:  make sure this change works on WIN32
						NumBytesRemaining = cBufferSize;
						if ((scRet = CTCursorAsyncRecv(&overlappedResponse, overlappedResponse->buf, 0, &NumBytesRemaining)) != CTSocketIOPending)
						{
							//the async operation returned immediately
							if (scRet == CTSuccess)
							{
								fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRemaining);
								//scRet = CTSSLDecryptMessage(overlappedResponse->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);
								//continue;
							}
							else //failure
							{
								fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv failed with CTDriverError = %ld\n", scRet);
								assert(1 == 0);
							}
						}
						//NumBytesRemaining = 0;
						break;
					}
					//continue;
				}
			}

			else if (overlappedResponse->stage == CT_OVERLAPPED_EXECUTE)//CT_OVERLAPPED_RECV_DECRYPT)
			{

				//Issue a blocking call to decrypt the message (again, only if using non-zero read path, which is exclusively used with MBEDTLS at current)

				/*
				PrevBytesRecvd = NumBytesRecv;	 //store the recvd bytes value from the overlapped
				NumBytesRemaining = cBufferSize; //we can never decrypt more bytes than the size of our buffer, and likely schannel will still separate that into chunks

				extraBuffer = overlappedResponse->buf;
				NumBytesRemaining = NumBytesRecv;
				scRet = SEC_E_OK;
				*/

				//while their is extra data to decrypt keep calling decrypt
				while (NumBytesRemaining > 0 && scRet == SEC_E_OK)
				{
					//move the extra buffer to the front of our operating buffer
					overlappedResponse->wsaBuf.buf = extraBuffer;
					extraBuffer = NULL;
					PrevBytesRecvd = NumBytesRecv;		//store the previous amount of decrypted bytes
					NumBytesRecv = NumBytesRemaining;	//set up for the next decryption 

					scRet = CTSSLDecryptMessage2(cursor->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv, &extraBuffer, &NumBytesRemaining);
					//fprintf(stderr, "CTConnection::DecryptResponseCallback C::Decrypted data: %d bytes recvd, %d bytes remaining, scRet = 0x%x\n", NumBytesRecv, NumBytesRemaining, scRet); //PrintText(NumBytesRecv, (PBYTE)(overlappedResponse->wsaBuf.buf + overlappedResponse->conn->sslContext->Sizes.cbHeader));

					if (scRet == SEC_E_OK)
					{
						//push past cbHeader and copy decrypted message bytes
						memcpy((char*)overlappedResponse->buf, (char*)overlappedResponse->wsaBuf.buf + cursor->conn->sslContext->Sizes.cbHeader, NumBytesRecv);
						overlappedResponse->buf += NumBytesRecv;//+ overlappedResponse->conn->sslContext->Sizes.cbHeader;// + overlappedResponse->conn->sslContext->Sizes.cbTrailer;

						//if( NumBytesRemaining == 0 )
						//{
							//scRet = ioctlsocket(cursor->conn->socket, FIONREAD, &NumBytesRemaining);

							//fprintf(stderr, "**** CTConnection::DecryptResponseCallback::ictlsocket C status = %d\n", (int)scRet);
							//fprintf(stderr, "**** CTConnection::DecryptResponseCallback::ictlsocket C = %d\n", (int)NumBytesRemaining);

							//select said there was data but ioctlsocket says there is no data
							//if( NumBytesRemaining == 0)

						if (cursor->headerLength == 0)
						{
							//search for end of protocol header (and set cxCursor headerLength property
							char* endOfHeader = NULL;

							if (cxConn)
							{
								std::shared_ptr<CXCursor> cxCursor = cxConn->getRequestCursorForKey(cursor->queryToken);
								endOfHeader = cxCursor->ProcessResponseHeader(cursor, cursor->file.buffer, NumBytesRecv);
							}
							else
							{
								endOfHeader = cursor->headerLengthCallback(cursor, cursor->file.buffer, NumBytesRecv);
							}

							//calculate size of header
							if (endOfHeader)
							{
								cursor->headerLength = endOfHeader - (cursor->file.buffer);

								memcpy(cursor->requestBuffer, cursor->file.buffer, cursor->headerLength);

								//fprintf(stderr, "CTDecryptResponseCallbacK::Header = \n\n%.*s\n\n", cursor->headerLength, cursor->requestBuffer);

								//copy body to start of buffer to overwrite header 
								memcpy(cursor->file.buffer, endOfHeader, NumBytesRecv - cursor->headerLength);
								//overlappedResponse->buf += endDecryptedBytes - endOfHeader;

								overlappedResponse->buf -= cursor->headerLength;
							}
						}
						assert(cursor->contentLength > 0);

						if (cursor->contentLength <= overlappedResponse->buf - cursor->file.buffer)
						{
							extraBufferOffset = 0;
							closeCursor = cursor;	//store a ptr to the current cursor so we can close it after replacing with next cursor
							memset(&nextCursorMsg, 0, sizeof(MSG));
							//if (PeekMessage(&nextCursorMsg, NULL, WM_USER, WM_USER + cursor->conn->queryCount - 1, PM_REMOVE)) //(nextCursor)
							if (PeekMessage(&nextCursorMsg, (HWND)-1, 0, 0, PM_REMOVE))
							{
								nextCursor = (CTCursor*)(nextCursorMsg.lParam);
								nextCursorOnConn = (CTCursor*)(nextCursorMsg.lParam);

								if (nextCursor->conn != closeCursor->conn)
								{
									memset(&nextCursorMsg, 0, sizeof(MSG));
									PeekMessage(&nextCursorMsg, (HWND)-1, WM_USER + closeCursor->conn->socket, WM_USER + closeCursor->conn->socket, PM_NOREMOVE);
									nextCursorOnConn = (CTCursor*)(nextCursorMsg.lParam);

									if (!nextCursorOnConn) nextCursorOnConn = nextCursor;
								}
#ifdef _DEBUG
								//assert(nextCursorMsg.message == WM_USER + (UINT)cursor->queryToken + 1);
								assert(nextCursor);
								assert(nextCursorOnConn);
#endif						
								fprintf(stderr, "CX_Dequeue_Recv_Decrypt::PeekMessage Removed Cursor (%llu)\n", nextCursorOnConn->queryToken);

								if (cursor->contentLength < overlappedResponse->buf - cursor->file.buffer)
								{
									fprintf(stderr, "vagina");

									cursor->conn->response_overlap_buffer_length = (size_t)overlappedResponse->buf - (size_t)(cursor->file.buffer + cursor->contentLength);
									assert(cursor->conn->response_overlap_buffer_length > 0);

									//NumBytesRemaining = cursor->conn->response_overlap_buffer_length;
									if (nextCursorOnConn && cursor->conn->response_overlap_buffer_length > 0)
									{
										//copy overlapping cursor's response from previous cursor's buffer to next cursor's buffer
										memcpy(nextCursorOnConn->file.buffer, cursor->file.buffer + cursor->contentLength, cursor->conn->response_overlap_buffer_length);

										//read the new cursor's header
										if (nextCursorOnConn->headerLength == 0)
										{
											//search for end of protocol header (and set cxCursor headerLength property
											assert(cursor->file.buffer != nextCursorOnConn->file.buffer);

											char* endOfHeader = NULL;
											if (cxConn)
											{
												std::shared_ptr<CXCursor> cxNextCursor = cxConn->getRequestCursorForKey(nextCursorOnConn->queryToken);
												endOfHeader = cxNextCursor->ProcessResponseHeader(nextCursorOnConn, nextCursorOnConn->file.buffer, cursor->conn->response_overlap_buffer_length);
											}
											else
												endOfHeader = nextCursorOnConn->headerLengthCallback(nextCursorOnConn, nextCursorOnConn->file.buffer, cursor->conn->response_overlap_buffer_length);


											//calculate size of header
											nextCursorOnConn->headerLength = endOfHeader - (nextCursorOnConn->file.buffer);

											//copy header to cursor's request buffer
											memcpy(nextCursorOnConn->requestBuffer, nextCursorOnConn->file.buffer, nextCursorOnConn->headerLength);

											//copy body to start of buffer to overwrite header
											assert(nextCursorOnConn->file.buffer);
											memcpy(nextCursorOnConn->file.buffer, endOfHeader, cursor->conn->response_overlap_buffer_length - nextCursorOnConn->headerLength);

											cursor->conn->response_overlap_buffer_length -= nextCursorOnConn->headerLength;
											extraBufferOffset = cursor->conn->response_overlap_buffer_length;

											/*
											if (nextCursor->contentLength <= cursor->conn->response_overlap_buffer_length)
											{
												//if (nextCursor->contentLength < cursor->conn->response_overlap_buffer_length)


												cursor->conn->response_overlap_buffer_length -= nextCursor->contentLength;

												cursor = nextCursor;
												nextCursor = NULL;
												if (PeekMessage(&nextCursorMsg, NULL, WM_USER + cursor->queryToken + 1, WM_USER + cursor->queryToken + 1, PM_REMOVE)) //(nextCursor)
												{
													assert(nextCursorMsg.message == WM_USER + (UINT)cursor->queryToken + 1);
													nextCursor = (CTCursor*)(nextCursorMsg.lParam);
													assert(nextCursor);
												}
												//if (nextCursor->contentLength < cursor->conn->response_overlap_buffer_length - nextCursor->headerLength)
												//{
												//	cursor = nextCursor;
												//
												//}

												//decrement the cursor queue count to finish ours
												cursor->conn->rxCursorQueueCount--;

												CTCursorCloseMappingWithSize(cursor, cursor->contentLength); //overlappedResponse->buf - cursor->file.buffer);
	#ifdef _DEBUG
														assert(cursor->responseCallback);
	#endif
														cursor->responseCallback(NULL, cursor);

														continue;

													}

													*/
										}

									}
								}
								if (nextCursor && NumBytesRemaining == 0)
								{
									fprintf(stderr, "penis");

									NumBytesRecv = cBufferSize;;
									overlappedResponse = &(nextCursor->overlappedResponse);
									//if ((scRet = CTCursorAsyncRecv(&(overlappedResponse), (void*)(nextCursor->file.buffer /* + cursor->conn->response_overlap_buffer_length - nextCursor->headerLength*/), 0, &NumBytesRecv)) != CTSocketIOPending)
									if ((scRet = CTCursorAsyncRecv(&(overlappedResponse), (void*)(overlappedResponse->buf/* + cursor->conn->response_overlap_buffer_length - nextCursor->headerLength*/), overlappedResponse->wsaBuf.buf - overlappedResponse->buf, &NumBytesRecv)) != CTSocketIOPending)
									{
										//the async operation returned immediately
										if (scRet == CTSuccess)
										{
											fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv (new cursor) returned immediate with %lu bytes\n", NumBytesRecv);

										}
										else //failure
										{
											fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv (new cursor) failed with CTDriverError = %ld\n", scRet);
										}
									}
									nextCursor = NULL;
									//break;
								}
								else
								{
									fprintf(stderr, "scrotum");

#ifdef _DEBUG
									assert(nextCursor);
									assert(nextCursor->file.buffer);
									assert(extraBuffer);
									assert(NumBytesRemaining > 0);
#endif
									if (nextCursor->conn != closeCursor->conn)
									{
										//memset(&nextCursorMsg, 0, sizeof(MSG));
										//PeekMessage(&nextCursorMsg, (HWND)-1, WM_USER + closeCursor->conn->socket, WM_USER + closeCursor->conn->socket, PM_NOREMOVE);
										//PostMessage(NULL, WM_USER + nextCursor->conn->socket, 0, (LPARAM)nextCursor);
										//nextCursor = (CTCursor*)(nextCursorMsg.lParam);
										//CTCursor* nextCursorOnConn = (CTCursor*)(nextCursorMsg.lParam);

										assert(nextCursorOnConn->conn == closeCursor->conn);
										assert(nextCursorOnConn->conn->socket == closeCursor->conn->socket);

										memcpy(nextCursorOnConn->file.buffer + extraBufferOffset, extraBuffer, NumBytesRemaining);
										extraBuffer = nextCursorOnConn->file.buffer + extraBufferOffset;

										nextCursorOnConn->overlappedResponse.buf = nextCursorOnConn->file.buffer + extraBufferOffset;
										nextCursorOnConn->overlappedResponse.wsaBuf.buf = nextCursorOnConn->file.buffer + extraBufferOffset + NumBytesRemaining;

										nextCursorOnConn->overlappedResponse.stage = CT_OVERLAPPED_EXECUTE;
										//nextCursor = nextCursorOnConn;
										assert(nextCursor);

										overlappedResponse = &(nextCursor->overlappedResponse);
										overlappedResponse->buf = nextCursor->file.buffer;// +extraBufferOffset;
										overlappedResponse->stage = CT_OVERLAPPED_EXECUTE;
										NumBytesRemaining = cBufferSize;
										NumBytesRecv = 0;


										if ((scRet = CTCursorAsyncRecv(&overlappedResponse, overlappedResponse->buf, overlappedResponse->wsaBuf.buf - overlappedResponse->buf, &NumBytesRemaining)) != CTSocketIOPending)
										{
											//the async operation returned immediately
											if (scRet == CTSuccess)
											{
												fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRemaining);
												//scRet = CTSSLDecryptMessage(overlappedResponse->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);
											}
											else //failure
											{
												fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv failed with CTDriverError = %ld\n", scRet);
												break;
											}
										}

										NumBytesRemaining = 0;
										extraBuffer = NULL;
										scRet = SEC_E_OK;


									}
									else
									{
										//since extra buffer may be pointing to current cursor buffer memory we have to copy it to the next
										//cursor BEFORE closing the current cursor's mapping
										memcpy(nextCursor->file.buffer + extraBufferOffset, extraBuffer, NumBytesRemaining);
										extraBuffer = nextCursor->file.buffer + extraBufferOffset;
										overlappedResponse = &(nextCursor->overlappedResponse);
										overlappedResponse->buf = nextCursor->file.buffer + extraBufferOffset;
									}


									cursor = nextCursor;
									nextCursor = NULL;
								}

							}

							//increment the connection response count
							//closeCursor->conn->rxCursorQueueCount--;
							closeCursor->conn->responseCount--;
							(*(intptr_t*)(rq->pnInflightCursors))--;
							inflightResponseCount--;
#ifdef _DEBUG
							//assert(closeCursor->responseCallback);
#endif
							//issue the response to the client caller
							if (cxConn) cxConn->distributeResponseWithCursorForToken(closeCursor->queryToken);
							else closeCursor->responseCallback(&err, closeCursor);
							closeCursor = NULL;	//set to NULL for posterity
						}
						else if (NumBytesRemaining == 0)
						{
							fprintf(stderr, "wanker");

							NumBytesRemaining = cBufferSize;
							if ((scRet = CTCursorAsyncRecv(&overlappedResponse, overlappedResponse->buf, 0, &NumBytesRemaining)) != CTSocketIOPending)
							{
								//the async operation returned immediately
								if (scRet == CTSuccess)
								{
									fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRemaining);
									//scRet = CTSSLDecryptMessage(overlappedResponse->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);
								}
								else //failure
								{
									fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv failed with CTDriverError = %ld\n", scRet);
									break;
								}
							}
							break;
						}

					}
				}

				if (scRet == SEC_E_INCOMPLETE_MESSAGE)
				{
					//fprintf(stderr, "CTConnection::DecryptResponseCallback C::SEC_E_INCOMPLETE_MESSAGE %d bytes recvd, %d bytes remaining\n", PrevBytesRecvd, NumBytesRemaining); //PrintText(NumBytesRecv, (PBYTE)(overlappedResponse->wsaBuf.buf + overlappedResponse->conn->sslContext->Sizes.cbHeader));

					//
					//	Handle INCOMPLETE ENCRYPTED MESSAGE AS FOLLOWS:
					// 
					//	1)  copy the remaining encrypted bytes to the start of our buffer so we can 
					//		append to them when receiving from socket on the next async iocp iteration
					//  
					//	2)  use NumBytesRecv var as an offset input parameter CTCursorAsyncRecv
					//
					//	3)  use NumByteRemaining var as input to CTCursorAsyncRecv to request num bytes to recv from socket
					//		(PrevBytesRecvd will be subtracted from NumBytesRemaining internal to CTAsyncRecv

					//For SCHANNEL:  CTSSLDecryptMessage2 will point extraBuffer at the input msg buffer
					//For WolfSSL:   CTSSLDecryptMessage2 will point extraBuffer at the input msg buffer + the size of the ssl header wolfssl already consumed
					if( extraBuffer ) memcpy((char*)overlappedResponse->buf, extraBuffer, NumBytesRemaining);
					NumBytesRemaining = cBufferSize;   //request exactly the amount to complete the message NumBytesRemaining 
													   //populated by CTSSLDecryptMessage2 above or request more if desired

					if ((scRet = CTCursorAsyncRecv(&overlappedResponse, overlappedResponse->buf, NumBytesRecv, &NumBytesRemaining)) != CTSocketIOPending)
					{
						//the async operation returned immediately
						if (scRet == CTSuccess)
						{
							//fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRemaining);
							//scRet = CTSSLDecryptMessage(overlappedResponse->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);
						}
						else //failure
						{
							fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv failed with CTDriverError = %ld\n", scRet);
							break;
						}
					}
				}

			}
			//fprintf(stderr, "\nCTDecryptResponseCallback::END\n\n");

			//TO DO:  Enable SSL negotation.  Determine when to appropriately do so...here?
			// The server wants to perform another handshake sequence.
			/*
			if(scRet == CTSSLRenegotiate)
			{
				fprintf(stderr, "CTDecryptResponseCallback::Server requested renegotiate!\n");
				//scRet = ClientHandshakeLoop( Socket, phCreds, phContext, FALSE, &ExtraBuffer);
				scRet = CTSSLHandshake(overlappedResponse->conn->socket, overlappedResponse->conn->sslContext, NULL);
				if(scRet != SEC_E_OK) return scRet;

				if(overlappedResponse->conn->sslContext->ExtraData.pvBuffer) // Move any "extra" data to the input buffer.
				{
					fprintf(stderr, "\nCTDecryptResponseCallback::Unhandled Extra Data\n");
					//MoveMemory(overlappedConn->conn->response_buf, overlappedConn->conn->sslContextRef->ExtraData.pvBuffer, overlappedConn->conn->sslContextRef->ExtraData.cbBuffer);
					//cbIoBuffer = sslContextRef->ExtraData.cbBuffer;
				}
			}
			*/
			//} end else (NumBytesRecv == 0)
		}

	}

#ifdef DEBUG
	fprintf(stderr, "\nCTDecryptResponseCallback::End\n");
#endif
	ExitThread(0);
	return 0;
}


unsigned long __stdcall CX_Dequeue_Encrypt_Send(LPVOID lpParameter)
{
	int i;
	ULONG_PTR CompletionKey;	//when to use completion key?
	uint64_t queryToken = 0;
	OVERLAPPED_ENTRY ovl_entry[CX_MAX_INFLIGHT_ENCRYPT_PACKETS];
	ULONG nPaquetsDequeued;

	CTCursor* cursor = NULL;
	CTOverlappedResponse* overlappedRequest = NULL;
	CTOverlappedResponse* overlappedResponse = NULL;

	unsigned long NumBytesSent = 0;
	unsigned long recvMsgLength = 0;

	CTSSLStatus scRet = 0;
	//CTClientError ctError = CTSuccess;
	CTError err = { (CTErrorClass)0,0,NULL };


	//CXConnection* cxConn = NULL;// (CXConnection*)lpParameter;
	CTKernelQueueType hCompletionPort = *(CTKernelQueueType*)lpParameter;

	unsigned long cBufferSize = ct_system_allocation_granularity();

	fprintf(stderr, "CXConnection::EncryptQueryCallback Begin\n");

	//TO DO:  Optimize number of packets dequeued at once
	while (GetQueuedCompletionStatusEx(hCompletionPort, ovl_entry, CX_MAX_INFLIGHT_ENCRYPT_PACKETS, &nPaquetsDequeued, INFINITE, TRUE))
	{
		fprintf(stderr, "CXConnection::EncryptQueryCallback::nPaquetsDequeued = %d\n", (int)nPaquetsDequeued);
		for (i = 0; i < nPaquetsDequeued; i++)
		{
			CompletionKey = (ULONG_PTR)ovl_entry[i].lpCompletionKey;
			NumBytesSent = ovl_entry[i].dwNumberOfBytesTransferred;
			overlappedRequest = (CTOverlappedResponse*)ovl_entry[i].lpOverlapped;

			int schedule_stage = CT_OVERLAPPED_SCHEDULE;
			if (!overlappedRequest)
			{
				fprintf(stderr, "CXConnection::EncryptQueryCallback::GetQueuedCompletionStatus continue\n");
				continue;
			}

			if (NumBytesSent == 0)
			{
				fprintf(stderr, "CXConnection::EncryptQueryCallback::Server Disconnected!!!\n");
			}
			else
			{
				//Get the cursor/connection
				//Get a the CTCursor from the overlappedRequest struct, retrieve the overlapped response struct memory, and store reference to the CTCursor in the about-to-be-queued overlappedResponse
				cursor = (CTCursor*)(overlappedRequest->cursor);
				//std::shared_ptr<CXCursor> cxCursor = cxConn->getRequestCursorForKey(cursor->queryToken);

#ifdef _DEBUG
				assert(cursor);
				assert(cursor->conn);
#endif
				NumBytesSent = overlappedRequest->len;
				fprintf(stderr, "CXConnection::EncryptQueryCallback::NumBytesSent = %d\n", (int)NumBytesSent);
				//fprintf(stderr, "CXConnection::EncryptQueryCallback::msg = %s\n", overlappedRequest->buf + sizeof(ReqlQueryMessageHeader));

				//Use the ReQLC API to synchronously
				//a)	encrypt the query message in the query buffer
				//b)	send the encrypted query data over the socket
				if (overlappedRequest->stage != CT_OVERLAPPED_SEND)
					scRet = CTSSLWrite(cursor->conn->socket, cursor->conn->sslContext, overlappedRequest->buf, &NumBytesSent);
				else
				{
					//TO DO:  create a send callback cursor attachment to remove post send TLS handshake logic here
					//assert(1 == 0);
					int cbData = send(cursor->conn->socket, (char*)overlappedRequest->buf, overlappedRequest->len, 0);

					if (cbData == SOCKET_ERROR || cbData == 0)
					{
						fprintf(stderr, "**** CTConnection::EncryptQueryCallback::CT_OVERLAPPED_SCHEDULE_SEND Error %d sending data to server (%d)\n", WSAGetLastError(), cbData);
					}

					//we haven't populated the cursor's overlappedResponse for recv'ing yet,
					//so use it here to pass the overlappedRequest->buf to the queryCallback (for instance, bc SCHANNEL needs to free handshake memory)
					cursor->overlappedResponse.buf = overlappedRequest->buf;
					err.id = cbData;
					if (cursor->queryCallback)
						cursor->queryCallback(&err, cursor);

					fprintf(stderr, "%d bytes of raw tcp data sent\n", cbData);
					cursor->overlappedResponse.buf = NULL;
					overlappedRequest->buf = NULL; //should this stay here or in the queryCallback?
					schedule_stage = CT_OVERLAPPED_SCHEDULE_RECV;
				}


#ifdef CTRANSPORT_USE_MBED_TLS
				//Issuing a zero buffer read will tell iocp to let us know when there is data available for reading from the socket
				//so we can issue our own blocking read
				recvMsgLength = 0;
#else
				//However, the zero buffer read doesn't seem to scale very well if Schannel is encrypting/decrypting for us 
				recvMsgLength = cBufferSize;
#endif
				queryToken = cursor->queryToken;
				fprintf(stderr, "CXConnection::EncryptQueryCallback::queryTOken = %llu\n", queryToken);

				//Issue a blocking read for debugging purposes
				//CTRecv(overlappedQuery->conn, (void*)&(overlappedQuery->conn->response_buffers[ (queryToken%REQL_MAX_INFLIGHT_QUERIES) * REQL_RESPONSE_BUFF_SIZE]), &recvMsgLength);


				//populate the overlapped response as needed before passing to CTCursorAsyncRecv
				overlappedResponse = &(cursor->overlappedResponse);
				overlappedResponse->cursor = (void*)cursor;
				overlappedResponse->len = cBufferSize;
				overlappedResponse->stage = (CTOverlappedStage)schedule_stage;


				//Use WSASockets + IOCP to asynchronously do one of the following (depending on the usage of our underlying ssl context):
				//a)	notify us when data is available for reading from the socket via the rxQueue/thread (ie MBEDTLS path)
				//b)	read the encrypted response message from the socket into the response buffer and then notify us on the rxQueue/thread (ie SCHANNEL path)
				//if ((ctError = CTAsyncRecv(overlappedRequest->conn, (void*)&(overlappedRequest->conn->response_buffers[(queryToken%CX_MAX_INFLIGHT_QUERIES) * cBufferSize]), 0, &recvMsgLength)) != CTSocketIOPending)
				/*
				if ((ctError = CTCursorAsyncRecv(&overlappedResponse, (void*)(cursor->file.buffer), 0, &recvMsgLength)) != CTSocketIOPending)
				{
					//the async operation returned immediately
					if (ctError == CTSuccess)
					{
						fprintf(stderr, "CXConnection::EncryptQueryCallback::CTAsyncRecv returned immediate with %lu bytes\n", recvMsgLength);
					}
					else //failure
					{
						fprintf(stderr, "CXConnection::EncryptQueryCallback::CTAsyncRecv failed with CTDriverError = %d\n", ctError);
					}
				}
				*/

				if( cursor->conn->object_wrapper || cursor->headerLengthCallback != 0 )
					CTCursorRecvOnQueue(&overlappedResponse, (void*)(cursor->file.buffer), 0, &recvMsgLength);

				//Manual encryption is is left here commented out as an example
				//scRet = CTSSLEncryptMessage(overlappedConn->conn->sslContext, overlappedConn->conn->response_buf, &NumBytesRecv);
				//if( scRet == CTSSLContextExpired ) break; // Server signaled end of session
				//if( scRet == CTSuccess ) break;

				//TO DO:  Enable renogotiate, but how would the send thread/queue know if the server wants to perform another handshake?
				// The server wants to perform another handshake sequence.
				/*
				if(scRet == CTSSLRenegotiate)
				{
					fprintf(stderr, "CTEncryptQueryCallback::Server requested renegotiate!\n");
					//scRet = ClientHandshakeLoop( Socket, phCreds, phContext, FALSE, &ExtraBuffer);
					scRet = CTSSLHandshake(overlappedQuery->conn->socket, overlappedQuery->conn->sslContext, NULL);
					if(scRet != SEC_E_OK) return scRet;

					if(overlappedQuery->conn->sslContext->ExtraData.pvBuffer) // Move any "extra" data to the input buffer.
					{
						fprintf(stderr, "\nCTEncryptQueryCallback::Unhandled Extra Data\n");
						//MoveMemory(overlappedConn->conn->response_buf, overlappedConn->conn->sslContextRef->ExtraData.pvBuffer, overlappedConn->conn->sslContextRef->ExtraData.cbBuffer);
						//cbIoBuffer = sslContextRef->ExtraData.cbBuffer;
					}
				}
				*/
			}
		}

	}

	fprintf(stderr, "\nCXConnectin::EncryptQueryCallback::End\n");
	ExitThread(0);
	return 0;

}

#endif
