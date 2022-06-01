#include "../CTransport.h"


#include <assert.h>

#ifndef EVFILT_READ
#define EVFILT_READ		(-1)
#define EVFILT_WRITE	(-2)
#define EVFILT_TIMER	(-7)
#define EV_EOF			0x8000
#endif

CTRANSPORT_API void CTCursorCloseFile(CTCursor *cursor)
{
#ifndef WIN32 
	ct_file_unmap(cursor->file.fd, cursor->file.buffer);
#endif
	ct_file_close(cursor->file.fd);
}

CTRANSPORT_API void CTCursorCloseFileMap(CTCursor *cursor)
{
	ct_file_unmap(cursor->file.fd, cursor->file.buffer);
}


void CTCursorCloseMappingWithSize(CTCursor* cursor, unsigned long fileSize)
{
	DWORD dwCurrentFilePosition;
	DWORD dwMaximumSizeHigh = (unsigned long long)fileSize >> 32;
	DWORD dwMaximumSizeLow = fileSize & 0xffffffffu;
	LONG offsetHigh = (LONG)dwMaximumSizeHigh;
	LONG offsetLow = (LONG)dwMaximumSizeLow;

	FlushViewOfFile(cursor->file.buffer , fileSize);
	UnmapViewOfFile(cursor->file.buffer );
	CloseHandle(cursor->file.mFile );
	cursor->file.size = fileSize;

	fprintf(stderr, "CTCursorCloseMappingWithSize::Closing file with size = %d bytes\n", fileSize);
	dwCurrentFilePosition = SetFilePointer(cursor->file.hFile, offsetLow, &offsetHigh, FILE_BEGIN); // provides offset from current position
	SetEndOfFile(cursor->file.hFile);
}

CTFileError CTCursorMapFileW(CTCursor * cursor, unsigned long fileSize)
{
	CTFileError err = CTFileSuccess;
#ifdef _WIN32
	DWORD dwErr;
	
	//open file descriptor/buffer
	DWORD dwMaximumSizeHigh, dwMaximumSizeLow;
	LPVOID mapAddress = NULL;
	char handleStr[sizeof(HANDLE)+1] = "\0";

	//set the desired file size on the cursor
	cursor->file.size= fileSize;
 
	//get a win32 file handle from the file descriptor
	cursor->file.hFile = (HANDLE)_get_osfhandle( cursor->file.fd  );

	memcpy(handleStr, &(cursor->file.hFile), sizeof(HANDLE));
	handleStr[sizeof(HANDLE)] = '\0';
	dwMaximumSizeHigh = (unsigned long long)cursor->file.size  >> 32;
	dwMaximumSizeLow = cursor->file.size & 0xffffffffu;

    //fprintf(stderr, "GetLastError (%d)", GetLastError());

	// try to allocate and map our space (get a win32 mapped file handle)
	if ( !(cursor->file.mFile = CreateFileMappingA(cursor->file.hFile, NULL, PAGE_READWRITE, dwMaximumSizeHigh, dwMaximumSizeLow, NULL))  )//||
		//!MapViewOfFileEx(buffer->mapping, FILE_MAP_ALL_ACCESS, 0, 0, ring_size, (char *)desired_addr + ring_size))
	{
		// something went wrong - clean up
		fprintf(stderr, "cr_file_map_to_buffer failed:  OS Virtual Mapping failed with error (%d)", GetLastError());
		err = CTFileMapError;
		return err;
	}
		
	// Offsets must be a multiple of the system's allocation granularity.  We
	// guarantee this by making our view size equal to the allocation granularity.
	if( !(cursor->file.buffer = (char *)MapViewOfFileEx(cursor->file.mFile, FILE_MAP_ALL_ACCESS, 0, 0, cursor->file.size , NULL)) )
	{
		fprintf(stderr, "cr_file_map_to_buffer failed:  OS Virtual Mapping failed2 ");
		err = CTFileMapViewError;
		return err;
	}
	//filebuffer = (char*)ct_file_map_to_buffer(&(filebuffer), fileSize, PROT_READWRITE, MAP_SHARED | MAP_NORESERVE, fileDescriptor, 0);
#endif

	return err;
}


CTFileError CTCursorMapFileR(CTCursor * cursor)
{
	CTFileError err = CTFileSuccess;
#ifdef _WIN32
	DWORD dwErr;
	
	//open file descriptor/buffer
	DWORD dwMaximumSizeHigh, dwMaximumSizeLow;
	LPVOID mapAddress = NULL;
	char handleStr[sizeof(HANDLE)+1] = "\0";

	//assume cursor already has file size on the cursor set
	//and that the file is already open
	//cursor->file.size= fileSize;
 
	//get a win32 file handle from the file descriptor
	cursor->file.hFile = (HANDLE)_get_osfhandle( cursor->file.fd  );

	memcpy(handleStr, &(cursor->file.hFile), sizeof(HANDLE));
	handleStr[sizeof(HANDLE)] = '\0';
	dwMaximumSizeHigh = (unsigned long long)cursor->file.size  >> 32;
	dwMaximumSizeLow = cursor->file.size & 0xffffffffu;

	// try to allocate and map our space (get a win32 mapped file handle)
	if ( !(cursor->file.mFile = CreateFileMappingA(cursor->file.hFile, NULL, PAGE_READONLY, dwMaximumSizeHigh, dwMaximumSizeLow, handleStr))  )//||
		//!MapViewOfFileEx(buffer->mapping, FILE_MAP_ALL_ACCESS, 0, 0, ring_size, (char *)desired_addr + ring_size))
	{
		// something went wrong - clean up
		fprintf(stderr, "CTCursorMapFileR::CreateFileMappingA failed:  OS Virtual Mapping failed");
		err = CTFileMapError;
	}
		
	// Offsets must be a multiple of the system's allocation granularity.  We
	// guarantee this by making our view size equal to the allocation granularity.
	if( !(cursor->file.buffer = (char *)MapViewOfFileEx(cursor->file.mFile, FILE_MAP_READ, 0, 0, cursor->file.size , NULL)) )
	{
		fprintf(stderr, "CTCursorMapFileR::MapViewOfFileEx failed:  OS Virtual Mapping failed 2");
		err = CTFileMapViewError;
	}
#endif
	return err;
}


CTFileError CTCursorCreateMapFileW(CTCursor * cursor, char* filepath, unsigned long fileSize)
{
	CTFileError err;

	//1 create file to populate the cursors file descriptor property
	cursor->file.fd = ct_file_create_w(filepath);
	//cursor->file.size= fileSize;
	//fprintf(stderr, "\nFile Size =  %llu bytes\n", fileSize);

	//2 MAP THE CURSOR'S FILE TO BUFFER FOR Writing
	err = CTCursorMapFileW(cursor, fileSize);
	return err;
}

int CT_MAX_CONNECTIONS = 0;
int CT_NUM_CONNECTIONS = 0;
CTConnection *CT_CONNECTION_POOL = NULL;

int CT_NUM_CURSORS = 0;
int CT_CURSOR_INDEX = 0;
CTCursor* CT_CURSOR_POOL = NULL;

void CTCreateConnectionPool(CTConnection* connectionPool, int numConnections)
{
	CT_CONNECTION_POOL = connectionPool;
	CT_MAX_CONNECTIONS = numConnections;
}

CTConnection * CTGetNextPoolConnection()
{
	CTConnection * conn = &(CT_CONNECTION_POOL[CT_NUM_CONNECTIONS]);
	assert(CT_MAX_CONNECTIONS > 0 && conn);
	CT_NUM_CONNECTIONS++;
	return conn;
}



void CTCreateCursorPool(CTCursor* cursorPool, int numCursors)
{
	CT_CURSOR_POOL = cursorPool;
	CT_NUM_CURSORS = numCursors;
}

CTCursor* CTGetNextPoolCursor()
{
	CTCursor* cursor = &(CT_CURSOR_POOL[CT_CURSOR_INDEX]);
	assert(CT_NUM_CURSORS > 0 && CT_CURSOR_INDEX < CT_NUM_CURSORS && cursor);
	CT_CURSOR_INDEX = (CT_CURSOR_INDEX+1)%CT_NUM_CURSORS;
	return cursor;
}

void CTSetCursorPoolIndex(int index)
{
	CT_CURSOR_INDEX = (index) % CT_NUM_CURSORS;
}


/***
 *  CTRecv
 *
 *  CTRecv will wait on a ReqlConnection's full duplex TCP socket
 *  for an EOF read event and return the buffer to the calling function
 ***/

void* CTRecv(CTConnection* conn, void * msg, unsigned long * msgLength)
{
    //OSStatus status;
    int ret; 
	char * decryptedMessagePtr = NULL; 
	unsigned long  totalMsgLength = 0;
	unsigned long remainingBufferSize = *msgLength;

	 //fprintf(stderr, "SSLRead remainingBufferSize = %d, msgLength = %d\n", (int)remainingBufferSize, (int)*msgLength);

    //struct kevent kev;
    //uintptr_t timout_event = -1;
    //uintptr_t out_of_range_event = -2;
    //uint32_t timeout = UINT_MAX;  //uint max will translate to timeout struct of null, which is timeout of forever
    
    //fprintf(stderr, "ReqlRecv Waiting for read...\n");
    //uintptr_t ret_event = kqueue_wait_with_timeout(r->event_queue, &kev, EVFILT_READ | EV_EOF, timout_event, out_of_range_event, r->socket, r->socket, timeout);
    if( CTSocketWait(conn->socket, conn->event_queue, EVFILT_READ | EV_EOF) != conn->socket )
    {
        fprintf(stderr, "something went wrong\n");
        return decryptedMessagePtr;
    }

	if (conn->target->ssl.method > CTSSL_NONE)
	{
#ifdef _WIN32
		ret = CTSSLRead(conn->socket, conn->sslContext, msg, &remainingBufferSize);
		if (ret != 0)
			ret = 0;
		else
		{
#ifdef CTRANSPORT_WOLFSSL
			decryptedMessagePtr = (char*)msg;
#elif defined(CTRANSPORT_USE_MBED_TLS)
			decryptedMessagePtr = (char*)msg;
#elif defined(_WIN32)
			decryptedMessagePtr = (char*)msg + conn->sslContext->Sizes.cbHeader;
#else
			decryptedMessagePtr = msg;
#endif
		}
	}
	else
	{
		
		ret = (size_t)recv(conn->socket, (char*)msg, 256, 0 );

		if(ret == SOCKET_ERROR)
		{
			fprintf(stderr, "**** CTRecv::Error %d reading data from server\n", WSAGetLastError());
			//scRet = SEC_E_INTERNAL_ERROR;
			assert(1 == 0);
			return -1;
		}
		else if(ret== 0)
		{
			fprintf(stderr, "**** CTRecv::Server unexpectedly disconnected\n");
			//scRet = SEC_E_INTERNAL_ERROR;
			return -1;
			assert(1 == 0);
		}
		fprintf(stderr, "CTRecv::%d bytes of handshake data received\n\n%.*s\n\n", ret, ret, (char*)msg);
		decryptedMessagePtr = (char*)msg;
	}
	*msgLength = ret;//(size_t)ret;
	totalMsgLength += *msgLength;
	remainingBufferSize -= *msgLength;
#elif defined(__APPLE__)
    status = SSLRead(conn->sslContext, msg, remainingBufferSize, msgLength);

	totalMsgLength += *msgLength;
    remainingBufferSize -= *msgLength;

    //fprintf(stderr, "Before errSSLWouldBlock...\n");

    while( status == errSSLWouldBlock)
    {
        //TO DO:  Figure out why this is happening for changefeeds ...
        fprintf(stderr, "SSLRead remainingBufferSize = %d, msgLength = %d\n", (int)remainingBufferSize, (int)*msgLength);
        status = SSLRead(conn->sslContext, msg+totalMsgLength, remainingBufferSize, msgLength);
        totalMsgLength += *msgLength;
        remainingBufferSize -= *msgLength;
    }

	if( status != noErr )
    {
        fprintf(stderr, "ReqlRecv::SSLRead failed with error: %d\n", (int)status);
    }
#endif
    //fprintf(stderr, "SSLRead final remainingBufferSize = %d, msgLength = %d\n", (int)remainingBufferSize, (int)*msgLength);
    *msgLength = totalMsgLength;
    return decryptedMessagePtr;
}
    
/***
 *  CTSend
 *
 *  Sends Raw Bytes ReqlConnection socket TLS + TCP Stream
 *  Will wait for the socket to begin writing to the network before returning
 ****/
int CTSend(CTConnection* conn, void * msg, unsigned long msgLength )
{
    //OSStatus status;
	int ret = 0;
    unsigned long bytesProcessed = 0;
    unsigned long max_msg_buffer_size = 4096;
    
    //TO DO:  check response and return to user if failure
    unsigned long totalBytesProcessed = 0;
    //fprintf(stderr, "\nconn->token = %llu", conn->token);
    //fprintf(stderr, "msg = %s\n", (char*)msg + sizeof( ReqlQueryMessageHeader) );
    uint8_t * msg_chunk = (uint8_t*)msg;
    
	assert(conn);
    assert(msg);
	
	while( totalBytesProcessed < msgLength )
    {
        unsigned long remainingBytes = msgLength - totalBytesProcessed;
        unsigned long msg_chunk_size = (max_msg_buffer_size < remainingBytes) ? max_msg_buffer_size : remainingBytes;

#ifdef _WIN32
		fprintf(stderr, "ReqlSend::preparing to send: \n%.*s\n", msgLength, (char*)msg);
		//ret = send(conn->socket, (const char*)msg_chunk, msg_chunk_size, 0 );
		assert( conn );
		
		if (conn->target->ssl.method != CTSSL_NONE)
		{
			ret = CTSSLWrite(conn->socket, conn->sslContext, msg_chunk, &msg_chunk_size);
			if (ret == SOCKET_ERROR || ret == 0)
			{
				fprintf(stderr, "****ReqlSend::Error %d sending data to server\n", WSAGetLastError());
				return ret;
			}
			//fprintf(stderr, "%d bytes of handshake data sent\n", ret);
			bytesProcessed = ret;
#elif defined(__APPLE__)
		status = SSLWrite(conn->sslContext, msg_chunk, msg_chunk_size, &bytesProcessed);
#endif
		}
		else
		{
			ret = send(conn->socket, msg_chunk, msg_chunk_size, 0);
			if (ret != msg_chunk_size)
			{
				//ffprintf(stderr, stderr, "IO SEND ERROR: ");
				switch (WSAGetLastError()) {
					assert(1 == -0);
#if EAGAIN != EWOULDBLOCK
				case EAGAIN: // EAGAIN == EWOULDBLOCK on some systems, but not others
#endif
				case EWOULDBLOCK:
					//ffprintf(stderr, stderr, "would block\n");
					//return WOLFSSL_CBIO_ERR_WANT_WRITE;
				case ECONNRESET:
					//ffprintf(stderr, stderr, "connection reset\n");
					//return WOLFSSL_CBIO_ERR_CONN_RST;
				case EINTR:
					//ffprintf(stderr, stderr, "socket interrupted\n");
					//return WOLFSSL_CBIO_ERR_ISR;
				case EPIPE:
					//ffprintf(stderr, stderr, "socket EPIPE\n");
					//return WOLFSSL_CBIO_ERR_CONN_CLOSE;
				default:
					//ffprintf(stderr, stderr, "general error\n");
					//return WOLFSSL_CBIO_ERR_GENERAL;
					return ret;
				}

			}

			if (ret == 0) {
				fprintf(stderr, "Connection closed\n");
				return 0;
			}
			bytesProcessed = ret;
		}

		totalBytesProcessed += bytesProcessed;
        msg_chunk += bytesProcessed;
    }
    
	//Any subsequent
    //So for now, our api will wait until the non-blocking socket begins to write to the network
    //before returning
    if( CTSocketWait(conn->socket, conn->event_queue, EVFILT_WRITE | EV_EOF) != conn->socket )
    {
        fprintf(stderr, "something went wrong\n");
        return -1;
    }


    return 0;
}

/***
 *	CTSendWithQueue
 * 
 *  Sends a message buffer to a dedicated platform "Queue" mechanism
 *  for encryption and then transmission over socket
 ***/
CTClientError CTSendWithQueue(CTConnection* conn, void * msg, unsigned long * msgLength)
{
#ifdef _WIN32

	ULONG_PTR dwCompletionKey = (ULONG_PTR)NULL;
	//Send Query Asynchronously with Windows IOCP
	//Create an overlapped connetion object to pass what we need to the iocp callback
	//Instead of allocating memory we will assume the send buffer is large enough and tack it on the end
	CTOverlappedResponse * overlappedQuery = (CTOverlappedResponse*) ( (char*)msg + *msgLength + 1 ); //+1 for null terminator needed for encryption
	//overlappedQuery = (ReqlOverlappedQuery *)malloc( sizeof(ReqlOverlappedQuery) );
	ZeroMemory(overlappedQuery, sizeof(CTOverlappedResponse)); //this is critical for proper overlapped/iocp operation!
	overlappedQuery->Overlapped.hEvent = CreateEvent(NULL, 0, 0, NULL); //Manual vs Automatic Reset Events Affect GetQueued... Operation!!!
	overlappedQuery->buf = (char*)msg;
	overlappedQuery->len = *msgLength;
	overlappedQuery->conn = conn;
	overlappedQuery->queryToken = *(uint64_t*)msg;

	fprintf(stderr, "overlapped->queryBuffer = %s\n", (char*)msg);

	//Post the overlapped object message asynchronously to the socket transmit thread queue using Win32 Overlapped IO and IOCP
	if( !PostQueuedCompletionStatus(conn->socketContext.txQueue, *msgLength, dwCompletionKey, &(overlappedQuery->Overlapped) ) )
	{
		fprintf(stderr, "\nCTSendWithQueue::PostQueuedCompletionStatus failed with error:  %d\n", GetLastError());
		return (CTClientError)GetLastError();
	}

	/*
	//Issue the async send
	//If WSARecv returns 0, the overlapped operation completed immediately and msgLength has been updated
	if( WSASend(conn->socket, &(overlappedConn->wsaBuf), 1, msgLength, overlappedConn->Flags, &(overlappedConn->Overlapped), NULL) == SOCKET_ERROR )
	{
		//WSA_IO_PENDING
		if( WSAGetLastError() != WSA_IO_PENDING )
		{
			fprintf(stderr,  "****ReqlAsyncSend::WSASend failed with error %d \n",  WSAGetLastError() );	
		}
		
		//forward the winsock system error to the client
		return (ReqlDriverError)WSAGetLastError();
	}
	*/

#elif defined(__APPLE__)
	//TO DO?
#endif

	//ReqlSuccess will indicate the async operation finished immediately
	return CTSuccess;

}




/***
 *	CTSendRequestWithToken 
 *
 *	Send a network request and tag it with a request token
 ***/
/*
uint64_t CTSendRequestWithToken(const char ** queryBufPtr, CTConnection * conn, uint64_t queryToken)//, void * options)//, ReqlQueryClosure callback)
{
	int32_t queryStrLength;
    const char* queryBuf = *queryBufPtr;
	queryStrLength = (int32_t)strlen(queryBuf);        
    CTSend(conn, (void*)(*queryBufPtr), queryStrLength);
    
    return queryToken;

}
*/

/**
 *	CTReqlRunQueryWithToken 
 *
 *	Populates a ReqQueryMessageHeader at the start of the outgoing network buffer first
 *  and then synchronously (blocking) sends with CTSend
 ***/
uint64_t CTReqlRunQueryWithToken(const char ** queryBufPtr, CTConnection* conn, uint64_t queryToken)//, void * options)//, ReqlQueryClosure callback)
{
    //ReqlError * errPtr = NULL;
    //ReqlError err = {ReqlDriverErrorClass, ReqlSuccess, NULL};
    
    //assert(!_writing && !_reading);
	int32_t queryHeaderLength;// = sizeof(ReqlQueryMessageHeader);
    int32_t queryStrLength;// = (int32_t)strlen(queryBuf);
	//char * sendBuffer;
	//unsigned long queryMessageLength;
    const char* queryBuf = *queryBufPtr + sizeof(ReqlQueryMessageHeader);
    //fprintf(stderr, "queryBuf = \n\n%s\n\n", queryBuf);

    ReqlQueryMessageHeader * queryHeader = (ReqlQueryMessageHeader*)*queryBufPtr;
    //fprintf(stderr, "sizeof(ReQLHeader) == %lu\n", sizeof(ReqlQueryMessageHeader));
    
    //fprintf(stderr, "ReqlRunQueryWithToken\n\n");
    //assert(*queryBuf && strlen(queryBuf) > sizeof(ReqlQueryMessageHeader));
    
    //Populate the network message with a header and the serialized query JSON string
    queryHeader->token = queryToken;//(conn->queryCount)++;//conn->token;
    queryHeaderLength = sizeof(ReqlQueryMessageHeader);
    queryStrLength = (int32_t)strlen(queryBuf);    
    queryHeader->length = queryStrLength;// + queryHeaderLength;

	//memcpy(queryBuf, &queryHeader, sizeof(ReqlQueryMessageHeader)); //copy query header
    //queryBuf[queryStrLength + queryHeaderLength] = '\0';  //cap with null char
    
    //sendBuffer = (char*)malloc( queryStrLength + queryHeaderLength);

	//copy the header to the buffer
	//memcpy(conn->query_buf[0], &queryHeader, queryHeaderLength); //copy query header
    //memcpy((char*)(conn->query_buf[0])+queryHeaderLength, queryBuf, queryStrLength); //copy query header
    //sendBuffer[queryStrLength + queryHeaderLength] = '\0';  //cap with null char
    
    //_writing = 1;    
    //fprintf(stderr, "sendBuffer = \n\n%s\n", sendBuffer + queryHeaderLength);
    //assert(queryBuf[queryIndex%2]);
    //ReqlSend(conn, (void*)&queryHeader, queryHeaderLength);

	//queryMessageLength = queryStrLength + queryHeaderLength;
    CTSend(conn, (void*)(*queryBufPtr), queryStrLength + sizeof(ReqlQueryMessageHeader));
    //free( sendBuffer);
    //fprintf(stderr, "ReqlRunQueryCtx End\n\n");
    
    //_writing = 0;
    
    return queryHeader->token;

}


/***
 *	CTSendOnQueue
 *
 *	Send a network buffer asynchronoulsy (using IOCP) and tag it with a request token
 ***/
uint64_t CTSendOnQueue(CTConnection * conn, char ** queryBufPtr, unsigned long queryStrLength, uint64_t queryToken)//, void * options)//, ReqlQueryClosure callback)
{
#ifdef _WIN32

	ULONG_PTR dwCompletionKey = (ULONG_PTR)NULL;
	//Send Query Asynchronously with Windows IOCP
	//Create an overlapped connetion object to pass what we need to the iocp callback
	//Instead of allocating memory we will assume the send buffer is large enough and tack it on the end
	//CTOverlappedRequest * overlappedQuery;// = (ReqlOverlappedQuery*) ( (char*)*queryBufPtr + queryStrLength + 1 ); //+1 for null terminator needed for encryption
	//fprintf(stderr, "overlapped->queryBuffer = %s\n", (char*)*queryBufPtr);

	CTOverlappedResponse * overlappedQuery = (CTOverlappedResponse*) ( (char*)*queryBufPtr + queryStrLength + 1 ); //+1 for null terminator needed for encryption
	//overlappedQuery = (ReqlOverlappedQuery *)malloc( sizeof(ReqlOverlappedQuery) );
	ZeroMemory(overlappedQuery, sizeof(CTOverlappedResponse)); //this is critical for proper overlapped/iocp operation!
	overlappedQuery->Overlapped.hEvent = CreateEvent(NULL, 0, 0, NULL); //Manual vs Automatic Reset Events Affect GetQueued... Operation!!!
	overlappedQuery->buf = (char*)*queryBufPtr;
	overlappedQuery->len = queryStrLength;
	overlappedQuery->conn = conn;
	overlappedQuery->queryToken = queryToken;//*(uint64_t*)msg;
	//overlappedQuery->overlappedResponse = overlappedResponse;

	//fprintf(stderr, "overlapped->queryBuffer = %s\n", (char*)*queryBufPtr);
	//Post the overlapped object message asynchronously to the socket transmit thread queue using Win32 Overlapped IO and IOCP
	if( !PostQueuedCompletionStatus(conn->socketContext.txQueue, queryStrLength, dwCompletionKey, &(overlappedQuery->Overlapped) ) )
	{
		fprintf(stderr, "\nCTSendOnQueue::PostQueuedCompletionStatus failed with error:  %d\n", GetLastError());
        assert(1 == 0);
		return (CTClientError)GetLastError();
	}

	/*
	//Issue the async send
	//If WSARecv returns 0, the overlapped operation completed immediately and msgLength has been updated
	if( WSASend(conn->socket, &(overlappedConn->wsaBuf), 1, msgLength, overlappedConn->Flags, &(overlappedConn->Overlapped), NULL) == SOCKET_ERROR )
	{
		//WSA_IO_PENDING
		if( WSAGetLastError() != WSA_IO_PENDING )
		{
			fprintf(stderr,  "****ReqlAsyncSend::WSASend failed with error %d \n",  WSAGetLastError() );	
		}
		
		//forward the winsock system error to the client
		return (ReqlDriverError)WSAGetLastError();
	}
	*/

#elif defined(__APPLE__)
	//TO DO?
#endif

	//ReqlSuccess will indicate the async operation finished immediately
	return queryToken;
}

uint64_t CTSendOnQueue2(CTConnection * conn, char ** queryBufPtr, unsigned long queryStrLength, uint64_t queryToken, CTOverlappedResponse* overlappedResponse)//, void * options)//, ReqlQueryClosure callback)
{
#ifdef _WIN32

	ULONG_PTR dwCompletionKey = (ULONG_PTR)NULL;
	//Send Query Asynchronously with Windows IOCP
	//Create an overlapped connetion object to pass what we need to the iocp callback
	//Instead of allocating memory we will assume the send buffer is large enough and tack it on the end
	//CTOverlappedRequest * overlappedQuery;// = (ReqlOverlappedQuery*) ( (char*)*queryBufPtr + queryStrLength + 1 ); //+1 for null terminator needed for encryption
	//fprintf(stderr, "overlapped->queryBuffer = %s\n", (char*)*queryBufPtr);

	CTOverlappedResponse * overlappedQuery = (CTOverlappedResponse*) ( (char*)*queryBufPtr + queryStrLength + 1 ); //+1 for null terminator needed for encryption
	//overlappedQuery = (ReqlOverlappedQuery *)malloc( sizeof(ReqlOverlappedQuery) );
	ZeroMemory(overlappedQuery, sizeof(CTOverlappedResponse)); //this is critical for proper overlapped/iocp operation!
	overlappedQuery->Overlapped.hEvent = CreateEvent(NULL, 0, 0, NULL); //Manual vs Automatic Reset Events Affect GetQueued... Operation!!!
	overlappedQuery->buf = (char*)*queryBufPtr;
	overlappedQuery->len = queryStrLength;
	overlappedQuery->conn = conn;
	overlappedQuery->queryToken = queryToken;//*(uint64_t*)msg;
	//overlappedQuery->overlappedResponse = overlappedResponse;

	//fprintf(stderr, "overlapped->queryBuffer = %s\n", (char*)*queryBufPtr);
	//Post the overlapped object message asynchronously to the socket transmit thread queue using Win32 Overlapped IO and IOCP
	if( !PostQueuedCompletionStatus(conn->socketContext.txQueue, queryStrLength, dwCompletionKey, &(overlappedQuery->Overlapped) ) )
	{
		fprintf(stderr, "\nCTSendOnQueue::PostQueuedCompletionStatus failed with error:  %d\n", GetLastError());
		return (CTClientError)GetLastError();
	}

	/*
	//Issue the async send
	//If WSARecv returns 0, the overlapped operation completed immediately and msgLength has been updated
	if( WSASend(conn->socket, &(overlappedConn->wsaBuf), 1, msgLength, overlappedConn->Flags, &(overlappedConn->Overlapped), NULL) == SOCKET_ERROR )
	{
		//WSA_IO_PENDING
		if( WSAGetLastError() != WSA_IO_PENDING )
		{
			fprintf(stderr,  "****ReqlAsyncSend::WSASend failed with error %d \n",  WSAGetLastError() );	
		}
		
		//forward the winsock system error to the client
		return (ReqlDriverError)WSAGetLastError();
	}
	*/

#elif defined(__APPLE__)
	//TO DO?
#endif

	//ReqlSuccess will indicate the async operation finished immediately
	return queryToken;
}

//#define CT_MAX_INFLIGHT_CONNECTION_TARGETS 1024
//CTOverlappedTarget CT_INFLIGHT_CONNECTION_TARGETS[CT_MAX_INFLIGHT_CONNECTION_TARGETS];
//static int CT_INFLIGHT_CONNECTION_COUNT = 0;

uint64_t CTTargetConnectOnQueue(CTTarget* target, CTConnectionCallback callback)// char** queryBufPtr, unsigned long queryStrLength)
{
	ULONG_PTR dwCompletionKey = (ULONG_PTR)NULL;
	//Send Query Asynchronously with Windows IOCP
	//Create an overlapped connetion object to pass what we need to the iocp callback
	//Instead of allocating memory we will assume the send buffer is large enough and tack it on the end
	//CTOverlappedRequest * overlappedQuery;// = (ReqlOverlappedQuery*) ( (char*)*queryBufPtr + queryStrLength + 1 ); //+1 for null terminator needed for encryption
	//fprintf(stderr, "overlapped->queryBuffer = %s\n", (char*)*queryBufPtr);

	CTOverlappedTarget* overlappedTarget = &(target->overlappedTarget);// &(CT_INFLIGHT_CONNECTION_TARGETS[CT_INFLIGHT_CONNECTION_COUNT]); //(CTOverlappedTarget*)malloc(sizeof(CTOverlappedTarget));
	//CT_INFLIGHT_CONNECTION_COUNT = ++CT_INFLIGHT_CONNECTION_COUNT % CT_MAX_INFLIGHT_CONNECTION_TARGETS;
																					
	ZeroMemory(overlappedTarget, sizeof(CTOverlappedTarget)); //this is critical for proper overlapped/iocp operation!
	overlappedTarget->Overlapped.hEvent = CreateEvent(NULL, 0, 0, NULL); //Manual vs Automatic Reset Events Affect GetQueued... Operation!!!
	//overlappedTarget->buf = (char*)*queryBufPtr;
	//overlappedTarget->len = queryStrLength;
	target->callback = callback;
	overlappedTarget->target = target;
	overlappedTarget->stage = CT_OVERLAPPED_SCHEDULE;
	//overlappedTarget->callback = callback;
	//overlappedQuery->cursor = (void*)cursor;
	//overlappedQuery->overlappedResponse = overlappedResponse;

	//fprintf(stderr, "overlapped->queryBuffer = %s\n", (char*)*queryBufPtr);
	//Post the overlapped object message asynchronously to the socket transmit thread queue using Win32 Overlapped IO and IOCP
	if (!PostQueuedCompletionStatus(target->cxQueue, sizeof(CTOverlappedTarget), dwCompletionKey, &(overlappedTarget->Overlapped)))
	{
		fprintf(stderr, "\nCTCursorConnectOnQueue::PostQueuedCompletionStatus failed with error:  %d\n", GetLastError());
		return (CTClientError)GetLastError();
	}

	return CTSuccess;
}

uint64_t CTCursorSendOnQueue(CTCursor * cursor, char ** queryBufPtr, unsigned long queryStrLength)
{

#ifdef _WIN32

	ULONG_PTR dwCompletionKey = (ULONG_PTR)NULL;
	//Send Query Asynchronously with Windows IOCP
	//Create an overlapped connetion object to pass what we need to the iocp callback
	//Instead of allocating memory we will assume the send buffer is large enough and tack it on the end
	//CTOverlappedRequest * overlappedQuery;// = (ReqlOverlappedQuery*) ( (char*)*queryBufPtr + queryStrLength + 1 ); //+1 for null terminator needed for encryption
	//fprintf(stderr, "overlapped->queryBuffer = %s\n", (char*)*queryBufPtr);
	
	//store stage input, store buffer ptr, etc before zeroing overlapped (or just consider only zeroing the overlapped portion)
	CTOverlappedStage stage = cursor->overlappedResponse.stage;
	char* queryBuf = *queryBufPtr;// cursor->overlappedResponse.stage;

	if( cursor->conn->target->ssl.method == CTSSL_NONE)
		stage = CT_OVERLAPPED_SEND; //we need to bypass encrypt bc sthis is the ssl handshake

#ifdef CTRANSPORT_WOLFSSL

	//CTCursor* handshakeCursor = CTGetNextPoolCursor();
	//memset(handshakeCursor, 0, sizeof(CTCursor));

	//WolfSSL Callbacks will need a connection object to place on CTCursor's for send/recv
	CTSSLEncryptTransient* transient = (CTSSLEncryptTransient*)&(cursor->requestBuffer[65535 - sizeof(CTSSLEncryptTransient)]);// { 0, 0, NULL };
	transient->wolf_socket_fd = 0;
	transient->messageLen = cursor->overlappedResponse.len;
	transient->messageBuffer = cursor->overlappedResponse.buf;
	wolfSSL_SetIOWriteCtx(cursor->conn->sslContext->ctx, transient); 
#endif

	//if( cursor->overlappedResponse.buf)
	CTOverlappedResponse* overlappedQuery = &(cursor->overlappedResponse);// (CTOverlappedResponse*)((char*)*queryBufPtr + queryStrLength + 1); //+1 for null terminator needed for encryption
	//overlappedQuery = (ReqlOverlappedQuery *)malloc( sizeof(ReqlOverlappedQuery) );
	ZeroMemory(overlappedQuery, sizeof(CTOverlappedResponse)); //this is critical for proper overlapped/iocp operation!
	overlappedQuery->Overlapped.hEvent = CreateEvent(NULL, 0, 0, NULL); //Manual vs Automatic Reset Events Affect GetQueued... Operation!!!
	overlappedQuery->buf = queryBuf;// (char*)*queryBufPtr;
	overlappedQuery->len = queryStrLength;
	//overlappedQuery->conn = conn;
	//overlappedQuery->queryToken = queryToken;//*(uint64_t*)msg;
	overlappedQuery->cursor = (void*)cursor;
	overlappedQuery->stage = stage;
	//overlappedQuery->overlappedResponse = overlappedResponse;

	//fprintf(stderr, "overlapped->queryBuffer = %s\n", (char*)*queryBufPtr);
	//Post the overlapped object message asynchronously to the socket transmit thread queue using Win32 Overlapped IO and IOCP
	if( !PostQueuedCompletionStatus(cursor->conn->socketContext.txQueue, queryStrLength, dwCompletionKey, &(overlappedQuery->Overlapped) ) )
	{
		fprintf(stderr, "\nCTCursorSendOnQueue::PostQueuedCompletionStatus failed with error:  %d\n", GetLastError());
		return (CTClientError)GetLastError();
	}

	cursor->conn->queryCount++;

	/*
	//Issue the async send
	//If WSARecv returns 0, the overlapped operation completed immediately and msgLength has been updated
	if( WSASend(conn->socket, &(overlappedConn->wsaBuf), 1, msgLength, overlappedConn->Flags, &(overlappedConn->Overlapped), NULL) == SOCKET_ERROR )
	{
		//WSA_IO_PENDING
		if( WSAGetLastError() != WSA_IO_PENDING )
		{
			fprintf(stderr,  "****ReqlAsyncSend::WSASend failed with error %d \n",  WSAGetLastError() );	
		}
		
		//forward the winsock system error to the client
		return (ReqlDriverError)WSAGetLastError();
	}
	*/

#elif defined(__APPLE__)
	//TO DO?
#endif

	//ReqlSuccess will indicate the async operation finished immediately
	return cursor->queryToken;
}

/***
 *	CTReqlRunQueryOnQueue
 *
 *  Does the same thing as CTSendOnQueue, but first populates ReQlQueryMessageHeader at the start of the outgoing network buffer
 *	Send a network buffer asynchronoulsy (using IOCP) and tag it with a request token
 ***/
uint64_t CTReQLRunQueryOnQueue(CTConnection * conn, const char ** queryBufPtr, unsigned long queryStrLength, uint64_t queryToken)//, void * options)//, ReqlQueryClosure callback)
{
    unsigned long queryHeaderLength, queryMessageLength;
    ReqlQueryMessageHeader * queryHeader = (ReqlQueryMessageHeader*)*queryBufPtr;
    const char* queryBuf = *queryBufPtr + sizeof(ReqlQueryMessageHeader);
	
    //Populate the network message buffer with a header and the serialized query JSON string
    queryHeader->token = queryToken;//(conn->queryCount)++;//conn->token;
    queryHeaderLength = sizeof(ReqlQueryMessageHeader);
    //queryStrLength = (int32_t)strlen(queryBuf);    
    queryHeader->length = queryStrLength;// + queryHeaderLength;

	fprintf(stderr, "CTReQLRunQueryOnQueue::queryBufPtr (%d) = %.*s", (int)queryStrLength, (int)queryStrLength, *queryBufPtr + sizeof(ReqlQueryMessageHeader));
    queryMessageLength = queryHeaderLength + queryStrLength;
	
	//ReqlSend(conn, (void*)&queryHeader, queryHeaderLength);
	CTSendOnQueue(conn, (char**)(queryBufPtr), queryMessageLength, queryToken);
    
    return queryHeader->token;
}




//this is a client call so it should return ReqlDriverError 
//msgLength must be unsigned long to match DWORD type on WIN32
CTClientError CTAsyncRecv(CTConnection* conn, void * msg, unsigned long offset, unsigned long * msgLength)
{
	//uint64_t queryToken = *(uint64_t*)msg;
#ifdef _WIN32

	
	//We used to tuck the IOCP Overlapped struct directly in the request buffer,
	//This works for responses that can be placed in a single buffer
	//But WSARecv needs page aligned or system allocation granularity aligned memory or it will fail
	CTOverlappedResponse * overlappedResponse;
	//unsigned long overlappedOffset = 0;// *msgLength - sizeof(ReqlOverlappedResponse);
	if (*msgLength > 0)
	{
		//overlappedOffset = *msgLength - sizeof(CTOverlappedResponse);;
		//*msgLength -= sizeof(CTOverlappedResponse);
		*msgLength -= offset;
	}
	//Create an overlapped connetion object to pass what we need to the iocp callback
	//Instead of allocating memory we will assume the recv buffer is large enough and tack it on the end
	//overlappedResponse = (CTOverlappedResponse*) ( (char*)msg + overlappedOffset ); //+1 for null terminator needed for encryption
	overlappedResponse = (ReqlOverlappedResponse *)malloc( sizeof(ReqlOverlappedResponse) );
	ZeroMemory(overlappedResponse, sizeof(CTOverlappedResponse)); //this is critical!
	overlappedResponse->Overlapped.hEvent = CreateEvent(NULL, 0, 0, NULL); //Manual vs Automatic Reset Events Affect GetQueued... Operation!!!
	overlappedResponse->wsaBuf.buf = (char*)msg + offset;//(char*)(conn->response_buf[queryToken%2]);
	overlappedResponse->wsaBuf.len = *msgLength;
	overlappedResponse->buf = (char*)msg;
	overlappedResponse->len = *msgLength;
	overlappedResponse->conn = conn;
	overlappedResponse->Flags = 0;

	//PER_IO_DATA *pPerIoData = new PER_IO_DATA;
	//ZeroMemory(pPerIoData, sizeof(PER_IO_DATA));;
	//pPerIoData->Socket = conn->socket;
	//conn->socketContext.Overlapped.hEvent = WSACreateEvent();
	//conn->socketContext.wsaBuf.buf = (char*)(conn->response_buf);
	//conn->socketContext.wsaBuf.len = REQL_RESPONSE_BUFF_SIZE;

	//Issue the async receive
	//If WSARecv returns 0, the overlapped operation completed immediately and msgLength has been updated
	fprintf(stderr, "CTAsyncRecv::Requesting %lu Bytes\n", *msgLength);
		fprintf(stderr, "CTAsyncRecv::conn->socket = %d\n", conn->socket);
	if( WSARecv(conn->socket, &(overlappedResponse->wsaBuf), 1, msgLength, &(overlappedResponse->Flags), &(overlappedResponse->Overlapped), NULL) == SOCKET_ERROR )
	{
		//WSA_IO_PENDING
		if( WSAGetLastError() != WSA_IO_PENDING )
		{
			fprintf(stderr,  "****ReqlAsyncRecv::WSARecv failed with error %d \n",  WSAGetLastError() );	
		} 
		
		//forward the winsock system error to the client
		return (CTClientError)WSAGetLastError();
	}


#elif defined(__APPLE__)
	//TO DO?
#endif

	//ReqlSuccess will indicate the async operation finished immediately
	return CTSuccess;

}


CTClientError CTAsyncRecv2(CTConnection* conn, void * msg, unsigned long offset, unsigned long * msgLength, uint64_t queryToken, CTOverlappedResponse** overlappedResponsePtr)
{
	//uint64_t queryToken = *(uint64_t*)msg;
#ifdef _WIN32

	CTOverlappedResponse * overlappedResponse = *overlappedResponsePtr;
	DWORD recvLength = *msgLength;
	
	//if message length is greater than 0, subtract offset
	*msgLength -= (*msgLength > 0) * offset; 
	//if message length was zero then so its recvLength, use the overlappedResponse->len property to get the recv length
	recvLength = *msgLength + (*msgLength == 0 ) * overlappedResponse->len; 

	//Can we avoid zeroing this memory every time?
	ZeroMemory(overlappedResponse, sizeof(CTOverlappedResponse)); //this is critical!
	overlappedResponse->Overlapped.hEvent = CreateEvent(NULL, 0, 0, NULL); //Manual vs Automatic Reset Events Affect GetQueued... Operation!!!
	overlappedResponse->wsaBuf.buf = (char*)msg + offset;//(char*)(conn->response_buf[queryToken%2]);
	overlappedResponse->wsaBuf.len = *msgLength;
	overlappedResponse->buf = (char*)msg;
	overlappedResponse->len = recvLength;//*msgLength;
	overlappedResponse->conn = conn;
	overlappedResponse->queryToken = queryToken;
	overlappedResponse->Flags = 0;


	//Issue the async receive
	//If WSARecv returns 0, the overlapped operation completed immediately and msgLength has been updated
	fprintf(stderr, "CTAsyncRecv::Requesting %lu Bytes\n", *msgLength);
	//	fprintf(stderr, "CTAsyncRecv::conn->socket = %d\n", conn->socket);
	if( WSARecv(conn->socket, &(overlappedResponse->wsaBuf), 1, msgLength, &(overlappedResponse->Flags), &(overlappedResponse->Overlapped), NULL) == SOCKET_ERROR )
	{
		//WSA_IO_PENDING
		if( WSAGetLastError() != WSA_IO_PENDING )
		{
			fprintf(stderr,  "****ReqlAsyncRecv::WSARecv failed with error %d \n",  WSAGetLastError() );	
		} 
		
		//forward the winsock system error to the client
		return (CTClientError)WSAGetLastError();
	}


#elif defined(__APPLE__)
	//TO DO?
#endif

	//ReqlSuccess will indicate the async operation finished immediately
	return CTSuccess;

}


CTClientError CTTargetAsyncRecvFrom(CTOverlappedTarget** overlappedTargetPtr, void* msg, unsigned long offset, unsigned long* msgLength)
{
#ifdef _WIN32
	CTOverlappedTarget* overlappedTarget = *overlappedTargetPtr;
	DWORD recvLength = *msgLength;

	//if message length is greater than 0, subtract offset
	*msgLength -= (*msgLength > 0) * offset;
	//if message length was zero then so its recvLength, use the overlappedResponse->len property as input to get the recv length
	recvLength = *msgLength + (*msgLength == 0);// *overlappedTarget->cursor->len;

	//CTOverlappedTarget* overlappedTarget = (CTOverlappedTarget*)service->ctx;
	//overlappedTarget = (CTOverlappedTarget*)malloc(sizeof(CTOverlappedTarget));
	ZeroMemory(overlappedTarget, sizeof(WSAOVERLAPPED)); //this is critical for proper overlapped/iocp operation!
	overlappedTarget->Overlapped.hEvent = CreateEvent(NULL, 0, 0, NULL); //Manual vs Automatic Reset Events Affect GetQueued... Operation!!!
	//overlappedTarget->buf = (char*)*queryBufPtr;
	//overlappedTarget->len = queryStrLength;
	//overlappedTarget->target = service;
	//overlappedTarget->callback = callback;
	//overlappedTarget->target->callback = callback;
	overlappedTarget->stage = CT_OVERLAPPED_RECV_FROM;
	overlappedTarget->Flags = 0;

	CTCursor* cursor = overlappedTarget->cursor;

	overlappedTarget->cursor->overlappedResponse.wsaBuf.buf = (char*)msg + offset;//(char*)(conn->response_buf[queryToken%2]);
	overlappedTarget->cursor->overlappedResponse.wsaBuf.len = *msgLength;


	//Issue the async receive
	//If WSARecv returns 0, the overlapped operation completed immediately and msgLength has been updated
//#ifdef _DEBUG
	fprintf(stderr, "CTAsyncRecvFrom::Requesting %lu Bytes\n", *msgLength);
	//	fprintf(stderr, "CTAsyncRecv::conn->socket = %d\n", conn->socket);
//#endif
	assert(overlappedTarget->target);

	int fromlen = sizeof(overlappedTarget->target->url.addr);
	if (WSARecvFrom(overlappedTarget->target->socket, &(overlappedTarget->cursor->overlappedResponse.wsaBuf), 1, msgLength, &(overlappedTarget->Flags), (struct sockaddr*)&(overlappedTarget->target->url.addr), &fromlen, &(overlappedTarget->Overlapped), NULL) == SOCKET_ERROR)
	{
		//WSA_IO_PENDING
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			//TO DO: move to an error log
			fprintf(stderr, "****CTAsyncRecvFrom::WSARecvFrom failed with error %d \n", WSAGetLastError());
			assert(1 == 0);
		}

		//forward the winsock system error to the client
		return (CTClientError)WSAGetLastError();
	}


#elif defined(__APPLE__)
	//TO DO?
#endif

	//ReqlSuccess will indicate the async operation finished immediately
	return CTSuccess;

}


CTClientError CTCursorAsyncRecv(CTOverlappedResponse** overlappedResponsePtr, void * msg, unsigned long offset, unsigned long * msgLength)
{
#ifdef _WIN32
	CTOverlappedResponse * overlappedResponse = *overlappedResponsePtr;
	DWORD recvLength = *msgLength;
	
	//if message length is greater than 0, subtract offset
	*msgLength -= (*msgLength > 0) * offset; 
	//if message length was zero then so its recvLength, use the overlappedResponse->len property as input to get the recv length
	recvLength = *msgLength + (*msgLength == 0 ) * overlappedResponse->len; 

	//Our CoreTransport housekeeping properties (cursor, cursor->conn, and queryToken) have already been set on the Overlapped struct as part of CTCursor initialization
	//Now populate the overlapped struct's WSABUF needed for IOCP to read socket buffer into (as well as our references to buf/len pointing to the start of the buffer to help as we decrypt [in place])
	//Question:  Can we avoid zeroing this memory every time?
	//Anser:	 Yes -- sort of -- we only need to zero the overlapped portion (whether we are reusing the OVERLAPPED struct or not...and we are) 
	ZeroMemory(overlappedResponse, sizeof(WSAOVERLAPPED)); //this is critical!
	overlappedResponse->Overlapped.hEvent = CreateEvent(NULL, 0, 0, NULL); //Manual vs Automatic Reset Events Affect GetQueued... Operation!!!
	overlappedResponse->wsaBuf.buf = (char*)msg + offset;//(char*)(conn->response_buf[queryToken%2]);
	overlappedResponse->wsaBuf.len = *msgLength;
	overlappedResponse->buf = (char*)msg;
	overlappedResponse->len = recvLength;//*msgLength;
	//overlappedResponse->conn = ((CTCursor*)overlappedResponse->cursor)->conn;
	//overlappedResponse->queryToken = queryToken;
	//overlappedResponse->cursor = (void*)cursor;
	overlappedResponse->Flags = 0;


	//Issue the async receive
	//If WSARecv returns 0, the overlapped operation completed immediately and msgLength has been updated
#ifdef _DEBUG
	//fprintf(stderr, "CTAsyncRecv::Requesting %lu Bytes\n", *msgLength);
	//	fprintf(stderr, "CTAsyncRecv::conn->socket = %d\n", conn->socket);
#endif

	if( WSARecv(((CTCursor*)overlappedResponse->cursor)->conn->socket, &(overlappedResponse->wsaBuf), 1, msgLength, &(overlappedResponse->Flags), &(overlappedResponse->Overlapped), NULL) == SOCKET_ERROR )
	{
		//WSA_IO_PENDING
		int err = WSAGetLastError();
		if( err != WSA_IO_PENDING )
		{
			//TO DO: move to an error log
			fprintf(stderr,  "****CTAsyncRecv::WSARecv failed with error %d \n",  WSAGetLastError() );	
            assert(1 == 0);
		} 
		
		//forward the winsock system error to the client
		return (CTClientError)WSAGetLastError();
	}


#elif defined(__APPLE__)
	//TO DO?
#endif

	//ReqlSuccess will indicate the async operation finished immediately
	return CTSuccess;

}


CTClientError CTCursorRecvFromOnQueue(CTOverlappedResponse** overlappedResponsePtr, void* msg, unsigned long offset, unsigned long* msgLength)
{
#ifdef _WIN32
	CTOverlappedResponse* overlappedResponse = *overlappedResponsePtr;
	DWORD recvLength = *msgLength;
	ULONG_PTR dwCompletionKey = (ULONG_PTR)NULL;

	//if message length is greater than 0, subtract offset
	*msgLength -= (*msgLength > 0) * offset;
	//if message length was zero then so its recvLength, use the overlappedResponse->len property as input to get the recv length
	recvLength = *msgLength + (*msgLength == 0) * overlappedResponse->len;

	//Our CoreTransport housekeeping properties (cursor, cursor->conn, and queryToken) have already been set on the Overlapped struct as part of CTCursor initialization
	//Now populate the overlapped struct's WSABUF needed for IOCP to read socket buffer into (as well as our references to buf/len pointing to the start of the buffer to help as we decrypt [in place])
	//Question:  Can we avoid zeroing this memory every time?
	//Anser:	 Yes -- sort of -- we only need to zero the overlapped portion (whether we are reusing the OVERLAPPED struct or not...and we are) 
	ZeroMemory(overlappedResponse, sizeof(WSAOVERLAPPED)); //this is critical!
	overlappedResponse->Overlapped.hEvent = CreateEvent(NULL, 0, 0, NULL); //Manual vs Automatic Reset Events Affect GetQueued... Operation!!!
	overlappedResponse->wsaBuf.buf = (char*)msg + offset;//(char*)(conn->response_buf[queryToken%2]);
	overlappedResponse->wsaBuf.len = *msgLength;
	overlappedResponse->buf = (char*)msg;
	overlappedResponse->len = recvLength;//*msgLength;
	//overlappedResponse->conn = ((CTCursor*)overlappedResponse->cursor)->conn;
	//overlappedResponse->queryToken = queryToken;
	//overlappedResponse->cursor = (void*)cursor;
	overlappedResponse->Flags = 0;
	//overlappedResponse->stage = CT_OVERLAPPED_SCHEDULE;

	//Issue the async receive
	//If WSARecv returns 0, the overlapped operation completed immediately and msgLength has been updated
#ifdef _DEBUG
	fprintf(stderr, "CTCursorRecvOnQueue::Scheduling %lu Bytes\n", *msgLength);
	//	fprintf(stderr, "CTAsyncRecv::conn->socket = %d\n", conn->socket);
#endif

	/*
	if (WSARecv(((CTCursor*)overlappedResponse->cursor)->conn->socket, &(overlappedResponse->wsaBuf), 1, msgLength, &(overlappedResponse->Flags), &(overlappedResponse->Overlapped), NULL) == SOCKET_ERROR)
	{
		//WSA_IO_PENDING
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			//TO DO: move to an error log
			fprintf(stderr, "****ReqlAsyncRecv::WSARecv failed with error %d \n", WSAGetLastError());
			assert(1 == 0);
		}

		//forward the winsock system error to the client
		return (CTClientError)WSAGetLastError();
	}
	*/

	//fprintf(stderr, "overlapped->queryBuffer = %s\n", (char*)*queryBufPtr);
	//Post the overlapped object message asynchronously to the socket transmit thread queue using Win32 Overlapped IO and IOCP
	if (!PostQueuedCompletionStatus(((CTCursor*)overlappedResponse->cursor)->conn->socketContext.rxQueue, *msgLength, dwCompletionKey, &(overlappedResponse->Overlapped)))
	{
		fprintf(stderr, "\nCTCursorSendOnQueue::PostQueuedCompletionStatus failed with error:  %d\n", GetLastError());
		return (CTClientError)GetLastError();
	}

#elif defined(__APPLE__)
	//TO DO?
#endif

	//ReqlSuccess will indicate the async operation finished immediately
	return CTSuccess;

}

CTClientError CTCursorRecvOnQueue(CTOverlappedResponse** overlappedResponsePtr, void* msg, unsigned long offset, unsigned long* msgLength)
{
#ifdef _WIN32
	CTOverlappedResponse* overlappedResponse = *overlappedResponsePtr;
	DWORD recvLength = *msgLength;
	ULONG_PTR dwCompletionKey = (ULONG_PTR)NULL;

	//if message length is greater than 0, subtract offset
	*msgLength -= (*msgLength > 0) * offset;
	//if message length was zero then so its recvLength, use the overlappedResponse->len property as input to get the recv length
	recvLength = *msgLength + (*msgLength == 0) * overlappedResponse->len;

	//Our CoreTransport housekeeping properties (cursor, cursor->conn, and queryToken) have already been set on the Overlapped struct as part of CTCursor initialization
	//Now populate the overlapped struct's WSABUF needed for IOCP to read socket buffer into (as well as our references to buf/len pointing to the start of the buffer to help as we decrypt [in place])
	//Question:  Can we avoid zeroing this memory every time?
	//Anser:	 Yes -- sort of -- we only need to zero the overlapped portion (whether we are reusing the OVERLAPPED struct or not...and we are) 
	ZeroMemory(overlappedResponse, sizeof(WSAOVERLAPPED)); //this is critical!
	overlappedResponse->Overlapped.hEvent = CreateEvent(NULL, 0, 0, NULL); //Manual vs Automatic Reset Events Affect GetQueued... Operation!!!
	overlappedResponse->wsaBuf.buf = (char*)msg + offset;//(char*)(conn->response_buf[queryToken%2]);
	overlappedResponse->wsaBuf.len = *msgLength;
	overlappedResponse->buf = (char*)msg;
	overlappedResponse->len = recvLength;//*msgLength;
	//overlappedResponse->conn = ((CTCursor*)overlappedResponse->cursor)->conn;
	//overlappedResponse->queryToken = queryToken;
	//overlappedResponse->cursor = (void*)cursor;
	overlappedResponse->Flags = 0;
	//overlappedResponse->stage = CT_OVERLAPPED_SCHEDULE;

	//Issue the async receive
	//If WSARecv returns 0, the overlapped operation completed immediately and msgLength has been updated
#ifdef _DEBUG
	fprintf(stderr, "CTCursorRecvOnQueue::Scheduling %lu Bytes\n", *msgLength);
	//	fprintf(stderr, "CTAsyncRecv::conn->socket = %d\n", conn->socket);
#endif

	/*
	if (WSARecv(((CTCursor*)overlappedResponse->cursor)->conn->socket, &(overlappedResponse->wsaBuf), 1, msgLength, &(overlappedResponse->Flags), &(overlappedResponse->Overlapped), NULL) == SOCKET_ERROR)
	{
		//WSA_IO_PENDING
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			//TO DO: move to an error log
			fprintf(stderr, "****ReqlAsyncRecv::WSARecv failed with error %d \n", WSAGetLastError());
			assert(1 == 0);
		}

		//forward the winsock system error to the client
		return (CTClientError)WSAGetLastError();
	}
	*/

	//fprintf(stderr, "overlapped->queryBuffer = %s\n", (char*)*queryBufPtr);
	//Post the overlapped object message asynchronously to the socket transmit thread queue using Win32 Overlapped IO and IOCP
	if (!PostQueuedCompletionStatus(((CTCursor*)overlappedResponse->cursor)->conn->socketContext.rxQueue, *msgLength, dwCompletionKey, &(overlappedResponse->Overlapped)))
	{
		fprintf(stderr, "\nCTCursorSendOnQueue::PostQueuedCompletionStatus failed with error:  %d\n", GetLastError());
		return (CTClientError)GetLastError();
	}

#elif defined(__APPLE__)
	//TO DO?
#endif

	//ReqlSuccess will indicate the async operation finished immediately
	return CTSuccess;

}

//#pragma comment(lib, "ws2_32.lib")

int CTSocketConnectOnQueue(CTSocket socketfd, CTTarget* service, CTConnectionCallback callback)
{


	OSStatus status;
	DWORD dwNumBytes = 0;
	GUID guid = WSAID_CONNECTEX;
	LPFN_CONNECTEX ConnectEx = NULL;
	ULONG_PTR dwCompletionKey = (ULONG_PTR)NULL;

	struct CTConnection conn = { 0 };
	struct CTError error = { (CTErrorClass)0,0,0 };    //Reql API client functions will generally return ints as errors

	struct sockaddr_in addr;


	//Resolve hostname asynchronously   
	/*
	// Resolve hostname synchronously
	 struct hostent *host = NULL;
	 if ( (host = gethostbyname(service->host)) == NULL )
	{
		fprintf(stderr, "gethostbyname failed with err:  %d\n", errno);
		perror(service->host);
		return ReqlDomainError;
	}
	fprintf(stderr, "ReqlSocketConnect 3\n");
	*/

	

	if (WSAIoctl(socketfd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &ConnectEx, sizeof(ConnectEx), &dwNumBytes, NULL, NULL) != 0)
	{
		assert(1 == 0);
	}

	char* target_host = service->proxy.host ? service->proxy.host : service->url.host;
	short target_port = service->proxy.host ? service->proxy.port : service->url.port;

	//push past any prefix if it's there
	char* host_prefix = strstr(target_host, "://");
	if (host_prefix) target_host = host_prefix + 3;

	// Fill a sockaddr_in socket host address and port (why do we copy the already resolved target addr to this local var?)
	//bzero(&addr, sizeof(addr));
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = service->url.addr.sin_addr.s_addr;//*(in_addr_t*)(service->url.addr.sa_data);//*(in_addr_t*)(host->h_addr);  //set host ip addr
	addr.sin_port = htons(target_port);
	
	service->socket = socketfd;
	
	//assert(service->ctx);
	CTOverlappedTarget* overlappedTarget = (CTOverlappedTarget*)&(service->overlappedTarget);// service->ctx;
	//overlappedTarget = (CTOverlappedTarget*)malloc(sizeof(CTOverlappedTarget));
	ZeroMemory(overlappedTarget, sizeof(WSAOVERLAPPED)); //this is critical for proper overlapped/iocp operation!
	overlappedTarget->Overlapped.hEvent = CreateEvent(NULL, 0, 0, NULL); //Manual vs Automatic Reset Events Affect GetQueued... Operation!!!
	//overlappedTarget->buf = (char*)*queryBufPtr;
	//overlappedTarget->len = queryStrLength;
	overlappedTarget->target = service;
	//overlappedTarget->callback = callback;
	overlappedTarget->target->callback = callback;
	overlappedTarget->stage = CT_OVERLAPPED_EXECUTE;
	
	/* ConnectEx requires the socket to be initially bound. */
	{
		struct sockaddr_in baddr;
		ZeroMemory(&baddr, sizeof(baddr));
		baddr.sin_family = AF_INET;
		baddr.sin_addr.s_addr = INADDR_ANY;
		baddr.sin_port = 0;
		int rc = bind(socketfd, (SOCKADDR*)&baddr, sizeof(baddr));
		if (rc != 0) {
			fprintf(stderr, "bind failed: %d\n", WSAGetLastError());
			CTSocketClose(socketfd);
			perror(target_host);
			return CTSocketBindError;
		}
	}

	//OVERLAPPED ol;
	//ZeroMemory(&ol, sizeof(ol));

	if (ConnectEx(socketfd, (struct sockaddr*)&addr, sizeof(addr), NULL, 0, NULL, &(overlappedTarget->Overlapped)) == FALSE)
	{

		if (WSAGetLastError() == ERROR_IO_PENDING)
		{
			fprintf(stderr, "ConnectEx pending\n");

			
			
			
		}
		else
		{
			fprintf(stderr, "ConnectEx failed: %d\n", WSAGetLastError());
			CTSocketClose(socketfd);
			perror(service->url.host);
			return CTSocketConnectError;
		}
	}
	else
	{
		fprintf(stderr, "ConnectEx succeeded immediately\n");
	}

	//The connection completed successfully, allocate memory to return to the client
	//error.id = CTSuccess;

//CONN_CALLBACK:
	//fprintf(stderr, "before callback!\n");
	//conn.target = overlappedTarget->target;
	//overlappedTarget->callback(&error, &conn);
	//fprintf(stderr, "After callback!\n");

	return CTSuccess;
}

//A helper function to perform a socket connection to a given service
//Will either return an kqueue fd associated with the input (blocking or non-blocking) socket
//or an error code
coroutine int CTSocketConnect(CTSocket socketfd, CTTarget * service)
{
    //fprintf(stderr, "CTSocketConnect\n");
    //yield();
    struct sockaddr_in addr;

    //Resolve hostname asynchronously   
    /*
    // Resolve hostname synchronously
     struct hostent *host = NULL;
     if ( (host = gethostbyname(service->host)) == NULL )
    {
        fprintf(stderr, "gethostbyname failed with err:  %d\n", errno);
        perror(service->host);
        return ReqlDomainError;
    }
    fprintf(stderr, "ReqlSocketConnect 3\n");
    */
    
    // Fill a sockaddr_in socket host address and port (why do we copy the already resolved target addr to this local var?)
    //bzero(&addr, sizeof(addr));
	memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = service->url.addr.sin_addr.s_addr;//*(in_addr_t*)(service->url.addr.sa_data);//*(in_addr_t*)(host->h_addr);  //set host ip addr
    addr.sin_port = service->proxy.host ? htons(service->proxy.port) : htons(service->url.port);
    
	char* connect_host = service->proxy.host ? service->proxy.host : service->url.host;

	//push past any prefix if it's there
	char* host_prefix = strstr(connect_host, "://");
	if (host_prefix) connect_host = host_prefix + 3;

    /*
    struct sockaddr_in * addr_in = (struct sockaddr_in*)&(service->url.addr);
    addr_in->sin_family = AF_INET;
    //addr_in->sin_addr.s_addr = AF_INET;
    addr_in->sin_port = service->port;
    */
    
#ifdef _WIN32
		// Connect the bsd socket to the remote server
	if ( WSAConnect(socketfd, (struct sockaddr*)&addr, sizeof(addr), NULL, NULL, NULL, NULL) == SOCKET_ERROR )
#elif defined(__APPLE__)
	// Connect the bsd socket to the remote server
    if ( connect(socketfd, (struct sockaddr*)&addr, sizeof(addr)) != 0 ) //== SOCKET_ERROR
#endif
    {
		int error = WSAGetLastError();
        //errno EINPROGRESS is expected for non blocking sockets
        if( error != WSAEINPROGRESS )
        {
            fprintf(stderr, "socket failed to connect to %s with error: %d\n", connect_host, WSAGetLastError());
            CTSocketClose(socketfd);
            perror(connect_host);
            return CTSocketConnectError;
        }
        else
        {
			fprintf(stderr, "\nReqlSocketConnect EINPROGRESS\n");
        }

    }

    //fprintf(stderr, "ReqlSocketConnect End\n");
    return CTSuccess;
}



//#define CT_REQL_NUM_NONCE_BYTES 20
//const char * clientKeyData = "Client Key";
//const char * serverKeyData = "Server Key";
//const char* authKey = "\"authentication\"";
//const char * successKey = "\"success\"";

int CTReQLHandshake(CTConnection * r, CTTarget* service)
{
	    OSStatus status;

	

    //SCRAM HMAC SHA-256 Auth Buffer Declarations

    char clientNonce[CT_REQL_NUM_NONCE_BYTES];                //a buffer to hold randomly generated UTF8 characters
    
    char saltedPassword[CC_SHA256_DIGEST_LENGTH];   //a buffer to hold hmac salt at each iteration

    char clientKey[CC_SHA256_DIGEST_LENGTH];        //a buffer to hold SCRAM generated clientKey
    char serverKey[CC_SHA256_DIGEST_LENGTH];        //a buffer to hold SCRAM generated serverKey

    char storedKey[CC_SHA256_DIGEST_LENGTH];        //a buffer to hold the SHA-256 hash of clientKey
    char clientSignature[CC_SHA256_DIGEST_LENGTH];  //used in combination with clientKey to generate a clientProof that the server uses to validate the client
    char serverSignature[CC_SHA256_DIGEST_LENGTH];  //used to generated a serverSignature the client uses to validate that sent by the server
    char clientProof[CC_SHA256_DIGEST_LENGTH];


    
    //SCRAM Message Exchange Buffer Declarations
    //  AuthMessage := client-first-message-bare + "," + server-first-message + "," + client-final-message-without-proof
    
    //RethinkDB JSON message exchange buffer declarations
    char MAGIC_NUMBER_RESPONSE_JSON[1024];

    char SERVER_FIRST_MESSAGE_JSON[1024];
    char CLIENT_FINAL_MESSAGE_JSON[1024];
    char SERVER_FINAL_MESSAGE_JSON[1024];
    
	//All buffers provided to ReqlSend must be in-place writeable
	char REQL_MAGIC_NUMBER_BUF[4] = "\xc3\xbd\xc2\x34";
	char CLIENT_FIRST_MESSAGE_JSON[1024] = "\0";
	char CLIENT_FINAL_MESSAGE[1024] = "\0";
    char SCRAM_AUTH_MESSAGE[1024] = "\0";

	char* successVal = NULL;
	char* authValue = NULL;
	char* tmp = NULL;
	void * base64SS = NULL;
	void * base64Proof = NULL;
	char * salt = NULL;

	char * mnResponsePtr = NULL;
	char * sFirstMessagePtr = NULL;
	char * sFinalMessagePtr = NULL;
	
	char delim[] = ",";
    char *token = NULL;//strtok(authValue, delim);
    char * serverNonce = NULL;
    char * base64Salt = NULL;
    char * saltIterations = NULL;

	bool magicNumberSuccess = false;
	bool sfmSuccess = false;

	memset(SERVER_FIRST_MESSAGE_JSON, 0, 1024);

	//Declare some variables to store the variable length of the buffers
	//as we perform message exchange/SCRAM Auth
	unsigned long AuthMessageLength = 0;/// = 0;
	unsigned long magicNumberResponseLength = 0;// = 1024;
	unsigned long readLength = 0;// = 1024;
	unsigned long authLength = 0;// = 0;
	size_t base64SSLength = 0;
	size_t base64ProofLength = 0;
	size_t saltLength = 0;

	int charIndex;


	//Declare some variables to store the variable length of the buffers
    //as we perform message exchange/SCRAM Auth
    AuthMessageLength = 0;
    magicNumberResponseLength = 1024;
    readLength = 1024;
	authLength = 0;
	saltLength = 0;

	fprintf(stderr, "CTReqlSASLHandshake begin\n");
    //  Generate a random client nonce for sasl scram sha 256 required by RethinkDB 2.3
    //  for populating the SCRAM client-first-message
    //  We must wrap SecRandomCopyBytes in a function and use it one byte at a time
    //  to ensure we get a valid UTF8 String (so this is pretty slow)
    if( (status = ca_scram_gen_rand_bytes( clientNonce, CT_REQL_NUM_NONCE_BYTES )) != noErr)
    {
        fprintf(stderr, "ca_scram_gen_rand_bytes failed with status:  %d\n", status);
        return CTSCRAMEncryptionError;
    }
    
    //  Populate SCRAM client-first-message (ie client-first-message containing username and random nonce)
    //  We choose to forgo creating a dedicated buffer for the client-first-message and populate
    //  directly into the SCRAM AuthMessage Accumulation buffer
    sprintf(SCRAM_AUTH_MESSAGE, "n=%s,r=%.*s", service->user, (int)CT_REQL_NUM_NONCE_BYTES, clientNonce);
    AuthMessageLength += (unsigned long)strlen(SCRAM_AUTH_MESSAGE);
    //fprintf(stderr, "CLIENT_FIRST_MESSAGE = \n\n%s\n\n", SCRAM_AUTH_MESSAGE);
    
    //  Populate client-first-message in JSON wrapper expected by RethinkDB ReQL Service
    //static const char *JSON_PROTOCOL_VERSION = "{\"protocol_version\":0,\0";
    //static const char *JSON_AUTH_METHOD = "\"authentication_method\":\"SCRAM-SHA-256\",\0";
    strcat( CLIENT_FIRST_MESSAGE_JSON, REQL_JSON_PROTOCOL_VERSION);//, strlen(JSON_PROTOCOL_VERSION));
    strcat( CLIENT_FIRST_MESSAGE_JSON, REQL_JSON_AUTH_METHOD);//, strlen(JSON_PROTOCOL_VERSION));
    sprintf(CLIENT_FIRST_MESSAGE_JSON + strlen(CLIENT_FIRST_MESSAGE_JSON), "\"authentication\":\"n,,%s\"}", SCRAM_AUTH_MESSAGE);
    //fprintf(stderr, "CLIENT_FIRST_MESSAGE_JSON = \n\n%s\n\n", CLIENT_FIRST_MESSAGE_JSON);
    
    //  The client-first-message is already in the AuthMessage buffer
    //  So we don't need to explicitly copy it
    //  But we do need to add a comma after the client-first-message
    //  and a null char to prepare for appending the next SCRAM messages
    SCRAM_AUTH_MESSAGE[AuthMessageLength] = ',';
    AuthMessageLength +=1;
    SCRAM_AUTH_MESSAGE[AuthMessageLength] = '\0';
    
    //  Asynchronously send (ie non-blocking send) both magic number and client-first-message-json
    //  buffers in succession over the (non-blocking) socket TLS+TCP connection
    CTSend(r, (void*)REQL_MAGIC_NUMBER_BUF, 4);
   
    //yield();
    
	//  Synchronously Wait for magic number response and SCRAM server-first-message
    //  --For now we just care about getting 'success: true' from the magic number response
    //  --The server-first-message contains salt, iteration count and a server nonce appended to our nonce for use in SCRAM HMAC SHA-256 Auth
    mnResponsePtr = (char*)CTRecv(r, MAGIC_NUMBER_RESPONSE_JSON, &magicNumberResponseLength);
    
	CTSend(r, CLIENT_FIRST_MESSAGE_JSON, strlen(CLIENT_FIRST_MESSAGE_JSON) + 1);  //Note:  Raw JSON messages sent to ReQL Server always needs the extra null character to determine EOF!!!
																				 //       However, Reql Query Messages must NOT contain the additional null character
	
	sFirstMessagePtr = (char*)CTRecv(r, SERVER_FIRST_MESSAGE_JSON, &readLength);
    fprintf(stderr, "MAGIC_NUMBER_RESPONSE_JSON = \n\n%s\n\n", mnResponsePtr);
	fprintf(stderr, "SERVER_FIRST_MESSAGE_JSON = \n\n%s\n\n", sFirstMessagePtr);
    
	if (!mnResponsePtr || !sFirstMessagePtr)
		return CTSASLHandshakeError;
    //  Parse the magic number json response buffer to see if magic number exchange was successful
    //  Note:  Here we are counting on the fact that rdb server json does not send whitespace!!!
    //bool magicNumberSuccess = false;
    //const char * successKey = "\"success\"";
    //char * successVal;
    if( (successVal = strstr(mnResponsePtr, REQL_SUCCESS_KEY)) != NULL )
    {
        successVal += strlen(REQL_SUCCESS_KEY) + 1;
        //if( *successVal == ':') successVal += 1;
        if( *successVal == '"') successVal+=1;
        if( strncmp( successVal, "true", MIN(4, strlen(successVal) ) ) == 0 )
            magicNumberSuccess = true;
    }
    if( !magicNumberSuccess )
    {
        fprintf(stderr, "RDB Magic Number Response Failure!\n");
        return CTSASLHandshakeError;
    }
    
    //  Parse the server-first-message json response buffer to see if client-first-message exchange was successful
    //  Note:  Here we are counting on the fact that rdb server json does not send whitespace!!!
    //bool sfmSuccess = false;
    if( (successVal = strstr(sFirstMessagePtr, REQL_SUCCESS_KEY)) != NULL )
    {
        successVal += strlen(REQL_SUCCESS_KEY) + 1;
        //if( *successVal == ':') successVal += 1;
        if( *successVal == '"') successVal+=1;
        if( strncmp( successVal, "true", MIN(4, strlen(successVal) ) ) == 0 )
            sfmSuccess = true;
    }
    if( !sfmSuccess )//|| !authValue || authLength < 1 )
    {
        fprintf(stderr, "RDB server-first-message response failure!\n");
        return CTSASLHandshakeError;
    }
    
    //  The nonce, salt, and iteration count (ie server-first-message) we need for SCRAM Auth are in the 'authentication' field
    //const char* authKey = "\"authentication\"";
    authValue = strstr(sFirstMessagePtr, REQL_AUTH_KEY);
    //size_t authLength = 0;
    if( authValue )
    {
        authValue += strlen(REQL_AUTH_KEY) + 1;
        //if( *successVal == ':') successVal += 1;
        if( *authValue == '"')
        {
            authValue+=1;
            tmp = (char*)authValue;
            while( *(tmp) != '"' ) tmp++;
            authLength = (unsigned long)(tmp - authValue);
        }
    }

    //  Copy the server-first-message UTF8 string to the AuthMessage buffer
    strncat( SCRAM_AUTH_MESSAGE, authValue, authLength);
    AuthMessageLength += authLength;
    SCRAM_AUTH_MESSAGE[AuthMessageLength] = ',';
    AuthMessageLength +=1;
    SCRAM_AUTH_MESSAGE[AuthMessageLength] = '\0';
    
    //  Parse the SCRAM server-first-message fo salt, iteration count and nonce
    //  We are looking for r=, s=, i=
    //char delim[] = ",";
    token = strtok(authValue, delim);
    //char * serverNonce = NULL;
    //char * base64Salt = NULL;
    //char * saltIterations = NULL;
    while(token != NULL)
    {
        if( token[0] == 'r'  )
            serverNonce = &(token[2]);
        else if( token[0] == 's' )
            base64Salt = &(token[2]);
        else if( token[0] == 'i' )
            saltIterations = &(token[2]);
        token = strtok(NULL, delim);
    }
    assert(serverNonce);
    assert(base64Salt);
    assert(saltIterations);
    //fprintf(stderr, "serverNonce = %s\n", serverNonce);
    //fprintf(stderr, "base64Salt = %s\n", base64Salt);
    //fprintf(stderr, "saltIterations = %d\n", atoi(saltIterations));
    //fprintf(stderr, "password = %s\n", service->password);
    
    //  While the nonce(s) is (are) already in ASCII encoding sans , and " characters,
    //  the salt is base64 encoded, so we have to decode it to UTF8
   // size_t saltLength = 0;
    salt = (char*)cr_base64_to_utf8(base64Salt, strlen(base64Salt), &saltLength );
   // assert( salt && saltLength > 0 );
	
    //  Run iterative hmac algorithm n times and concatenate the result in an XOR buffer in order to salt the password
    //      SaltedPassword  := Hi(Normalize(password), salt, i)
    //  Note:  Not really any way to tell if ca_scram_... fails
	memset(saltedPassword, 0, CC_SHA256_DIGEST_LENGTH);
    ca_scram_salt_password( service->password, strlen(service->password), salt, saltLength, atoi(saltIterations), saltedPassword );
    //fprintf(stderr, "salted password = %.*s\n", CC_SHA256_DIGEST_LENGTH, saltedPassword);
    
    //  Generate Client and Server Key buffers
    //  by encrypting the verbatim strings "Client Key" and "Server Key, respectively,
    //  using keyed HMAC SHA 256 with the salted password as the secret key
    //      ClientKey       := HMAC(SaltedPassword, "Client Key")
    //      ServerKey       := HMAC(SaltedPassword, "Server Key")
    ca_scram_hmac(saltedPassword, CC_SHA256_DIGEST_LENGTH, REQL_CLIENT_KEY, strlen(REQL_CLIENT_KEY), clientKey );
    ca_scram_hmac(saltedPassword, CC_SHA256_DIGEST_LENGTH, REQL_SERVER_KEY, strlen(REQL_SERVER_KEY), serverKey );
    //fprintf(stderr, "clientKey = %.*s\n", CC_SHA256_DIGEST_LENGTH, clientKey);
    //fprintf(stderr, "serverKey = %.*s\n", CC_SHA256_DIGEST_LENGTH, serverKey);
    
    //  Calculate the "Normal" SHA 256 Hash of the clientKey
    //      storedKey := H(clientKey)
    ca_scram_hash(clientKey, CC_SHA256_DIGEST_LENGTH, storedKey);
    //fprintf(stderr, "storedKey = %.*s\n", CC_SHA256_DIGEST_LENGTH, storedKey);
    
    //  Populate SCRAM client-final-message (ie channel binding and random nonce WITHOUT client proof)
    //  E.g.  "authentication": "c=biws,r=rOprNGfwEbeRWgbNEkqO%hvYDpWUa2RaTCAfuxFIlj)hNlF$k0,p=dHzbZapWIk4jUhN+Ute9ytag9zjfMHgsqmmiz7AndVQ="
    sprintf(CLIENT_FINAL_MESSAGE, "c=biws,r=%s", serverNonce);
    //fprintf(stderr, "CLIENT_FINAL_MESSAGE = \n\n%s\n\n", CLIENT_FINAL_MESSAGE);
    
    //Copy the client-final-message (without client proof) to the AuthMessage buffer
    strncat( SCRAM_AUTH_MESSAGE, CLIENT_FINAL_MESSAGE, strlen(CLIENT_FINAL_MESSAGE));
    AuthMessageLength += (unsigned long)strlen(CLIENT_FINAL_MESSAGE);
    //fprintf(stderr, "SCRAM_AUTH_MESSAGE = \n\n%s\n\n", SCRAM_AUTH_MESSAGE);
    
    //Now that we have the complete Auth Message we can use it in the SCRAM procedure
    //to calculate client and server signatures
    // ClientSignature := HMAC(StoredKey, AuthMessage)
    // ServerSignature := HMAC(ServerKey, AuthMessage)
    ca_scram_hmac(storedKey, CC_SHA256_DIGEST_LENGTH, SCRAM_AUTH_MESSAGE, AuthMessageLength, clientSignature );
    ca_scram_hmac(serverKey, CC_SHA256_DIGEST_LENGTH, SCRAM_AUTH_MESSAGE, AuthMessageLength, serverSignature );
    
    //And finally we calculate the client proof to put in the client-final message
    //ClientProof     := ClientKey XOR ClientSignature
    for(charIndex=0; charIndex<CC_SHA256_DIGEST_LENGTH; charIndex++)
        clientProof[charIndex] = (clientKey[charIndex] ^ clientSignature[charIndex]);
    
    //The Client Proof Bytes need to be Base64 encoded
    base64ProofLength = 0;
    base64Proof = cr_utf8_to_base64(clientProof, CC_SHA256_DIGEST_LENGTH, 0, &base64ProofLength );
    assert( base64Proof && base64ProofLength > 0 );
    
    //The Client Proof Bytes need to be Base64 encoded
    base64SSLength = 0;
    base64SS = cr_utf8_to_base64(serverSignature, CC_SHA256_DIGEST_LENGTH, 0, &base64SSLength );
    assert( base64SS && base64SSLength > 0 );
    //fprintf(stderr, "Base64 Server Signature:  %s\n", base64SS);

    //Add the client proof to the end of the client-final-message
    sprintf(CLIENT_FINAL_MESSAGE_JSON, "{\"authentication\":\"%s,p=%.*s\"}", CLIENT_FINAL_MESSAGE, (int)base64ProofLength, (char*)base64Proof);
    //fprintf(stderr, "CLIENT_FINAL_MESSAGE_JSON = \n\n%s", CLIENT_FINAL_MESSAGE_JSON);
    
    //Send the client-final-message wrapped in json
	//CLIENT_FINAL_MESSAGE_JSON[strlen(CLIENT_FINAL_MESSAGE_JSON)] = '\0';
    CTSend(r, CLIENT_FINAL_MESSAGE_JSON, strlen(CLIENT_FINAL_MESSAGE_JSON)+1);  //Note:  JSON always needs the extra null character to determine EOF!!!
    
    //yield();

    //Wait for SERVER_FINAL_MESSAGE
    readLength = 1024;
    sFinalMessagePtr = (char*)CTRecv(r, SERVER_FINAL_MESSAGE_JSON, &readLength);
    fprintf(stderr, "SERVER_FINAL_MESSAGE_JSON = \n\n%s\n", sFinalMessagePtr);
    
    //Validate Server Signature
    authLength = 0;
    if( (authValue = strstr(sFinalMessagePtr, REQL_AUTH_KEY)) != NULL )
    {
        authValue += strlen(REQL_AUTH_KEY) + 1;
        //if( *successVal == ':') successVal += 1;
        if( *authValue == '"')
        {
            authValue+=1;
            tmp = (char*)authValue;
            while( *(tmp) != '"' ) tmp++;
            authLength = (unsigned long)(tmp - authValue);
        }
        
        if( *authValue == 'v' && *(authValue+1) == '=')
        {
            authValue += 2;
            authLength -= 2;
        }
        else
            authValue = NULL;
    }
    else//if( !authValue )
    {
        fprintf(stderr, "Failed to retrieve server signature from SCRAM server-final-message.\n");
        return CTSASLHandshakeError;
    }
    
    //  Compare server sent signature to client generated server signature
    if( strncmp((char*)base64SS,  authValue, authLength) != 0)
    {
        fprintf(stderr, "Failed to authenticate server signature: %s\n", authValue);
        return CTSASLHandshakeError;
    }
    
    //free Base64 <-> UTF8 string conversion allocations
    free( salt );
    free( base64Proof );
    free( base64SS );
    
    return CTSuccess;
}


char* ReQLHandshakeHeaderLengthCallback(struct CTCursor* cursor, char* buffer, unsigned long length)
{
	fprintf(stderr, "ReQLHandshakeHeaderLengthCallback...\n\n");

	//the purpose of this function is to return the end of the header in the 
	//connection's tcp stream to CoreTransport decrypt/recv thread
	char* endOfHeader = buffer;// +sizeof(ReqlQueryMessageHeader);

	//The client *must* set the cursor's contentLength property
	//to aid CoreTransport in knowing when to stop reading from the socket

	//if (cursor->contentLength == 0)
	cursor->contentLength = length;// 5 + 88 + 5 + 4583 + 5 + 333 + 5 + 4;// length;// ((ReqlQueryMessageHeader*)buffer)->length;

//The cursor headerLength is calculated as follows after this function returns
	return endOfHeader;
}


void ReQLMagicNumberResponseCallback(CTError* err, CTCursor* cursor)
{
	fprintf(stderr, "ReQLFirstMessageResponseCallback header:  \n\n%.*s\n\n", cursor->headerLength, cursor->requestBuffer);

	//TO DO:  parse any existing errors first
	struct CTError error = { (CTErrorClass)0,0,0 };    //Reql API client functions will generally return ints as errors

	//if ((ret = CTSSLHandshakeProcessFirstResponse(cursor, cursor->conn->socket, cursor->conn->sslContext)) != noErr)
	if( error.id = CTReQLHandshakeProcessMagicNumberResponse(cursor->requestBuffer, strlen(cursor->requestBuffer)) != noErr )
	{
#ifdef CTRANSPORT_WOLFSSL
		if (ret == SSL_ERROR_WANT_READ)
		{
			assert(1 == 0);
			return CTSuccess;
		}
		else if (ret == SSL_ERROR_WANT_WRITE)
		{
			assert(1 == 0);
			return CTSuccess;
		}
#endif
		//ReqlSSLCertificateDestroy(&rootCertRef);
		fprintf(stderr, "CTReQLHandshakeProcessMagicNumberResponse failed with status:  %d\n", error.id);
		error.id = CTSASLHandshakeError;
		//CTCloseSSLSocket(cursor->conn->sslContext, cursor->conn->socket);
		cursor->conn->target->callback(&error, cursor->conn);
	}
}


void ReQLFinalMessageResponseCallback(CTError* err, CTCursor* cursor)
{
	fprintf(stderr, "ReQLSecondMessageResponseCallback header:  \n\n%.*s\n\n", cursor->headerLength, cursor->requestBuffer);
	
	//TO DO:  parse any existing errors first
	struct CTError error = { (CTErrorClass)0,0,0 };    //Reql API client functions will generally return ints as errors

	char* base64SSPtr = NULL;//
	memcpy(&base64SSPtr, &(cursor->requestBuffer[65536 - sizeof(char*)]), sizeof(char*));
	if ((error.id = CTReQLHandshakeProcessFinalMessageResponse(cursor->requestBuffer, strlen(cursor->requestBuffer), base64SSPtr)) != CTSuccess)
	{
#ifdef CTRANSPORT_WOLFSSL
		if (error.id == SSL_ERROR_WANT_READ)
		{
			assert(1 == 0);
			return CTSuccess;
		}
		else if (error.id == SSL_ERROR_WANT_WRITE)
		{
			assert(1 == 0);
			return CTSuccess;
		}
#endif
		//ReqlSSLCertificateDestroy(&rootCertRef);
		fprintf(stderr, "CTReQLHandshakeProcessFinalMessageResponse failed with status:  %d\n", error.id);
		error.id = CTSASLHandshakeError;
		//CTCloseSSLSocket(cursor->conn->sslContext, cursor->conn->socket);
	}

	cursor->conn->responseCount = 0;  //we incremented this for the handshake, it is critical to reset this for the client before returning the connection
	cursor->conn->queryCount = 0;
	CT_CURSOR_INDEX = 0;
	cursor->conn->target->callback(&error, cursor->conn);
}

void ReQLFirstMessageResponseCallback(CTError* err, CTCursor* cursor)
{
	fprintf(stderr, "ReQLFirstMessageResponseCallback header:  \n\n%.*s\n\n", cursor->headerLength, cursor->requestBuffer);
	
	//TO DO:  parse any existing errors first
	struct CTError error = { (CTErrorClass)0,0,0 };    //Reql API client functions will generally return ints as errors

	//typedef union BytePtrUnion
	//{

	//}

	char* base64SSPtr = NULL;// &(cursor->requestBuffer[65536 - sizeof(void*)]);
	//if ((ret = CTSSLHandshakeProcessFirstResponse(cursor, cursor->conn->socket, cursor->conn->sslContext)) != noErr)
	if (( base64SSPtr = CTReQLHandshakeProcessFirstMessageResponse(cursor->requestBuffer, strlen(cursor->requestBuffer), cursor->conn->service->password)) == NULL)
	{
#ifdef CTRANSPORT_WOLFSSL
		if (ret == SSL_ERROR_WANT_READ)
		{
			assert(1 == 0);
			return CTSuccess;
		}
		else if (ret == SSL_ERROR_WANT_WRITE)
		{
			assert(1 == 0);
			return CTSuccess;
		}
#endif
		//ReqlSSLCertificateDestroy(&rootCertRef);
		fprintf(stderr, "CTReQLHandshakeProcessFirstMessageResponse failed\n");
		error.id = CTSASLHandshakeError;
		//CTCloseSSLSocket(cursor->conn->sslContext, cursor->conn->socket);
		cursor->conn->target->callback(&error, cursor->conn);
		return;
	}

	//copy the base64 address to storage for use by next async handshake message
	memcpy(&(cursor->requestBuffer[65536 - sizeof(char*)]), &base64SSPtr, sizeof(char*));

	cursor->headerLengthCallback = ReQLHandshakeHeaderLengthCallback; //we'll use same headerLenghtCallback for all ReQLHandshake messages in the sequence
	cursor->responseCallback = ReQLFinalMessageResponseCallback;	   //the response itself will be specific to processing the actual message at each stage

	cursor->overlappedResponse.buf = cursor->requestBuffer;
	cursor->overlappedResponse.len = strlen(cursor->requestBuffer) + 1;
	cursor->overlappedResponse.stage = CT_OVERLAPPED_SCHEDULE; //we need to bypass encrypt bc sthis is the ssl handshake
	//assert(->->overlappedResponse.buf);
	CTCursorSendOnQueue(cursor, (char**)&(cursor->overlappedResponse.buf), cursor->overlappedResponse.len);
}

//CTRANSPORT_ALIGN(8) char MAGIC_NUMBER_RESPONSE_JSON[256];
//CTRANSPORT_ALIGN(8) char SERVER_FIRST_MESSAGE_JSON[256];
//CTRANSPORT_ALIGN(8) char SERVER_FINAL_MESSAGE_JSON[256];

int CTReQLAsyncHandshake(CTConnection* conn, CTTarget* service, CTConnectionClosure callback)
{
	OSStatus status;

	//SCRAM Message Exchange Buffer Declarations
	//  AuthMessage := client-first-message-bare + "," + server-first-message + "," + client-final-message-without-proof

	//All buffers provided to ReqlSend must be in-place writeable
	CTRANSPORT_ALIGN(8) char clientNonce[CT_REQL_NUM_NONCE_BYTES];                //a buffer to hold randomly generated UTF8 characters
	CTRANSPORT_ALIGN(8) char REQL_MAGIC_NUMBER_BUF[4] = { '\xc3', '\xbd', '\xc2', '\x34' };
	CTRANSPORT_ALIGN(8) char CLIENT_FIRST_MESSAGE_JSON[256] = "\0";
	//CTRANSPORT_ALIGN(8) char SCRAM_AUTH_MESSAGE[1024] = "\0";

	//char* base64SS = NULL;
	unsigned long AuthMessageLength = 0;/// = 0;

	CTCursor* handshakeCursor = CTGetNextPoolCursor();
	CTCursor* firstMsgCursor = CTGetNextPoolCursor();
	memset(handshakeCursor, 0, sizeof(CTCursor));
	memset(firstMsgCursor, 0, sizeof(CTCursor));

	char* SCRAM_AUTH_MESSAGE = firstMsgCursor->requestBuffer + 256;
	SCRAM_AUTH_MESSAGE[0] = '\0';
	fprintf(stderr, "CTReQLAsyncHandshake begin\n\n");
	//  Generate a random client nonce for sasl scram sha 256 required by RethinkDB 2.3
	//  for populating the SCRAM client-first-message
	//  We must wrap SecRandomCopyBytes in a function and use it one byte at a time
	//  to ensure we get a valid UTF8 String (so this is pretty slow)
	if ((status = ca_scram_gen_rand_bytes(clientNonce, CT_REQL_NUM_NONCE_BYTES)) != noErr)
	{
		fprintf(stderr, "ca_scram_gen_rand_bytes failed with status:  %d\n", status);
		struct CTError ctError = { (CTErrorClass)CTDriverErrorClass, CTSCRAMEncryptionError, NULL };    //Reql API client functions will generally return ints as errors
		callback(&ctError, conn);
		return CTSCRAMEncryptionError;
	}

	conn->service->callback = callback;

	//  Populate SCRAM client-first-message (ie client-first-message containing username and random nonce)
	//  We choose to forgo creating a dedicated buffer for the client-first-message and populate
	//  directly into the SCRAM AuthMessage Accumulation buffer
	sprintf(SCRAM_AUTH_MESSAGE, "n=%s,r=%.*s", service->user, (int)CT_REQL_NUM_NONCE_BYTES, clientNonce);
	AuthMessageLength += (unsigned long)strlen(SCRAM_AUTH_MESSAGE);
	//fprintf(stderr, "CLIENT_FIRST_MESSAGE = \n\n%s\n\n", SCRAM_AUTH_MESSAGE);

	//  Populate client-first-message in JSON wrapper expected by RethinkDB ReQL Service
	//static const char *JSON_PROTOCOL_VERSION = "{\"protocol_version\":0,\0";
	//static const char *JSON_AUTH_METHOD = "\"authentication_method\":\"SCRAM-SHA-256\",\0";
	strcat(CLIENT_FIRST_MESSAGE_JSON, REQL_JSON_PROTOCOL_VERSION);//, strlen(JSON_PROTOCOL_VERSION));
	strcat(CLIENT_FIRST_MESSAGE_JSON, REQL_JSON_AUTH_METHOD);//, strlen(JSON_PROTOCOL_VERSION));
	sprintf(CLIENT_FIRST_MESSAGE_JSON + strlen(CLIENT_FIRST_MESSAGE_JSON), "\"authentication\":\"n,,%s\"}", SCRAM_AUTH_MESSAGE);
	fprintf(stderr, "CLIENT_FIRST_MESSAGE_JSON = \n\n%s\n\n", CLIENT_FIRST_MESSAGE_JSON);

	//  The client-first-message is already in the AuthMessage buffer
	//  So we don't need to explicitly copy it
	//  But we do need to add a comma after the client-first-message
	//  and a null char to prepare for appending the next SCRAM messages
	SCRAM_AUTH_MESSAGE[AuthMessageLength] = ',';
	AuthMessageLength += 1;
	SCRAM_AUTH_MESSAGE[AuthMessageLength] = '\0';

	//memset(MAGIC_NUMBER_RESPONSE_JSON, 0, 256);
	//memset(SERVER_FIRST_MESSAGE_JSON, 0, 256);
	//memset(SERVER_FINAL_MESSAGE_JSON, 0, 256);


	//send the TLS Handshake first message in an async non-blocking fashion
	handshakeCursor->headerLengthCallback = ReQLHandshakeHeaderLengthCallback; //we'll use same headerLenghtCallback for all ReQLHandshake messages in the sequence
	handshakeCursor->responseCallback = ReQLMagicNumberResponseCallback;	   //the response itself will be specific to processing the actual message at each stage

	firstMsgCursor->headerLengthCallback = ReQLHandshakeHeaderLengthCallback; //we'll use same headerLenghtCallback for all ReQLHandshake messages in the sequence
	firstMsgCursor->responseCallback = ReQLFirstMessageResponseCallback;	   //the response itself will be specific to processing the actual message at each stage

																			   //copy the magic number to the handshake cursor request buffer
	memcpy(handshakeCursor->requestBuffer, REQL_MAGIC_NUMBER_BUF, 4);
	memcpy(firstMsgCursor->requestBuffer, CLIENT_FIRST_MESSAGE_JSON, strlen(CLIENT_FIRST_MESSAGE_JSON) + 1);

	handshakeCursor->file.buffer = handshakeCursor->requestBuffer;
	handshakeCursor->overlappedResponse.buf = (char*)(handshakeCursor->file.buffer);
	handshakeCursor->overlappedResponse.len = 4;

	firstMsgCursor->file.buffer = firstMsgCursor->requestBuffer;
	firstMsgCursor->overlappedResponse.buf = (char*)(firstMsgCursor->file.buffer);
	firstMsgCursor->overlappedResponse.len = strlen(CLIENT_FIRST_MESSAGE_JSON) + 1;

	handshakeCursor->conn = conn; //assume conn is permanent memory from core transport connection pool
	handshakeCursor->overlappedResponse.stage = CT_OVERLAPPED_SCHEDULE; //we need to bypass encrypt bc sthis is the ssl handshake
	assert(handshakeCursor->overlappedResponse.buf);
	CTCursorSendOnQueue(handshakeCursor, (char**)&(handshakeCursor->overlappedResponse.buf), handshakeCursor->overlappedResponse.len);

	firstMsgCursor->conn = conn; //assume conn is permanent memory from core transport connection pool
	firstMsgCursor->overlappedResponse.stage = CT_OVERLAPPED_SCHEDULE; //we need to bypass encrypt bc sthis is the ssl handshake
	firstMsgCursor->queryToken = 1;
	assert(firstMsgCursor->overlappedResponse.buf);
	CTCursorSendOnQueue(firstMsgCursor, (char**)&(firstMsgCursor->overlappedResponse.buf), firstMsgCursor->overlappedResponse.len);
	
	return CTSuccess;
}

coroutine int CTSSLRoutineSendFirstMessage(CTConnection* conn, char* hostname, char* caPath)
{
	int ret;

	//OSStatus status;
	//Note:  PCredHandle (PSecHandle) is the WIN32 equivalent of SecCertificateRef
	CTSecCertificateRef rootCertRef = NULL;

	// Create credentials.

	//  ReqSSLCertificate (Obtains a native Security Certificate object/handle) 
	//
	//  MBEDTLS NOTES:
	//
	//  WIN32 NOTES:
	//		ReqlSecCertificateRef is of type PCredHandle on WIN32, but it needs to point to allocated memory before supplying as input to AcquireCredentialHandle (ACH)
	//		so ReqlSSLCertificate will allocate memory for this pointer and return it to the client, but the client is responsible for freeing it later 
	//
	//  DARWIN NOTES:
	//
	//		Read CA file in DER format from disk so we can use it/add it to the trust chain during the SSL Handshake Authentication
	//		-- Non (password protected) PEM files can be converted to DER format offline with OpenSSL or in app with by removing PEM headers/footers and converting (from or to?) BASE64.  
	//		   DER data may need headers chopped of in some instances.
	//		-- Password protected PEM files must have their private/public key pairs generated using PCK12.  This can be done offline with OpenSSL or in app using Secure Transport
	//

#ifdef CTRANSPORT_WOLFSSL
	rootCertRef = caPath;
#elif defined(CTRANSPORT_USE_MBED_TLS) || defined(__APPLE__)
	if (caPath)
	{
		if ((rootCertRef = CTSSLCertificate(caPath)) == NULL)
		{
			fprintf(stderr, "ReqlSSLCertificate failed to generate SecCertificateRef\n");
			CTCloseSSLSocket(conn->sslContext, conn->socket);
			return CTSSLCertificateError;
		}
	}
#endif
	//ReqlSSLInit();

	fprintf(stderr, "----- Credentials Initialized\n");

	//ReqlSSLContextRef sslContext = NULL;
	//  Create an SSL/TLS Context using a 3rd Party Framework such as Apple SecureTransport, OpenSSL or mbedTLS
	//  THEN send the first message in a synchronous fashion

	void* sslFirstMessageBuffer = NULL;
	int sslFirstMessageLen = 0;
	if ((conn->sslContext = CTSSLContextCreate(&(conn->socket), &rootCertRef, hostname, &sslFirstMessageBuffer, sslFirstMessageLen)) == NULL)
	{
		//ReqlSSLCertificateDestroy(&rootCertRef);
		return CTSecureTransportError;
	}
	fprintf(stderr, "----- SSL Context Initialized\n");


	CTSSLHandshakeSendFirstMessage(conn->socket, conn->sslContext, sslFirstMessageBuffer, sslFirstMessageLen);

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);     // Initialize the structure.
	fprintf(stderr, TEXT("CTPageSize() = %d.\n"), systemInfo.dwPageSize);
	return (unsigned long)systemInfo.dwPageSize;

}


char* SSLFirstMessageHeaderLengthCallback(struct CTCursor* cursor, char* buffer, unsigned long length)
{
	fprintf(stderr, "SSLFirstMessageHeaderLengthCallback...\n\n");

	//the purpose of this function is to return the end of the header in the 
	//connection's tcp stream to CoreTransport decrypt/recv thread
	char* endOfHeader = buffer;// +sizeof(ReqlQueryMessageHeader);

	//The client *must* set the cursor's contentLength property
	//to aid CoreTransport in knowing when to stop reading from the socket

	//if (cursor->contentLength == 0)
		cursor->contentLength = length;// 5 + 88 + 5 + 4583 + 5 + 333 + 5 + 4;// length;// ((ReqlQueryMessageHeader*)buffer)->length;

	//The cursor headerLength is calculated as follows after this function returns
	return endOfHeader;
}



void SSLFirstMessageResponseCallback(CTError* err, CTCursor* cursor)
{
	fprintf(stderr, "SSLFirstMessageResponseCallback header:  \n\n%.*s\n\n", cursor->headerLength, cursor->requestBuffer);
	int ret = 0;
	if ((ret = CTSSLHandshakeProcessFirstResponse(cursor, cursor->conn->socket, cursor->conn->sslContext)) != noErr)
	{
#ifdef CTRANSPORT_WOLFSSL
		if (ret == SSL_ERROR_WANT_READ)
		{
			assert(1 == 0);
			return CTSuccess;
		}
		else if (ret == SSL_ERROR_WANT_WRITE)
		{
			assert(1 == 0);
			return CTSuccess;
		}
#endif
		//ReqlSSLCertificateDestroy(&rootCertRef);
		fprintf(stderr, "CTSSLHandshake with status:  %d\n", ret);
		err->id = CTSSLHandshakeError;
		CTCloseSSLSocket(cursor->conn->sslContext, cursor->conn->socket);
		cursor->conn->target->callback(err, cursor->conn);
	}
}

void SSLFirstMessageQueryCallback(CTError* err, CTCursor* cursor)
{
	int ret = 0;
	int wolfErr = 0;
	char buffer[80];
	CTClientError scRet;

	fprintf(stderr, "SSLFirstMessageQueryCallback len:  \n\n%.*s\n\n", cursor->headerLength, cursor->requestBuffer);

#ifdef CTRANSPORT_WOLFSSL

	CTSSLDecryptTransient dTransient = { 0, 0, 0, cursor->file.buffer };
	CTSSLEncryptTransient eTransient = { 0, cursor->overlappedResponse.len, NULL };

	fprintf(stderr, "SSLFirstMessageQueryCallback::CoreTransport supplying (%lu) handshake bytes to WolfSSL...\n", cursor->headerLength + cursor->contentLength);

	//we are finished sending the first response, reset the send context for the second client message after the recv

	//Let WolfSSL Decrypt as many bytes from our encrypted input buffer ass possible in a single pass to wolfSSL_read
	wolfSSL_SetIOWriteCtx(cursor->conn->sslContext->ctx, &eTransient);
	wolfSSL_SetIOReadCtx(cursor->conn->sslContext->ctx, &dTransient);


	/* Connect to wolfSSL on the server side */
	while ((ret = wolfSSL_connect(cursor->conn->sslContext->ctx)) != SSL_SUCCESS) {

		wolfErr = wolfSSL_get_error(cursor->conn->sslContext->ctx, 0);
		wolfSSL_ERR_error_string(wolfErr, buffer);
		ffprintf(stderr, stderr, "SSLFirstMessageQueryCallback: wolfSSL_connect failed to send/read handshake data (%d): \n\n%s\n\n", wolfSSL_get_error(cursor->conn->sslContext->ctx, 0), buffer);

		//HANDLE INCOMPLETE MESSAGE: the buffer didn't have as many bytes as WolfSSL needed for decryption
		if (wolfErr == WOLFSSL_ERROR_WANT_READ)
		{
			//assert(1 == 0);
			//eTransient = (CTSSLEncryptTransient*)wolfSSL_GetIOWriteCtx(cursor->conn->sslContext->ctx);
			//eTransient->ct_socket_fd = cursor->conn->socket;
			//wolfSSL_SetIOWriteCtx(cursor->conn->sslContext->ctx, &(cursor->conn->socket));
			return CTSuccess;
		}
		else if (wolfErr == SSL_ERROR_WANT_WRITE)
		{
			//wolfSSL_SetIOWriteCtx(cursor->conn->sslContext->ctx, &eTransient);
			//assert(1 == 0);

			//eTransient = (CTSSLEncryptTransient*)wolfSSL_GetIOWriteCtx(cursor->conn->sslContext->ctx);
			//eTransient->wolf_socket_fd = cursor->conn->socket;
			//eTransient->ct_socket_fd = 0;
			/*
			assert(eTransient && eTransient->messageLen == 6);
			//the second client handshake message
			cursor->headerLengthCallback = SSLSecondMessageHeaderLengthCallback;
			cursor->responseCallback = SSLSecondMessageResponseCallback;
			cursor->queryCallback = SSLSecondMessageQueryCallback;
			cursor->contentLength = cursor->headerLength = 0;
			cursor->conn->responseCount = 0;
			cursor->file.buffer = cursor->requestBuffer;
			cursor->overlappedResponse.buf = eTransient->messageBuffer;// (char*)(OutBuffers[0].pvBuffer);
			cursor->overlappedResponse.len = eTransient->messageLen;//(char*)(OutBuffers[0].cbBuffer);
			cursor->overlappedResponse.stage = CT_OVERLAPPED_SEND; //we need to send but bypass encrypt bc this is the ssl handshake
			assert(cursor->overlappedResponse.buf);
			CTCursorSendOnQueue(cursor, (char**)&(cursor->overlappedResponse.buf), cursor->overlappedResponse.len);
			return CTSuccess;
			*/
			//continue;

		}
		/*
		err = wolfSSL_get_error(sslContextRef->ctx, ret);
		if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE)
		{
			ffprintf(stderr, stderr, "ERROR: wolfSSL handshake failed!\n");
		}
		return err;
		*/
	}
	//wolf_handshake_complete = 1;
#endif(_WIN32)
}


typedef struct CTSocks4ConnectMessage {
	uint8_t version;// = 0x04; //SOCKS version 4 (obviously)
	uint8_t command;// = 0x01; //1 is TCP CONNECT command
	uint16_t port; //16 bit/2 byte port (network order)
	uint32_t ip; //32 bit/4 byte IP address (network order)
	uint32_t userid;
	uint8_t nullbyte;// = 0x00; //null byte sent at message end
	//uint8_t nullbyte2;
}CTSocks4ConnectMessage;


typedef struct CTSocks4ResponseMessage {
	uint8_t version;// = 0x04; //SOCKS version 4 (obviously)
	uint8_t command;// = 0x01; //1 is TCP CONNECT command
	uint16_t port; //16 bit/2 byte port (network order)
	uint32_t ip; //32 bit/4 byte IP address (network order)
}CTSocks4ResponseMessage;

/***/

typedef struct CTSocks5ClientGreeting
{
	uint8_t VER;
	uint8_t NAUTH;
	uint8_t AUTH;
}CTSocks5ClientGreeting;

typedef struct CTSocks5ServerChoice
{
	uint8_t VER;
	uint8_t CAUTH;
}CTSocks5ServerChoice;

typedef struct CTSocks5ClientAuth
{
	uint8_t VER;
	uint8_t IDLEN;
	uint8_t ID;
	uint8_t PWLEN;
	uint8_t PW;
}CTSocks5ClientAuth;

typedef struct CTSocks5ServerAuth
{
	uint8_t VER;
	uint8_t STATUS;
}CTSocks5ServerAuth;

typedef struct CTSocks5ClientConnect
{
	uint8_t     VER;
	uint8_t     CMD;
	uint8_t     RSV;
	uint8_t     TYPE;
	//uint8_t*  ADDR;
	uint16_t    PORT;

}CTSocks5ClientConnect;

typedef struct CTSocks5ServerConnect
{
	uint8_t     VER;
	uint8_t     STATUS;
	uint8_t     RSV;
	uint8_t     TYPE;
	//uint8_t*  BINDADDR;
	//uint16_t	BINDPORT;
}CTSocks5ServerConnect;


char* HTTPProxyHeaderLengthCallback(struct CTCursor* cursor, char* buffer, unsigned long length)
{
	fprintf(stderr, "HTTPProxyHeaderLengthCallback...\n\n");

	char* endOfHeader = strstr(buffer, "\r\n\r\n");

	if (!endOfHeader)
		return NULL;// assert(1 == 0);

	//set ptr to end of http header
	endOfHeader += sizeof("\r\n\r\n") - 1;

	//calculate size of header
	cursor->headerLength = endOfHeader - buffer;
	
	cursor->contentLength = 0;
	return endOfHeader;

}


void HTTPProxyResponseCallback(CTError* err, CTCursor* cursor)
{
	fprintf(stderr, "ProxyHandshakeMessageResponseCallback header:  \n\n%.*s\n\n", cursor->headerLength, cursor->requestBuffer);
	int status = 0;


	if (0) //((ret = CTSSLHandshakeProcessFirstResponse(cursor, cursor->conn->socket, cursor->conn->sslContext)) != noErr)
	{

#ifdef CTRANSPORT_WOLFSSL
		if (ret == SSL_ERROR_WANT_READ)
		{
			assert(1 == 0);
			return CTSuccess;
		}
		else if (ret == SSL_ERROR_WANT_WRITE)
		{
			assert(1 == 0);
			return CTSuccess;
		}
#endif
		//ReqlSSLCertificateDestroy(&rootCertRef);
		fprintf(stderr, "CTProxyHandshake failed with status:  %d\n", status);
		err->id = CTProxyHandshakeError;
		CTCloseSSLSocket(cursor->conn->sslContext, cursor->conn->socket);
		cursor->conn->target->callback(err, cursor->conn);
		return;
	}
	else if (cursor->conn->target->ssl.method > CTSSL_NONE)
	{
		//Initate the Async SSL Handshake scheduling from this thread (ie cxQueue) to rxQueue and txQueue threads
		if ((status = CTSSLRoutine(cursor->conn, cursor->conn->socketContext.host, cursor->conn->target->ssl.ca)) != 0)
		{
			fprintf(stderr, "CTConnection::CTConnectThread::CTSSLRoutine failed with error: %d\n", (int)status);
			err->id = CTSSLHandshakeError;
			CTCloseSSLSocket(cursor->conn->sslContext, cursor->conn->socket);
			cursor->conn->target->callback(err, cursor->conn);
		}

		return;
	}

	err->id = CTSuccess;
	cursor->conn->responseCount = 0;  //we incremented this for the handshake, it is critical to reset this for the client before returning the connection
	cursor->conn->queryCount = 0;
	CT_CURSOR_INDEX = 0;
	cursor->conn->target->callback(err, cursor->conn);

}


char* Socks5ProxyHeaderLengthCallback(struct CTCursor* cursor, char* buffer, unsigned long length)
{
	fprintf(stderr, "Socks5ProxyHeaderLengthCallback...\n\n");
	cursor->contentLength = 2;
	return buffer; 
}

char* Socks5ClientConnectHeaderLengthCallback(struct CTCursor* cursor, char* buffer, unsigned long length)
{
	fprintf(stderr, "Socks5ClientConnectHeaderLengthCallback...\n\n");
	
	CTSocks5ServerConnect serverConnect = { 0 };

	uint8_t hostLen = 0;
	uint8_t hostOffset = 4;

	uint16_t hostPort = 0;
	memcpy(&serverConnect, buffer, hostOffset);

	if (serverConnect.TYPE == 0x01) //IPv4
	{
		hostLen = 4;

		//put the 4 ipv4 network bytes into an IN_ADDR struct for inet_ntop
		struct sockaddr_storage ip_addr = { 0 };
		struct sockaddr_in* ipv4_addr = (struct sockaddr_in*)&ip_addr;

		socklen_t addrlen = sizeof(*ipv4_addr);
		memset(ipv4_addr, 0, addrlen);
		ipv4_addr->sin_family = AF_INET;
		ipv4_addr->sin_port = 0x00;// htons(port);
		memcpy(&(ipv4_addr->sin_addr), buffer + 4, hostLen);
		

		char IP_ADDR_BUFFER[256] = "\0";
		//uint32_t ip = 0;
		//memcpy(&ip, buffer + 4, hostLen);

		assert( inet_ntop(AF_INET, &(ipv4_addr->sin_addr), IP_ADDR_BUFFER, 256) );
		fprintf(stderr, "SERVER BINDADDR (IPv4) = %.*s\n", strlen(IP_ADDR_BUFFER), IP_ADDR_BUFFER);
	}
	else if (serverConnect.TYPE == 0x03) //DOMAINS SHOULD BE RESOLVED
	{
		//assert(1 == 0);
		memcpy(&hostLen, buffer + hostOffset, 1);
		hostOffset++;
		fprintf(stderr, "SERVER BINDADDR (DOMAIN) = %.*s\n", (int)hostLen, buffer + hostOffset);
	}
	else if (serverConnect.TYPE == 0x04) //IPv6
	{
		hostLen = 16;
	}
	
	memcpy(&hostPort, buffer + 4 + hostLen, 2);
	hostPort = ntohs(hostPort);

	fprintf(stderr, "SERVER BINDPORT = %hu\n\n", hostPort);
	
	cursor->contentLength = hostOffset + hostLen + 2;
	return buffer;
}


void Socks5ClientConnectResponseCallback(CTError* err, CTCursor* cursor)
{
	fprintf(stderr, "Socks5ClientConnectResponseCallback response:  \n\n%.*s\n\n", cursor->contentLength, cursor->file.buffer);
	int status = 0;


	//CTSocks4ResponseMessage responseMessage = { 0 };
	//assert(sizeof(CTSocks4ResponseMessage) == 8);
	//assert(cursor->contentLength == sizeof(CTSocks4ResponseMessage));	
	//memcpy(&responseMessage, cursor->file.buffer, sizeof(CTSocks4ResponseMessage));

	CTSocks5ServerConnect serverConnectResult = { cursor->file.buffer[0], cursor->file.buffer[1], cursor->file.buffer[3] };
	assert(serverConnectResult.VER == 0x05);
	assert(serverConnectResult.STATUS == 0x00);
	fprintf(stderr, "SOCKS5 SERVER CONNECT RESULT = %x\n\n", serverConnectResult.STATUS);

	if (cursor->conn->target->ssl.method > CTSSL_NONE)
	{
		//Initate the Async SSL Handshake scheduling from this thread (ie cxQueue) to rxQueue and txQueue threads
		if ((status = CTSSLRoutine(cursor->conn, cursor->conn->socketContext.host, cursor->conn->target->ssl.ca)) != 0)
		{
			fprintf(stderr, "CTConnection::CTConnectThread::CTSSLRoutine failed with error: %d\n", (int)status);
			err->id = CTSSLHandshakeError;
			CTCloseSSLSocket(cursor->conn->sslContext, cursor->conn->socket);
			cursor->conn->target->callback(err, cursor->conn);
		}

		return;
	}

	err->id = CTSuccess;
	cursor->conn->responseCount = 0;  //we incremented this for the handshake, it is critical to reset this for the client before returning the connection
	cursor->conn->queryCount = 0;
	CT_CURSOR_INDEX = 0;
	cursor->conn->target->callback(err, cursor->conn);

}

void Socks5ClientAuthResponseCallback(CTError* err, CTCursor* cursor)
{
	fprintf(stderr, "Socks5ProxyClientAuthResponseCallback response:  \n\n%.*s\n\n", cursor->contentLength, cursor->file.buffer);
	int status = 0;


	//CTSocks4ResponseMessage responseMessage = { 0 };
	//assert(sizeof(CTSocks4ResponseMessage) == 8);
	//assert(cursor->contentLength == sizeof(CTSocks4ResponseMessage));	
	//memcpy(&responseMessage, cursor->file.buffer, sizeof(CTSocks4ResponseMessage));

	CTSocks5ServerAuth serverAuthResult = { cursor->file.buffer[0], cursor->file.buffer[1] };
	assert(serverAuthResult.VER == 0x01);
	assert(serverAuthResult.STATUS == 0x00);
	fprintf(stderr, "SOCKS5 SERVER AUTH RESULT = %x\n\n", serverAuthResult.STATUS);


	CTSocks5ClientConnect clientConnect = { 0x05, 0x01, 0x00, 0x03, cursor->conn->target->url.port };
	clientConnect.PORT = htons(clientConnect.PORT);

	uint8_t hostLen = strlen(cursor->conn->target->url.host);

	cursor->headerLength = 0;
	cursor->contentLength = 0;

	cursor->headerLengthCallback = Socks5ClientConnectHeaderLengthCallback;
	cursor->responseCallback = Socks5ClientConnectResponseCallback;
	//cursor->queryCallback = SSLSecondMessageQueryCallback;

	memcpy(cursor->requestBuffer, &clientConnect, 4);
	memcpy(cursor->requestBuffer+4, &hostLen, 1);
	memcpy(cursor->requestBuffer + 5, cursor->conn->target->url.host, hostLen);
	memcpy(cursor->requestBuffer + 5 + hostLen, &(clientConnect.PORT), 2);
	cursor->requestBuffer[5 + hostLen + 2] = '\0';

	cursor->file.buffer = cursor->requestBuffer;
	cursor->overlappedResponse.buf = (char*)(cursor->requestBuffer);
	cursor->overlappedResponse.len = 5 + hostLen + 2;// sizeof(CTSocks4ConnectMessage);// strlen(CONNECT_REQUEST);
	cursor->overlappedResponse.stage = CT_OVERLAPPED_SEND; //we need to send but bypass encrypt bc this is the ssl handshake
	assert(cursor->overlappedResponse.buf);
	CTCursorSendOnQueue(cursor, (char**)&(cursor->overlappedResponse.buf), cursor->overlappedResponse.len);


}

void Socks5ClientGreetingReponseCallback(CTError* err, CTCursor* cursor)
{

	fprintf(stderr, "Socks5ClientGreetingReponseCallback response:  \n\n%.*s\n\n", cursor->contentLength, cursor->file.buffer);

	//CTSocks4ResponseMessage responseMessage = { 0 };
	//assert(sizeof(CTSocks4ResponseMessage) == 8);
	//assert(cursor->contentLength == sizeof(CTSocks4ResponseMessage));	
	//memcpy(&responseMessage, cursor->file.buffer, sizeof(CTSocks4ResponseMessage));

	CTSocks5ServerChoice serverAuthChoice = { cursor->file.buffer[0], cursor->file.buffer[1] };
	assert(serverAuthChoice.VER == 0x05);
	fprintf(stderr, "SOCKS5 SERVER AUTH CHOICE = %x\n\n", serverAuthChoice.CAUTH);


	/*
	CTSocks5ClientAuth clientAuth = { 0x01, 0x01, 0x02 };

	//reset cursor properties
	cursor->headerLength = 0;
	cursor->contentLength = 0;

	cursor->headerLengthCallback = Socks5ProxyHeaderLengthCallback;
	cursor->responseCallback = Socks5ClientAuthResponseCallback;
	//cursor->queryCallback = SSLSecondMessageQueryCallback;

	memcpy(cursor->requestBuffer, &clientAuth, 3);
	cursor->requestBuffer[3] = '\0';

	cursor->file.buffer = cursor->requestBuffer;
	cursor->overlappedResponse.buf = (char*)(cursor->requestBuffer);
	cursor->overlappedResponse.len = 3;// sizeof(CTSocks4ConnectMessage);// strlen(CONNECT_REQUEST);
	cursor->overlappedResponse.stage = CT_OVERLAPPED_SEND; //we need to send but bypass encrypt bc this is the ssl handshake
	assert(cursor->overlappedResponse.buf);
	CTCursorSendOnQueue(cursor, (char**)&(cursor->overlappedResponse.buf), cursor->overlappedResponse.len);
	*/


	CTSocks5ClientConnect clientConnect = { 0x05, 0x01, 0x00, 0x03, cursor->conn->target->url.port };
	clientConnect.PORT = htons(clientConnect.PORT);

	uint8_t hostLen = strlen(cursor->conn->target->url.host);

	cursor->headerLength = 0;
	cursor->contentLength = 0;

	cursor->headerLengthCallback = Socks5ClientConnectHeaderLengthCallback;
	cursor->responseCallback = Socks5ClientConnectResponseCallback;
	//cursor->queryCallback = SSLSecondMessageQueryCallback;

	memcpy(cursor->requestBuffer, &clientConnect, 4);
	memcpy(cursor->requestBuffer + 4, &hostLen, 1);
	memcpy(cursor->requestBuffer + 5, cursor->conn->target->url.host, hostLen);
	memcpy(cursor->requestBuffer + 5 + hostLen, &(clientConnect.PORT), 2);
	cursor->requestBuffer[5 + hostLen + 2] = '\0';

	cursor->file.buffer = cursor->requestBuffer;
	cursor->overlappedResponse.buf = (char*)(cursor->requestBuffer);
	cursor->overlappedResponse.len = 5 + hostLen + 2;// sizeof(CTSocks4ConnectMessage);// strlen(CONNECT_REQUEST);
	cursor->overlappedResponse.stage = CT_OVERLAPPED_SEND; //we need to send but bypass encrypt bc this is the ssl handshake
	assert(cursor->overlappedResponse.buf);
	CTCursorSendOnQueue(cursor, (char**)&(cursor->overlappedResponse.buf), cursor->overlappedResponse.len);


}

typedef enum CTPROTCOL
{
	//TCP PROTCOLS
	HTTP,
	HTTPS,
	REQL,
	SOCKS4,
	SOCKS4A,
	SOCKS5
}CTPROTOCOL;

coroutine int CTProxyHandshake(CTConnection* conn)
{
	int prefix_length = 0;
	CTPROTOCOL proxy_protocol = HTTP;  //HTTP proxy is the default
	char* host_prefix = strstr(conn->target->proxy.host, "://");
	if (host_prefix)
	{
		prefix_length = host_prefix - conn->target->proxy.host;
		host_prefix = conn->target->proxy.host;

		if (prefix_length >= 4 && (host_prefix[0] == 'h' && host_prefix[1] == 't' && host_prefix[2] == 't' && host_prefix[3] == 'p'))
		{
			if (prefix_length == 4) proxy_protocol = HTTP; //HTTP proxy
			else assert(1 == 0);							//HTTPS proxy not supported yet
		}
		else if (prefix_length >= 5 && (host_prefix[0] == 's' && host_prefix[1] == 'o' && host_prefix[2] == 'c' && host_prefix[3] == 'k' && host_prefix[4] == 's'))
		{
			if (prefix_length == 5) proxy_protocol = 1; //SOCKS5
			else if (prefix_length == 6)
			{
				if (host_prefix[5] == '4')		assert(1 == 0); //SOCKS4 not supported yet
				else if (host_prefix[5] == '5') proxy_protocol = SOCKS5; //SOCKS5
				else							assert(1 == 0); //unsupported SOCKS version
			}
			else if (prefix_length == 7)
			{
				if (host_prefix[5] == '4' && host_prefix[6] == '4a') assert(1 == 0); //SOCKS4a not supported yet
				else											  assert(1 == 0); //unsupported SOCKS subversion
			}
		}
		else assert(1 == 0); //unsupported proxy protocol
	}


	CTError error = { 0 };
	int status = 0;
	if (conn->socketContext.txQueue)
	{
		CTCursor* handshakeCursor = CTGetNextPoolCursor();
		//send the TLS Handshake first message in an async non-blocking fashion
		memset(handshakeCursor, 0, sizeof(CTCursor));

		if (proxy_protocol == HTTP)
		{
			handshakeCursor->headerLengthCallback = HTTPProxyHeaderLengthCallback;
			handshakeCursor->responseCallback = HTTPProxyResponseCallback;

			char CONNECT_REQUEST[1024] = "CONNECT example.com:443 HTTP/1.1\r\nHost: example.com\r\nUser-Agent: CoreTransport\r\n\r\n\0";
			memcpy(handshakeCursor->requestBuffer, CONNECT_REQUEST, strlen(CONNECT_REQUEST));
			handshakeCursor->overlappedResponse.len = strlen(CONNECT_REQUEST);
		}
		else
		{
			handshakeCursor->headerLengthCallback = Socks5ProxyHeaderLengthCallback;
			handshakeCursor->responseCallback = Socks5ClientGreetingReponseCallback;

			CTSocks5ClientGreeting clientGreeting = { 0x05, 0x01, 0x00 };
			memcpy(handshakeCursor->requestBuffer, &clientGreeting, 3);
			handshakeCursor->requestBuffer[3] = '\0';
			handshakeCursor->overlappedResponse.len = 3;
		}

		handshakeCursor->file.buffer = handshakeCursor->requestBuffer;
		handshakeCursor->overlappedResponse.buf = (char*)(handshakeCursor->requestBuffer);
		//handshakeCursor->overlappedResponse.len = strlen(CONNECT_REQUEST);

		handshakeCursor->conn = conn; //assume conn is permanent memory from core transport connection pool
		handshakeCursor->overlappedResponse.stage = CT_OVERLAPPED_SEND; //we need to bypass encrypt bc sthis is the proxy handshake
		assert(handshakeCursor->overlappedResponse.buf);
		CTCursorSendOnQueue(handshakeCursor, (char**)&(handshakeCursor->overlappedResponse.buf), handshakeCursor->overlappedResponse.len);
		return CTSuccess;
	}
	else
	{
		//assert(1 == 0);
		
		if (proxy_protocol == HTTP)
		{
			char CONNECT_REQUEST[1024] = "CONNECT example.com:443 HTTP/1.1\r\nHost: example.com\r\nUser-Agent: CoreTransport\r\n\r\n\0";
			char RESPONSE_BUFFER[1024];// = "CONNECT example.com:80 HTTP/1.1\r\nHost: example.com\r\nUser-Agent: CoreTransport\r\n\r\n\0";

			unsigned long responseLength = 1024;
			int ssl_method = conn->target->ssl.method;
			conn->target->ssl.method = CTSSL_NONE;

			CTSend(conn, (void*)(char*)CONNECT_REQUEST, strlen(CONNECT_REQUEST));

			char* connectRequestResponsePtr = (char*)CTRecv(conn, RESPONSE_BUFFER, &responseLength);
			fprintf(stderr, "CTProxyHandshake::HTT Proxy Connect RESPONSE_BUFFER = \n\n%s\n\n", connectRequestResponsePtr);
			
			//TO DO:  parse RESPONSE_BUFFER

			conn->target->ssl.method = ssl_method;

		}
		else
		{
			assert(1 == 0); //Have not implemented multimessage SOCKS handshake for synchronous pipeline yet
		}

		//do synchronous ssl routine
		if (conn->target->ssl.method > 0 && (status = CTSSLRoutine(conn, conn->socketContext.host, conn->target->ssl.ca)) != 0)
		{
			fprintf(stderr, "CTProxyHandshake::CTSSLRoutine failed with error: %d\n", (int)status);
			error.id = CTSSLHandshakeError;
			conn->target->callback(&error, conn);
			return error.id;
		}
		
	}

	return CTSuccess;
}

coroutine int CTSSLRoutine(CTConnection *conn, char * hostname, char * caPath)
{
	int ret;

	//OSStatus status;
	//Note:  PCredHandle (PSecHandle) is the WIN32 equivalent of SecCertificateRef
    CTSecCertificateRef rootCertRef = NULL;

	// Create credentials.

	//  ReqSSLCertificate (Obtains a native Security Certificate object/handle) 
	//
	//  MBEDTLS NOTES:
	//
	//  WIN32 NOTES:
	//		ReqlSecCertificateRef is of type PCredHandle on WIN32, but it needs to point to allocated memory before supplying as input to AcquireCredentialHandle (ACH)
	//		so ReqlSSLCertificate will allocate memory for this pointer and return it to the client, but the client is responsible for freeing it later 
	//
	//  DARWIN NOTES:
	//
    //		Read CA file in DER format from disk so we can use it/add it to the trust chain during the SSL Handshake Authentication
    //		-- Non (password protected) PEM files can be converted to DER format offline with OpenSSL or in app with by removing PEM headers/footers and converting (from or to?) BASE64.  
	//		   DER data may need headers chopped of in some instances.
    //		-- Password protected PEM files must have their private/public key pairs generated using PCK12.  This can be done offline with OpenSSL or in app using Secure Transport
	//

#ifdef CTRANSPORT_WOLFSSL
	rootCertRef = caPath;
#elif defined(CTRANSPORT_USE_MBED_TLS) || defined(__APPLE__)
	if (caPath)
	{
		if ((rootCertRef = CTSSLCertificate(caPath)) == NULL)
		{
			fprintf(stderr, "ReqlSSLCertificate failed to generate SecCertificateRef\n");
			CTCloseSSLSocket(conn->sslContext, conn->socket);
			return CTSSLCertificateError;
		}
	}
#endif
	//ReqlSSLInit();
	
	fprintf(stderr, "----- Credentials Initialized\n");



	//ReqlSSLContextRef sslContext = NULL;
	//  Create an SSL/TLS Context using a 3rd Party Framework such as Apple SecureTransport, OpenSSL or mbedTLS
	//  THEN send the first message in a synchronous fashion

	void* sslFirstMessageBuffer = NULL;
	int sslFirstMessageLen = 0;
	if ((conn->sslContext = CTSSLContextCreate(&(conn->socket), &rootCertRef, hostname, &sslFirstMessageBuffer, &sslFirstMessageLen)) == NULL)
	{
		//ReqlSSLCertificateDestroy(&rootCertRef);
		return CTSecureTransportError;
	}
	fprintf(stderr, "----- SSL Context Initialized\n");

#ifdef __APPLE__   
    
    //7.#  Elevate Trust Settings on the CA (Not on iOS, mon!)
    //const void *ctq_keys[] =    { kSecTrustSettingsResult };
    //const void *ctq_values[] =  { kSecTrustSettingsResultTrustRoot };
    //CFDictionaryRef newTrustSettings = CFDictionaryCreate(kCFAllocatorDefault, ctq_keys, ctq_values, 3, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    
    //status = SecTrustSettingsSetTrustSettings(rootCert, kSecTrustSettingsDomainAdmin, newTrustSettings);
    //if (status != errSecSuccess) {
    //    fprintf(stderr, "Could not change the admin trust setting for a certificate. Error: %d", status);
    //    return -1;
    //}
    
    //  Add the CA Root Certificate to the device keychain
    //  (thus letting the system determine for us that it is a valid certificate in a SecCertificateRef, but not SecIdentityRef)
    if( (status = ReqlSaveCertToKeychain( rootCertRef )) != noErr )
    {
        fprintf(stderr, "Failed to save root ca cert to device keychain!\n");
		CTCloseSSLSocket(conn->sslContext, conn->socket);
        CFRelease(rootCertRef);
        return CTSSLKeychainError;
    }
    //Note:  P12 stuff used to be sandwiched here
    //SecIdentityRef certIdentity = ReqlSSLCertficateIdentity();
#endif 

#ifdef CTRANSPORT_WOLFSSL

	//get a cursor to use for the first async non-blocking handshake message
	CTCursor* handshakeCursor = CTGetNextPoolCursor();
	memset(handshakeCursor, 0, sizeof(CTCursor));

	//WolfSSL Callbacks will need a connection object to place on CTCursor's for send/recv
	CTSSLEncryptTransient* transient = (CTSSLEncryptTransient*)&(handshakeCursor->requestBuffer[65535 - sizeof(CTSSLEncryptTransient)]);// { 0, 0, NULL };
	transient->wolf_socket_fd = 0;
	transient->messageLen = 0;
	transient->messageBuffer;
	wolfSSL_SetIOWriteCtx(conn->sslContext->ctx, transient);
#elif defined(_WIN32)
	//send the first message in an async non-blocking fashion on a thread pool queeu
	if (conn->socketContext.txQueue)
	{
		CTCursor* handshakeCursor = CTGetNextPoolCursor();
		//send the TLS Handshake first message in an async non-blocking fashion
		memset(handshakeCursor, 0, sizeof(CTCursor));

		handshakeCursor->headerLengthCallback = SSLFirstMessageHeaderLengthCallback;
		handshakeCursor->responseCallback = SSLFirstMessageResponseCallback;

		handshakeCursor->file.buffer = handshakeCursor->requestBuffer;
		handshakeCursor->overlappedResponse.buf = (char*)(sslFirstMessageBuffer);
		handshakeCursor->overlappedResponse.len = sslFirstMessageLen;

		handshakeCursor->conn = conn; //assume conn is permanent memory from core transport connection pool
		handshakeCursor->overlappedResponse.stage = CT_OVERLAPPED_SEND; //we need to bypass encrypt bc sthis is the ssl handshake
		assert(handshakeCursor->overlappedResponse.buf);
		CTCursorSendOnQueue(handshakeCursor, (char**)&(handshakeCursor->overlappedResponse.buf), handshakeCursor->overlappedResponse.len);
		return CTSuccess;
	}

	//send first message in a blocking fashion
	CTSSLHandshakeSendFirstMessage(conn->socket, conn->sslContext, sslFirstMessageBuffer, sslFirstMessageLen);
#endif
	//else if( !conn->socketContext.txQueue )
	//{

		//commence with the rest of the TLS handshake
		if ((ret = CTSSLHandshake(conn->socket, conn->sslContext, rootCertRef, hostname)) != noErr)
		{
#ifdef CTRANSPORT_WOLFSSL
			if (ret == SSL_ERROR_WANT_READ)
			{
				assert(1 == 0);
				return CTSuccess;

			}
			else if (ret == SSL_ERROR_WANT_WRITE)
			{


				//send the TLS Handshake first message in an async non-blocking fashion
				handshakeCursor->headerLengthCallback = SSLFirstMessageHeaderLengthCallback;
				handshakeCursor->responseCallback = SSLFirstMessageResponseCallback;
				//handshakeCursor->queryCallback = SSLFirstMessageQueryCallback;
				handshakeCursor->file.buffer = handshakeCursor->requestBuffer;
				handshakeCursor->overlappedResponse.buf = transient->messageBuffer;// (char*)(sslFirstMessageBuffer);
				handshakeCursor->overlappedResponse.len = transient->messageLen;// sslFirstMessageLen;

				handshakeCursor->conn = conn; //assume conn is permanent memory from core transport connection pool
				handshakeCursor->overlappedResponse.stage = CT_OVERLAPPED_SEND; //we need to bypass encrypt bc sthis is the ssl handshake
				assert(handshakeCursor->overlappedResponse.buf);
				
				CTCursorSendOnQueue(handshakeCursor, (char**)&(handshakeCursor->overlappedResponse.buf), handshakeCursor->overlappedResponse.len);
				return CTSuccess;
			}
#endif
			//ReqlSSLCertificateDestroy(&rootCertRef);
			fprintf(stderr, "CTSSLHandshake with status:  %d\n", ret);
			CTCloseSSLSocket(conn->sslContext, conn->socket);
			return CTSSLHandshakeError;
		}

		if ((ret = CTVerifyServerCertificate(conn->sslContext, hostname)) != noErr)
		{
			//ReqlSSLCertificateDestroy(&rootCertRef);
			fprintf(stderr, "CTVerifyServerCertificate with status:  %d\n", ret);
			CTCloseSSLSocket(conn->sslContext, conn->socket);
			return CTSSLHandshakeError;
		}


	//}


	//mbedtls_ssl_flush_output(&(conn->sslContext->ctx));

	//fprintf(stderr, "conn->Socket = %d\n", conn->socket);
	//fprintf(stderr, "conn->SocketContext.socket = %d\n", conn->socketContext.socket);
	//fprintf(stderr, "conn->SocketContext.ctx.fd = %d\n", conn->socketContext.ctx.fd);

	//fprintf(stderr, "conn->sslContext = %llu\n", conn->sslContext);

	/*
	//char REQL_MAGIC_NUMBER_BUF[5] = "\xc3\xbd\xc2\x34";
	size_t bytesToWrite = 4;
	int errOrBytesWritten = mbedtls_ssl_write(&(conn->sslContext->ctx), (const unsigned char *)REQL_MAGIC_NUMBER_BUF, bytesToWrite);
	if (errOrBytesWritten < 0) {
		if (errOrBytesWritten != MBEDTLS_ERR_SSL_WANT_READ &&
			errOrBytesWritten != MBEDTLS_ERR_SSL_WANT_WRITE) {

			fprintf(stderr, "ReqlSSLWrite::mbedtls_ssl_write = %d\n", errOrBytesWritten);

		}
}
	fprintf(stderr, "ReqlSSLWrite::erroOrBytesWritten = %d\n", errOrBytesWritten);
	//fprintf(stderr, "ReqlSSLWrite::erroOrBytesWritten = %d\n", errOrBytesWritten);

	system("pause");

	*/
	//ReqlSend(&conn, (void*)REQL_MAGIC_NUMBER_BUF, 4);


 #ifdef __APPLE__   
    CFRelease(rootCert);
#endif
    return CTSuccess;
}

unsigned long CTPageSize()
{
#ifdef _WIN32
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);     // Initialize the structure.
	fprintf(stderr, TEXT("CTPageSize() = %d.\n"), systemInfo.dwPageSize);
	return (unsigned long)systemInfo.dwPageSize;
#elif defined(__APPLE__)

#endif
}

coroutine int CTConnectRoutine(CTTarget * service, CTConnectionClosure callback)
{
    OSStatus status;    //But Reql API functions that purely rely on system OSStatus functions will return OSStatus
	//int ret;
	//char port[6];

    //  Coroutines rely on integration with a run loop [running on a thread] to return operation
    //  to the coroutine when it yields operation back to the calling run loop
    //
    //  For Darwin platforms that either:
    //   1)  A CFRunLoop associated with a main thread in a cocoa app [Cocoa]
    //   2)  A CFRunLoop run from a background thread in a Darwin environment [Core Foundation]
    //   3)  A custom event loop implemented with kqueue [Custom]
    //CFRunLoopRef         runLoop;
    //CFRunLoopSourceRef   runLoopSource;
    //CFRunLoopObserverRef runLoopObserver;
    
    //Allocate connection as stack memory until we are sure the connection is valid
    //At which point we will copy result to the client via the closure result block
    struct CTConnection conn = {0};
    struct CTError error = {(CTErrorClass)0,0,0};    //Reql API client functions will generally return ints as errors

	conn.socketContext.host = service->url.host;
    //Allocate connection as dynamic memory
    //ReØMQL * conn = (ReØMQL*)calloc( 1, sizeof(ReØMQL));//{0};
    
    /*
    //Get current run loop
    runLoop = CFRunLoopGetCurrent();
    
    //Create and install a dummy run loop source as soon as possible to keep the loop alive until the async coroutine completes
    runLoopSource = CFRunLoopSourceCreate(NULL, 0, &REQL_RUN_LOOP_SRC_CTX);
    CFRunLoopAddSource( runLoop, runLoopSource, kCFRunLoopDefaultMode);
    
    //Create a run loop observer to inject yields from the run loop back to this (and other) coroutines
    // TO DO:  It would be nice to avoid this branching
    CFOptionFlags runLoopActivities = ( runLoop == CFRunLoopGetMain() ) ? REQL_RUN_LOOP_WAITING : REQL_RUN_LOOP_RUNNING;
    if ( !(runLoopObserver = CFRunLoopObserverCreate(kCFAllocatorDefault, runLoopActivities, true, 0, &ReqlRunLoopObserver, NULL)) )
    {
        fprintf(stderr, "Failed to create run loop observer!\n");
        error.id = ReqlRunLoopError;
        goto CONN_CALLBACK;
    }

    //  Install a repeating run loop observer so we can call coroutine API yield
    CFRunLoopAddObserver(runLoop, runLoopObserver, kCFRunLoopDefaultMode);
    */

#ifdef _WIN32//CTRANSPORT_USE_MBED_TLS
	fprintf(stderr, "Before CTSocketCreate()\n");
    //  Create a [non-blocking] TCP socket and return a socket fd
    if( (conn.socket = CTSocketCreate((service->cxQueue ? 1 : 0))) < 0)
    {
        error.id = (int)conn.socket; //Note: hope that 64 bit socket handle is within 32 bit range?!
        goto CONN_CALLBACK;
        //return conn.socket;
    }

	fprintf(stderr, "Before CTSocketConnect\n");

    //  Connect the [non-blocking] socket to the server asynchronously and return a kqueue fd
    //  associated with the socket so we can listen/wait for the connection to complete

	if (service->cxQueue)
	{
		//CTSocketClose(service->socket);
		service->socket = conn.socket;
		CreateIoCompletionPort((HANDLE)service->socket, service->cxQueue, (ULONG_PTR)NULL, 1);
		return CTSocketConnectOnQueue(service->socket, service, callback);
	}
	else if( (conn.event_queue = CTSocketConnect(conn.socket, service)) < 0 )
    {
        error.id = (int)conn.socket;
        goto CONN_CALLBACK;
    }

	/*
	if (CTSocketCreateEventQueue(&(conn.socketContext)) < 0)
	{
		fprintf(stderr, "ReqlSocketCreateEventQueue failed\n");
		error.id = (int)conn.event_queue;
		goto CONN_CALLBACK;
	}
	*/
#else
	itoa((int)service->port, port, 10);
	mbedtls_net_init(&(conn.socketContext.ctx));
	if ( (ret = mbedtls_net_connect(&(conn.socketContext.ctx), service->host, port, MBEDTLS_NET_PROTO_TCP)) != 0)
	{
		Reql_fprintf(stderr, "ReqlConnectRoutine::mbedtls_net_connect failed and returned %d\n\n", ret);
		return ret;
	}
#endif

    //Yield operation back to the calling run loop
    //This wake up is critical for pumping on idle, but I am not sure why it needs to come after the yield
#ifndef _WIN32
    yield();
#endif
#ifdef __APPLE__
    CFRunLoopWakeUp(CFRunLoopGetCurrent());
#endif

	fprintf(stderr, "Before CTSocketWait\n");

    //Wait for socket connection to complete
    if( CTSocketWait(conn.socket, conn.event_queue, EVFILT_WRITE) != conn.socket )
    {
        fprintf(stderr, "CTSocketWait failed\n");
        error.id = CTSocketEventError;
        CTSocketClose(conn.socket);
        goto CONN_CALLBACK;
        //return ReqlEventError;
    }

	fprintf(stderr, "Before CTSocketGetError\n");

    //Check for socket error to ensure that the connection succeeded
    if( (error.id = CTSocketGetError(conn.socket)) != 0 )
    {
        // connection failed; error code is in 'result'
        fprintf(stderr, "socket async connect failed with result: %d", error.id );
        CTSocketClose(conn.socket);
        error.id = CTSocketConnectError;
        goto CONN_CALLBACK;
        //return ReqlConnectError;
    }
    
    //There is some short order work that would be good to parallelize regarding loading the cert and saving it to keychain
    //However, the SSLHandshake loop has already been optimized with yields for every asynchronous call

	fprintf(stderr, "Before CTSSLRoutine Host = %s\n", conn.socketContext.host);
	fprintf(stderr, "Before CTSSLRoutine Host = %s\n", service->url.host);

	if (service->proxy.host)
	{
		conn.target = service;
		CTProxyHandshake(&conn);
	}
    else if( service->ssl.method > 0 && (status = CTSSLRoutine(&conn, conn.socketContext.host, service->ssl.ca)) != 0 )
    {
        fprintf(stderr, "CTSSLRoutine failed with error: %d\n", (int)status );
        error.id = CTSSLHandshakeError;
        goto CONN_CALLBACK;
        //return status;
    }

	//if the client specified tx, rx queues on the target as input
	//copy them to the socket context now for the blocking ssl handshake routine if no cxQueue was specified
	conn.socketContext.txQueue = service->txQueue;// CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, dwCompletionKey, 0);
	conn.socketContext.rxQueue = service->rxQueue;// CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, dwCompletionKey, 0);
	if (conn.socketContext.rxQueue)
	{
		ULONG_PTR dwCompletionKey = (ULONG_PTR)NULL;
		CreateIoCompletionPort((HANDLE)conn.socket, conn.socketContext.rxQueue, dwCompletionKey, 1);
	}
	
    //The connection completed successfully, allocate memory to return to the client
    error.id = CTSuccess;
    
CONN_CALLBACK:
    //fprintf(stderr, "before callback!\n");
	conn.target = service;
    callback(&error, &conn);
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
    return error.id;
}

void populate_sockaddr(int af, int port, char addr[], struct sockaddr_storage* dst, socklen_t* addrlen) {

	if (af == AF_INET) {
		struct sockaddr_in* dst_in4 = (struct sockaddr_in*)dst;

		*addrlen = sizeof(*dst_in4);
		memset(dst_in4, 0, *addrlen);
		dst_in4->sin_family = af;
		dst_in4->sin_port = htons(port);
		inet_pton(af, addr, &dst_in4->sin_addr);

	}
	else if (af == AF_INET6) {
		struct sockaddr_in6* dst_in6 = (struct sockaddr_in6*)dst;

		*addrlen = sizeof(*dst_in6);
		memset(dst_in6, 0, *addrlen);
		dst_in6->sin6_family = af;
		dst_in6->sin6_port = htons(port);
		// unnecessary because of the memset(): dst_in6->sin6_flowinfo = 0;
		inet_pton(af, addr, &dst_in6->sin6_addr);
	} // else ...
}


char* DNSResolveHeaderLengthCallback(struct CTCursor* cursor, char* buffer, unsigned long length)
{
	fprintf(stderr, "DNSResolveResponseCallback headerLengthCallback...\n\n");

	//the purpose of this function is to return the end of the header in the 
	//connection's tcp stream to CoreTransport decrypt/recv thread
	char* endOfHeader = buffer;// +sizeof(ReqlQueryMessageHeader);

	//The client *must* set the cursor's contentLength property
	//to aid CoreTransport in knowing when to stop reading from the socket
	cursor->contentLength = length;// ((ReqlQueryMessageHeader*)buffer)->length;

	//The cursor headerLength is calculated as follows after this function returns
	return endOfHeader;
}


void DNSResolveResponseCallback(CTError* err, CTCursor* cursor)
{
	fprintf(stderr, "DNSResolveResponseCallback header:  \n\n%.*s\n\n", cursor->headerLength, cursor->requestBuffer);

	int ret = 0;
	socklen_t addrlen;
	int DNS_PORT = 53;
	//populate_sockaddr(AF_INET, (int)DNS_PORT, "8.8.8.8", &(cursor->target->url.addr), &addrlen);

	char* target_host = cursor->target->proxy.host ? cursor->target->proxy.host : cursor->target->url.host;
	short target_port = cursor->target->proxy.host ? cursor->target->proxy.port : cursor->target->url.port;

	char* host_prefix = strstr(target_host, "://");
	if (host_prefix) target_host = host_prefix + 3;

	CTError error = { 0,0,NULL };
	//check the result
	//int fromlen = sizeof(target->url.addr);
	int cbReceived = cursor->contentLength + cursor->headerLength;// recvfrom(target->socket, buff, 2048, 0, (struct sockaddr*)&(target->url.addr), &fromlen);

	if (cbReceived < 0)
	{
		fprintf(stderr, "Error Receiving Data: %d", cbReceived);
		error.id = cbReceived;
		goto CONN_CALLBACK;
	}

	if (0 == cbReceived)
	{
		fprintf(stderr, "No Data Received: %d", cbReceived);
		assert(1 == 0);
		//error.id = cbReceived;
		//goto CONN_CALLBACK;
	}

	fprintf(stderr, "recvfrom (%d) = \n\n%.*s\n\n", cbReceived, cbReceived, cursor->file.buffer);


	//-- Parse the DNS response received with DNS API
	//local pDnsResponseBuff = ffi.cast("DNS_MESSAGE_BUFFER*", buff);
	DNS_MESSAGE_BUFFER* pDnsResponseBuff = (DNS_MESSAGE_BUFFER*)cursor->file.buffer;
	PDNS_HEADER pHeader = &(pDnsResponseBuff->MessageHead);
	DNS_BYTE_FLIP_HEADER_COUNTS(pHeader);
	if (pDnsResponseBuff->MessageHead.Xid != cursor->target->dns.wID)
	{
		fprintf(stderr, "Wrong TransactionID: X.id %d != wID %d", pDnsResponseBuff->MessageHead.Xid, cursor->target->dns.wID);
		assert(1 == 0);
	}

	DNS_RECORD* pRecord = NULL;// ffi.new("DNS_RECORD *[1]", nil);
	WORD dnsLen = (WORD)cbReceived;
	DNS_STATUS status = DnsExtractRecordsFromMessage_UTF8(pDnsResponseBuff, dnsLen, &(pRecord));

	if (status != 0)
	{
		fprintf(stderr, "DnsExtractRecordsFromMessage_UTF8 failed with error code: %d\n", status);
		//assert(1 == 0);
		populate_sockaddr(AF_INET, (int)(target_port), target_host, &(cursor->target->url.addr), &addrlen);
	}


	//if (pRecord == NULL) assert(1 == 0);

	//pRecord = pRecord[0];
	DNS_RECORD* pRecordA = (DNS_RECORD*)pRecord;

	while (pRecordA && pRecordA->wType == DNS_TYPE_A)
	{
		//local a = IN_ADDR();
		//a.S_addr = record.Data.A.IpAddress

		//TO DO:  we are only handling A records and only the first one
		populate_sockaddr(AF_INET, (int)(target_port), target_host, &(cursor->target->url.addr), &addrlen);
		cursor->target->url.addr.sin_addr.s_addr = pRecordA->Data.A.IpAddress;
		assert(INADDR_NONE != cursor->target->url.addr.sin_addr.s_addr);
		//break;

		pRecordA = pRecordA->pNext;
	}

	//--Free the resources
	if (pRecord)
		DnsRecordListFree(pRecord, DnsFreeRecordList);


	//initiate the async connection
	fprintf(stderr, "CTTargetResolveHost host = %s\n", target_host);
	CTConnectRoutine(cursor->target, cursor->target->callback);
	return;

CONN_CALLBACK:
	//CTConnection conn = { 0 };
	//conn.target = target;
	cursor->target->callback(&error, NULL);
	return;// error.id;
}


/***
 *  CTTargetResolveHost
 *
 *  DNS Lookups can take a long time on iOS, so we need an async DNS resolve mechanism.
 *  However, getaddrinfo_a is not available for Darwin, so we use CFHost API to schedule
 *  the result of the async DNS lookup on internal CFNetwork thread which will post our result
 *  callback to the calling runloop
 *
 ***/
coroutine int CTTargetResolveHost(CTTarget* target, CTConnectionClosure callback)
{

#ifndef _WIN32
	//Launch asynchronous query using modified version of Sustrik's libdill DNS implementation
	struct dill_ipaddr addr[1];
	struct dns_addrinfo* ai = NULL;
	if (dill_ipaddr_dns_query_ai(&ai, addr, 1, service->host, service->port, DILL_IPADDR_IPV4, service->dns.resconf, service->dns.nssconf) != 0 || ai == NULL)
	{
		fprintf(stderr, "service->dns.resconf = %s", service->dns.resconf);
		fprintf(stderr, "dill_ipaddr_dns_query_ai failed with errno:  %d", errno);
		ReqlError err = { ReqlDriverErrorClass, ReqlDNSError, NULL };
		callback(&err, NULL);
		return ReqlDNSError;
	}

	//TO DO:  We could use this opportunity to do work that validates ReqlService properties, like loading SSL Certificate
	//CFRunLoopWakeUp(CFRunLoopGetCurrent());
	//CFRunLoopSourceSignal(runLoopSource);
	CFRunLoopWakeUp(CFRunLoopGetCurrent());

	//fprintf(stderr, "Before DNS wait\n");
	yield();

	int numResolvedAddresses = 0;
	if ((numResolvedAddresses = dill_ipaddr_dns_query_wait_ai(ai, addr, 1, service->port, DILL_IPADDR_IPV4, -1)) < 1)
	{
		//fprintf(stderr, "dill_ipaddr_dns_query_wait_ai failed to resolve any IPV4 addresses!");
		ReqlError err = { ReqlDriverErrorClass, ReqlDNSError, NULL };
		callback(&err, NULL);
		return ReqlDNSError;
	}

	//fprintf(stderr, "DNS resolved %d ip address(es)\n", numResolvedAddresses);
	struct sockaddr_in* ptr = (struct sockaddr_in*)addr;//(addr[0]);
	service->url.addr = *ptr;//(struct sockaddr_in*)addresses;

#elif 0

	//TODO:
		//--------------------------------------------------------------------
	//  ConnectAuthSocket establishes an authenticated socket connection 
	//  with a server and initializes needed security package resources.

	DWORD dwRetval;
	char port[6];
	struct addrinfo* result = NULL;
	struct addrinfo* ptr = NULL;
	struct addrinfo hints;
	int i = 0;

	struct sockaddr_in* sockaddr_ipv4;
	//unsigned long  ulAddress;
	struct hostent* pHost = NULL;
	//sockaddr_in    sin;

	//--------------------------------------------------------------------
	//  Lookup the server's address.
	fprintf(stderr, "ReqlServiceResolveHost host = %s\n", target->host);

	target->url.addr.sin_addr.s_addr = inet_addr(target->host);
	if (INADDR_NONE == target->url.addr.sin_addr.s_addr)
	{

		/*
		pHost = gethostbyname (service->host);
		if ( pHost==NULL) {
			fprintf(stderr, "gethostbyname failed\n");
			//WSACleanup();
			return -1;
		}
		memcpy((void*)&(service->url.addr.sin_addr.s_addr) , (void*)(pHost->h_addr), (size_t)(strlen(pHost->h_addr)) );

		//memcpy((void*)&(service->addr) , (void*)*(pHost->h_addr_list), (size_t)(pHost->h_length));
		*/

		//set port param
		_itoa((int)target->port, port, 10);
		//
		memset(&hints, 0, sizeof(struct addrinfo));
		dwRetval = getaddrinfo(target->host, port, &hints, &result);
		if (dwRetval != 0) {
			fprintf(stderr, "getaddrinfo failed with error: %d\n", dwRetval);
			//WSACleanup();
			return -1;
		}

		// Retrieve each address and print out the hex bytes
		for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
		{

			fprintf(stderr, "getaddrinfo response %d\n", i++);
			fprintf(stderr, "\tFlags: 0x%x\n", ptr->ai_flags);
			fprintf(stderr, "\tFamily: ");
			fprintf(stderr, "\tLength of this sockaddr: %d\n", (int)(ptr->ai_addrlen));
			fprintf(stderr, "\tCanonical name: %s\n", ptr->ai_canonname);

			if (ptr->ai_family != AF_INET)
			{
				continue;
			}
			else
			{
				sockaddr_ipv4 = (struct sockaddr_in*)result->ai_addr;
				fprintf(stderr, "ReqlServiceResolveHost host before copy = %s\n", target->host);

				memcpy(&(target->url.addr), sockaddr_ipv4, result->ai_addrlen);
				fprintf(stderr, "ReqlServiceResolveHost host after copy = %s\n", target->host);

				break;
			}

		}
		freeaddrinfo(result);


	}

#else

	struct CTConnection conn = { 0 };
	struct CTError error = { (CTErrorClass)0,0,0 };    //Reql API client functions will generally return ints as errors

	//conn.socketContext.host = target->host;

	//local remoteAddr = sockaddr_in(53, AF_INET);
	//remoteAddr.sin_addr.S_addr = ws2_32.inet_addr(servername);


	fprintf(stderr, "CTTargetResoleHost Start\n");
	fflush(stderr);

	//struct sockaddr_strorage addr;
	socklen_t addrlen;
	int DNS_PORT = 53;
	char* target_host = target->proxy.host ? target->proxy.host : target->url.host;
	short target_port = target->proxy.host  ? target->proxy.port : target->url.port;

	//push past any prefix if it's there
	char* host_prefix = strstr(target_host, "://");
	if (host_prefix) target_host = host_prefix + 3;

	populate_sockaddr(AF_INET, (int)DNS_PORT, "8.8.8.8", &(target->url.addr), &addrlen);

	//target->url.addr.sin_addr.s_addr = inet_addr("8.8.8.8");
	assert(INADDR_NONE != target->url.addr.sin_addr.s_addr);


	fprintf(stderr, "Before CTSocketCreateUDP()\n");
	fflush(stderr);

	//  Create a [non-blocking] TCP socket and return a socket fd
	if ((target->socket = CTSocketCreateUDP()) < 0)
	{
		error.id = (int)target->socket; //Note: hope that 64 bit socket handle is within 32 bit range?!
		goto CONN_CALLBACK;
	}

	fprintf(stderr, "After CTSocketCreateUDP()\n");
	fflush(stderr);

	CreateIoCompletionPort((HANDLE)target->socket, target->cxQueue, (ULONG_PTR)NULL, 1);


	fprintf(stderr, "After CreateIoCompletionPort()\n");
	fflush(stderr);


	/*
	fprintf(stderr, "Before CTSocketCreateEventQueue(0)\n");

	if (CTSocketCreateEventQueue(&(conn.socketContext)) < 0)
	{
		fprintf(stderr, "ReqlSocketCreateEventQueue failed\n");
		error.id = (int)conn.event_queue;
		goto CONN_CALLBACK;
	}
	*/

	//wType = wType or ffi.C.DNS_TYPE_A
	//msTimeout = msTimeout or 60 * 1000  -- 1 minute


	//-- Prepare the DNS request
	//local dwBuffSize = ffi.new("DWORD[1]", 2048);
	//local buff = ffi.new("uint8_t[2048]")
	//local wID = clock:GetCurrentTicks() % 65536;

	DWORD dwBuffSize = 2048;
	uint8_t buff[2048];
	WORD wType = DNS_TYPE_A;
	BOOL wRecursiveNameQuery = TRUE;

	target->dns.wID = GetTickCount() % 65536;

	//wType = ffi.C.DNS_TYPE_MX - mail records
	//wType = ffi.C.DNS_TYPE_CNAME

	memset(buff, 0, 2048);

	//TO DO:  Figure out how to write DNS Question manually for fully xplatform goodness
	//local res = windns_ffi.DnsWriteQuestionToBuffer_UTF8(ffi.cast("DNS_MESSAGE_BUFFER*", buff), dwBuffSize, ffi.cast("char *", strToQuery), wType, wID, true)
	BOOL res = DnsWriteQuestionToBuffer_UTF8((DNS_MESSAGE_BUFFER*)buff, &dwBuffSize, (char*)(target_host), wType, target->dns.wID, wRecursiveNameQuery);
	if (res == 0)
	{
		error.id = CTDNSError;
		goto CONN_CALLBACK;
	}

	fprintf(stderr, "DnsWriteQuestionToBuffer_UTF8 (%d) = \n\n%.*s\n\n", dwBuffSize, dwBuffSize, buff);
	fflush(stderr);

	//-- Send the request.
	int iRes = sendto(target->socket, buff, dwBuffSize, 0, (struct sockaddr*)&(target->url.addr), sizeof(target->url.addr));

	if (iRes == SOCKET_ERROR)
	{
		fprintf(stderr, "Error sending data: %d", WSAGetLastError());
		error.id = WSAGetLastError();
		goto CONN_CALLBACK;
	}

	if (target->cxQueue)
	{
		//TO DO: figure out how to maintain using the same cursor for the connection DNS resolve and TLS handshake...
		CTCursor* DNSResolveCursor = CTGetNextPoolCursor();

		memset(DNSResolveCursor, 0, sizeof(CTCursor));

		DNSResolveCursor->target = target;
		DNSResolveCursor->headerLengthCallback = DNSResolveHeaderLengthCallback;
		DNSResolveCursor->responseCallback = DNSResolveResponseCallback;

		DNSResolveCursor->file.buffer = DNSResolveCursor->requestBuffer;
		DNSResolveCursor->overlappedResponse.buf = DNSResolveCursor->file.buffer;
		DNSResolveCursor->overlappedResponse.len = 2048;// (char*)(OutBuffers[0].cbBuffer);
		unsigned long msgLength = 2048;

		CTOverlappedTarget* olTarget = (CTOverlappedTarget*)&(target->overlappedTarget);// (target->ctx);
		olTarget->cursor = DNSResolveCursor;
		olTarget->target = target;
		return CTTargetAsyncRecvFrom((CTOverlappedTarget**)&(olTarget), (char*)(DNSResolveCursor->overlappedResponse.buf), 0, &msgLength);
	}
	else
	{
		//-- Try to receive the results
		//local RecvFromAddr = sockaddr_in();
		//local RecvFromAddrSize = ffi.sizeof(RecvFromAddr);
		//local cbReceived, err = self.Socket:receiveFrom(RecvFromAddr, RecvFromAddrSize, buff, 2048);

		//struct sockaddr_in RecvFromAddr = sockaddr_in(IPPORT_DNS, AF_INET);
		memset(buff, 0, 2048);

		int fromlen = sizeof(target->url.addr);
		int cbReceived = recvfrom(target->socket, buff, 2048, 0, (struct sockaddr*)&(target->url.addr), &fromlen);

		if (cbReceived < 0)
		{
			fprintf(stderr, "Error Receiving Data: %d", cbReceived);
			error.id = cbReceived;
			goto CONN_CALLBACK;
		}

		if (0 == cbReceived)
		{
			fprintf(stderr, "No Data Received: %d", cbReceived);
			assert(1 == 0);
			//error.id = cbReceived;
			//goto CONN_CALLBACK;
		}

		fprintf(stderr, "recvfrom (%d) = \n\n%.*s\n\n", cbReceived, cbReceived, buff);


		//-- Parse the DNS response received with DNS API
		//local pDnsResponseBuff = ffi.cast("DNS_MESSAGE_BUFFER*", buff);
		DNS_MESSAGE_BUFFER* pDnsResponseBuff = (DNS_MESSAGE_BUFFER*)buff;
		PDNS_HEADER pHeader = &(pDnsResponseBuff->MessageHead);
		DNS_BYTE_FLIP_HEADER_COUNTS(pHeader);
		if (pDnsResponseBuff->MessageHead.Xid != target->dns.wID)
		{
			fprintf(stderr, "Wrong TransactionID: X.id %d != wID %d", pDnsResponseBuff->MessageHead.Xid, target->dns.wID);
			assert(1 == 0);
		}

		DNS_RECORD* pRecord = NULL;// ffi.new("DNS_RECORD *[1]", nil);
		WORD dnsLen = (WORD)cbReceived;
		DNS_STATUS status = DnsExtractRecordsFromMessage_UTF8(pDnsResponseBuff, dnsLen, &(pRecord));

		if (status != 0)
		{
			fprintf(stderr, "DnsExtractRecordsFromMessage_UTF8 failed with error code: %d\n", status);
			//assert(1 == 0);

			populate_sockaddr(AF_INET, (int)(target_port), target_host, &(target->url.addr), &addrlen);

		}

		else
		{
			if (pRecord == NULL) assert(1 == 0);

			//pRecord = pRecord[0];
			DNS_RECORD* pRecordA = (DNS_RECORD*)pRecord;

			while (pRecordA )
			{
				//local a = IN_ADDR();
				//a.S_addr = record.Data.A.IpAddress

				if (pRecordA->wType == wType)
				{
					//TO DO:  we are only handling A records and only the first one
					populate_sockaddr(AF_INET, (int)(target_port), target_host, &(target->url.addr), &addrlen);
					target->url.addr.sin_addr.s_addr = pRecordA->Data.A.IpAddress;
					assert(INADDR_NONE != target->url.addr.sin_addr.s_addr);
					break;
				}
				pRecordA = pRecordA->pNext;
			}

			//--Free the resources
			if (pRecord)
				DnsRecordListFree(pRecord, DnsFreeRecordList);
		}

#endif    
		//Perform the socket connection
		fprintf(stderr, "CTTargetResolveHost host = %s\n", target_host);
		return CTConnectRoutine(target, callback);
	}

CONN_CALLBACK:
	conn.target = target;
	callback(&error, &conn);
	return error.id;
    /*
    //Remove observer that yields to coroutines
    CFRunLoopRemoveObserver(CFRunLoopGetCurrent(), runLoopObserver, kCFRunLoopDefaultMode);
    //Remove dummy source that keeps run loop alive
    CFRunLoopRemoveSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);
    
    //release runloop object memory
    CFRelease(runLoopObserver);
    CFRelease(runLoopSource);
    */
    
    //return CTSuccess;
}






/***
 *  CTConnect
 *
 *  Asynchronously connect to a remote RethinkDB Service with a ReqlService
 *
 *  Schedules DNS host resolve and returns immediately
 ***/
int CTConnect(CTTarget * target, CTConnectionClosure callback)
{
	//-------------------------------------------------------------------
    //  Initialize the socket libraries (ie Winsock 2)
	CTSocketInit();

	//-------------------------------------------------------------------
    //  Initialize and the SSL SSP security package.
	CTSSLInit();

	//-------------------------------------------------------------------
    //  Initialize the scram SSP security package.
	ca_scram_init();
	
    //  Issue a request to resolve Host DNS to a sockaddr asynchonrously using CFHost API
    //  It will perform the DNS request asynchronously on a bg thread internally maintained by CFHost API
    //  and return the result via callback to the current run loop
    //go(ReqlServiceResolveHost(service, callback));

	//Resolve on ThreadQueue vs Resolve on calling thread
	if (target->cxQueue)
		return CTTargetConnectOnQueue(target, callback);
	else
		return CTTargetResolveHost(target, callback);

    //return CTSuccess;
}


//A helper function to close an SSL Context and a socket in one shot
int CTCloseSSLSocket(CTSSLContextRef sslContextRef, CTSocket socketfd)
{
	CTSSLContextDestroy(sslContextRef);
    return CTSocketClose(socketfd);
}


int CTCloseConnection( CTConnection * conn )
{
	//if( conn->query_buffers )
	//	free(conn->query_buffers);
	/*
	if (conn->query_buffers)
	{
		VirtualFree(conn->query_buffers, 0, MEM_RELEASE);
		//free( conn->response_buffers );
	}
	conn->query_buffers = NULL;
	
	if( conn->response_buffers )
	{
		VirtualFree(conn->response_buffers, 0, MEM_RELEASE);
		//free( conn->response_buffers );
	}
	conn->response_buffers = NULL;
	*/
	
    return CTCloseSSLSocket(conn->sslContext, conn->socket);
}
