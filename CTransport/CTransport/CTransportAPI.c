#include "../CTransport.h"


#include <assert.h>



#ifndef EVFILT_READ
#define EVFILT_READ		(-1)
#define EVFILT_WRITE	(-2)
#define EVFILT_TIMER	(-7)
#define EV_EOF			0x8000
#endif


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

	 //printf("SSLRead remainingBufferSize = %d, msgLength = %d\n", (int)remainingBufferSize, (int)*msgLength);

    //struct kevent kev;
    //uintptr_t timout_event = -1;
    //uintptr_t out_of_range_event = -2;
    //uint32_t timeout = UINT_MAX;  //uint max will translate to timeout struct of null, which is timeout of forever
    
    //printf("ReqlRecv Waiting for read...\n");
    //uintptr_t ret_event = kqueue_wait_with_timeout(r->event_queue, &kev, EVFILT_READ | EV_EOF, timout_event, out_of_range_event, r->socket, r->socket, timeout);
    if( CTSocketWait(conn->socket, conn->event_queue, EVFILT_READ | EV_EOF) != conn->socket )
    {
        printf("something went wrong\n");
        return decryptedMessagePtr;
    }
#ifdef _WIN32
	ret = CTSSLRead(conn->socket, conn->sslContext, msg, &remainingBufferSize);
	if( ret <= 0 )
		ret = 0;
	else
	{
#ifdef CTRANSPORT_USE_MBED_TLS
		decryptedMessagePtr = msg;
#elif defined(_WIN32)
		decryptedMessagePtr = (char*)msg + conn->sslContext->Sizes.cbHeader;
#else
		decryptedMessagePtr = msg;
#endif
	}
	/*
	ret = (size_t)recv(conn->socket, (char*)msg, remainingBufferSize, 0 );
	
	if(ret == SOCKET_ERROR)
    {
        printf("**** ReqlRecv::Error %d reading data from server\n", WSAGetLastError());
        //scRet = SEC_E_INTERNAL_ERROR;
        return -1;
    }
    else if(ret== 0)
    {
        printf("**** ReqlRecv::Server unexpectedly disconnected\n");
        //scRet = SEC_E_INTERNAL_ERROR;
        return -1;
    }
    printf("ReqlRecv::%d bytes of handshake data received\n\n%.*s\n\n", ret, ret, (char*)msg);
	*/

	*msgLength = (size_t)ret;
	totalMsgLength += *msgLength;
	remainingBufferSize -= *msgLength;
#elif defined(__APPLE__)
    status = SSLRead(conn->sslContext, msg, remainingBufferSize, msgLength);

	totalMsgLength += *msgLength;
    remainingBufferSize -= *msgLength;

    //printf("Before errSSLWouldBlock...\n");

    while( status == errSSLWouldBlock)
    {
        //TO DO:  Figure out why this is happening for changefeeds ...
        printf("SSLRead remainingBufferSize = %d, msgLength = %d\n", (int)remainingBufferSize, (int)*msgLength);
        status = SSLRead(conn->sslContext, msg+totalMsgLength, remainingBufferSize, msgLength);
        totalMsgLength += *msgLength;
        remainingBufferSize -= *msgLength;
    }

	if( status != noErr )
    {
        printf("ReqlRecv::SSLRead failed with error: %d\n", (int)status);
    }
#endif
    //printf("SSLRead final remainingBufferSize = %d, msgLength = %d\n", (int)remainingBufferSize, (int)*msgLength);
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
    //printf("\nconn->token = %llu", conn->token);
    //printf("msg = %s\n", (char*)msg + sizeof( ReqlQueryMessageHeader) );
    uint8_t * msg_chunk = (uint8_t*)msg;
    
	assert(conn);
    assert(msg);
	
	while( totalBytesProcessed < msgLength )
    {
        unsigned long remainingBytes = msgLength - totalBytesProcessed;
        unsigned long msg_chunk_size = (max_msg_buffer_size < remainingBytes) ? max_msg_buffer_size : remainingBytes;

#ifdef _WIN32
		printf("ReqlSend::preparing to send: \n%.*s\n", msgLength, (char*)msg);
		//ret = send(conn->socket, (const char*)msg_chunk, msg_chunk_size, 0 );
		assert( conn );
		ret = CTSSLWrite(conn->socket, conn->sslContext, msg_chunk, &msg_chunk_size);
		if(ret == SOCKET_ERROR || ret == 0)
        {
			printf( "****ReqlSend::Error %d sending data to server\n",  WSAGetLastError() );
            return ret;
        }
		//printf("%d bytes of handshake data sent\n", ret);
		bytesProcessed = ret;
#elif defined(__APPLE__)
        status = SSLWrite(conn->sslContext, msg_chunk, msg_chunk_size, &bytesProcessed);
#endif
		totalBytesProcessed += bytesProcessed;
        msg_chunk += bytesProcessed;
    }
    
	//Any subsequent
    //So for now, our api will wait until the non-blocking socket begins to write to the network
    //before returning
    if( CTSocketWait(conn->socket, conn->event_queue, EVFILT_WRITE | EV_EOF) != conn->socket )
    {
        printf("something went wrong\n");
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
	CTOverlappedRequest * overlappedQuery = (CTOverlappedRequest*) ( (char*)msg + *msgLength + 1 ); //+1 for null terminator needed for encryption
	//overlappedQuery = (ReqlOverlappedQuery *)malloc( sizeof(ReqlOverlappedQuery) );
	ZeroMemory(overlappedQuery, sizeof(CTOverlappedRequest)); //this is critical for proper overlapped/iocp operation!
	overlappedQuery->Overlapped.hEvent = CreateEvent(NULL, 0, 0, NULL); //Manual vs Automatic Reset Events Affect GetQueued... Operation!!!
	overlappedQuery->buf = (char*)msg;
	overlappedQuery->len = *msgLength;
	overlappedQuery->conn = conn;
	overlappedQuery->queryToken = *(uint64_t*)msg;

	printf("overlapped->queryBuffer = %s\n", (char*)msg);

	//Post the overlapped object message asynchronously to the socket transmit thread queue using Win32 Overlapped IO and IOCP
	if( !PostQueuedCompletionStatus(conn->socketContext.txQueue, *msgLength, dwCompletionKey, &(overlappedQuery->Overlapped) ) )
	{
		printf("\nReqlAsyncSend::PostQueuedCompletionStatus failed with error:  %d\n", GetLastError());
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
			printf( "****ReqlAsyncSend::WSASend failed with error %d \n",  WSAGetLastError() );	
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
    //printf("queryBuf = \n\n%s\n\n", queryBuf);

    ReqlQueryMessageHeader * queryHeader = (ReqlQueryMessageHeader*)*queryBufPtr;
    //printf("sizeof(ReQLHeader) == %lu\n", sizeof(ReqlQueryMessageHeader));
    
    //printf("ReqlRunQueryWithToken\n\n");
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
    //printf("sendBuffer = \n\n%s\n", sendBuffer + queryHeaderLength);
    //assert(queryBuf[queryIndex%2]);
    //ReqlSend(conn, (void*)&queryHeader, queryHeaderLength);

	//queryMessageLength = queryStrLength + queryHeaderLength;
    CTSend(conn, (void*)(*queryBufPtr), queryStrLength + sizeof(ReqlQueryMessageHeader));
    //free( sendBuffer);
    //printf("ReqlRunQueryCtx End\n\n");
    
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
	//printf("overlapped->queryBuffer = %s\n", (char*)*queryBufPtr);

	CTOverlappedRequest * overlappedQuery = (CTOverlappedRequest*) ( (char*)*queryBufPtr + queryStrLength + 1 ); //+1 for null terminator needed for encryption
	//overlappedQuery = (ReqlOverlappedQuery *)malloc( sizeof(ReqlOverlappedQuery) );
	ZeroMemory(overlappedQuery, sizeof(CTOverlappedRequest)); //this is critical for proper overlapped/iocp operation!
	overlappedQuery->Overlapped.hEvent = CreateEvent(NULL, 0, 0, NULL); //Manual vs Automatic Reset Events Affect GetQueued... Operation!!!
	overlappedQuery->buf = (char*)*queryBufPtr;
	overlappedQuery->len = queryStrLength;
	overlappedQuery->conn = conn;
	overlappedQuery->queryToken = queryToken;//*(uint64_t*)msg;

	//printf("overlapped->queryBuffer = %s\n", (char*)*queryBufPtr);
	//Post the overlapped object message asynchronously to the socket transmit thread queue using Win32 Overlapped IO and IOCP
	if( !PostQueuedCompletionStatus(conn->socketContext.txQueue, queryStrLength, dwCompletionKey, &(overlappedQuery->Overlapped) ) )
	{
		printf("\nCTSendOnQueue::PostQueuedCompletionStatus failed with error:  %d\n", GetLastError());
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
			printf( "****ReqlAsyncSend::WSASend failed with error %d \n",  WSAGetLastError() );	
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


/***
 *	CTReqlRunQueryOnQueue
 *
 *  Does the same thing as CTSendOnQueue, but first populates ReQlQueryMessageHeader at the start of the outgoing network buffer
 *	Send a network buffer asynchronoulsy (using IOCP) and tag it with a request token
 ***/
uint64_t CTReQLRunQueryOnQueue(CTConnection * conn, const char ** queryBufPtr, unsigned long queryStrLength, uint64_t queryToken)//, void * options)//, ReqlQueryClosure callback)
{
    //ReqlError * errPtr = NULL;
    //ReqlError err = {ReqlDriverErrorClass, ReqlSuccess, NULL};
    
    //assert(!_writing && !_reading);
	unsigned long queryHeaderLength, /*queryStrLength,*/ queryMessageLength;
    ReqlQueryMessageHeader * queryHeader = (ReqlQueryMessageHeader*)*queryBufPtr;
    //printf("sizeof(ReQLHeader) == %lu\n", sizeof(ReqlQueryMessageHeader));
	const char* queryBuf = *queryBufPtr + sizeof(ReqlQueryMessageHeader);
	//printf("queryBuf = \n\n%s\n\n", queryBuf);
    
    //printf("ReqlRunQueryWithToken\n\n");
    //assert(*queryBuf && strlen(queryBuf) > sizeof(ReqlQueryMessageHeader));
    
    //Populate the network message buffer with a header and the serialized query JSON string
    queryHeader->token = queryToken;//(conn->queryCount)++;//conn->token;
    queryHeaderLength = sizeof(ReqlQueryMessageHeader);
    //queryStrLength = (int32_t)strlen(queryBuf);    
    queryHeader->length = queryStrLength;// + queryHeaderLength;

	//memcpy(queryBuf, &queryHeader, sizeof(ReqlQueryMessageHeader)); //copy query header
    //queryBuf[queryStrLength + queryHeaderLength] = '\0';  //cap with null char
    
	//copy the header to the buffer
    //memcpy(conn->query_buf[queryToken%2], &queryHeader, queryHeaderLength); //copy query header
    //memcpy((char*)(conn->query_buf[queryToken%2]) + queryHeaderLength, queryBuf, queryStrLength); //copy query header
    //sendBuffer[queryStrLength + queryHeaderLength] = '\0';  //cap with null char
    
	printf("ReqlRunQueryWithTokenOnQueue::queryBufPtr (%d) = %.*s", (int)queryStrLength, (int)queryStrLength, *queryBufPtr + sizeof(ReqlQueryMessageHeader));
    //_writing = 1;    
    queryMessageLength = queryHeaderLength + queryStrLength;
	//ReqlSend(conn, (void*)&queryHeader, queryHeaderLength);
	CTSendOnQueue(conn, (char**)(queryBufPtr), queryMessageLength, queryToken);
    //free( sendBuffer);
    //printf("ReqlRunQueryCtx End\n\n");
    
    //_writing = 0;
    
    return queryHeader->token;

}




//this is a client call so it should return ReqlDriverError 
//msgLength must be unsigned long to match DWORD type on WIN32
CTClientError CTAsyncRecv(CTConnection* conn, void * msg, unsigned long * msgLength)
{
	//uint64_t queryToken = *(uint64_t*)msg;
#ifdef _WIN32

	//WSAOVERLAPPED wOverlapped;
	//OVERLAPPED ol;

	//DWORD lpNumberOfBytesRecvd = *msgLength;
	//DWORD flags = 0;
	//DWORD numBuffers = 1;
	//Recv Query Asynchronously with Windows IOCP
	CTOverlappedResponse * overlappedResponse;
	unsigned long overlappedOffset = 0;// *msgLength - sizeof(ReqlOverlappedResponse);
	if (*msgLength > 0)
		overlappedOffset = *msgLength - sizeof(CTOverlappedResponse);;

	//Create an overlapped connetion object to pass what we need to the iocp callback
	//Instead of allocating memory we will assume the recv buffer is large enough and tack it on the end
	overlappedResponse = (CTOverlappedResponse*) ( (char*)msg + overlappedOffset ); //+1 for null terminator needed for encryption
	//overlappedResponse = (ReqlOverlappedResponse *)malloc( sizeof(ReqlOverlappedResponse) );
	ZeroMemory(overlappedResponse, sizeof(CTOverlappedResponse)); //this is critical!
	overlappedResponse->Overlapped.hEvent = NULL;// WSACreateEvent();//CreateEvent(NULL, 0, 0, NULL); //Manual vs Automatic Reset Events Affect GetQueued... Operation!!!
	overlappedResponse->wsaBuf.buf = (char*)msg;//(char*)(conn->response_buf[queryToken%2]);
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
	if( WSARecv(conn->socket, &(overlappedResponse->wsaBuf), 1, msgLength, &(overlappedResponse->Flags), &(overlappedResponse->Overlapped), NULL) == SOCKET_ERROR )
	{
		//WSA_IO_PENDING
		if( WSAGetLastError() != WSA_IO_PENDING )
		{
			printf( "****ReqlAsyncRecv::WSARecv failed with error %d \n",  WSAGetLastError() );	
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


//A helper function to perform a socket connection to a given service
//Will either return an kqueue fd associated with the input (blocking or non-blocking) socket
//or an error code
coroutine int CTSocketConnect(CTSocket socketfd, CTTarget * service)
{
    //printf("ReqlSocketConnect\n");
    //yield();
    struct sockaddr_in addr;
 
    //Resolve hostname asynchronously   
    /*
    // Resolve hostname synchronously
     struct hostent *host = NULL;
     if ( (host = gethostbyname(service->host)) == NULL )
    {
        printf("gethostbyname failed with err:  %d\n", errno);
        perror(service->host);
        return ReqlDomainError;
    }
    printf("ReqlSocketConnect 3\n");
    */
    
    // Fill a sockaddr_in socket host address and port
    //bzero(&addr, sizeof(addr));
	memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = service->host_addr.sin_addr.s_addr;//*(in_addr_t*)(service->host_addr.sa_data);//*(in_addr_t*)(host->h_addr);  //set host ip addr
    addr.sin_port = htons(service->port);
    
    /*
    struct sockaddr_in * addr_in = (struct sockaddr_in*)&(service->host_addr);
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
        //errno EINPROGRESS is expected for non blocking sockets
        if( errno != EINPROGRESS )
        {
            printf("socket failed to connect to %s with error: %d\n", service->host, errno);
            CTSocketClose(socketfd);
            perror(service->host);
            return CTSocketConnectError;
        }
        else
        {
			printf("\nReqlSocketConnect EINPROGRESS\n");
        }
    }
    //printf("ReqlSocketConnect End\n");
    return CTSuccess;
}



#define CT_REQL_NUM_NONCE_BYTES 20
const char * clientKeyData = "Client Key";
const char * serverKeyData = "Server Key";
const char* authKey = "\"authentication\"";
const char * successKey = "\"success\"";

    
static const char *JSON_PROTOCOL_VERSION = "{\"protocol_version\":0,\0";
static const char *JSON_AUTH_METHOD = "\"authentication_method\":\"SCRAM-SHA-256\",\0";
int CTReQLSASLHandshake(CTConnection * r, CTTarget* service)
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

	printf("ReqlSASLHandshake begin\n");
    //  Generate a random client nonce for sasl scram sha 256 required by RethinkDB 2.3
    //  for populating the SCRAM client-first-message
    //  We must wrap SecRandomCopyBytes in a function and use it one byte at a time
    //  to ensure we get a valid UTF8 String (so this is pretty slow)
    if( (status = ct_scram_gen_rand_bytes( clientNonce, CT_REQL_NUM_NONCE_BYTES )) != noErr)
    {
        printf("ct_scram_gen_rand_bytes failed with status:  %d\n", status);
        return CTSCRAMEncryptionError;
    }
    
    //  Populate SCRAM client-first-message (ie client-first-message containing username and random nonce)
    //  We choose to forgo creating a dedicated buffer for the client-first-message and populate
    //  directly into the SCRAM AuthMessage Accumulation buffer
    sprintf(SCRAM_AUTH_MESSAGE, "n=%s,r=%.*s", service->user, (int)CT_REQL_NUM_NONCE_BYTES, clientNonce);
    AuthMessageLength += (unsigned long)strlen(SCRAM_AUTH_MESSAGE);
    //printf("CLIENT_FIRST_MESSAGE = \n\n%s\n\n", SCRAM_AUTH_MESSAGE);
    
    //  Populate client-first-message in JSON wrapper expected by RethinkDB ReQL Service
    //static const char *JSON_PROTOCOL_VERSION = "{\"protocol_version\":0,\0";
    //static const char *JSON_AUTH_METHOD = "\"authentication_method\":\"SCRAM-SHA-256\",\0";
    strcat( CLIENT_FIRST_MESSAGE_JSON, JSON_PROTOCOL_VERSION);//, strlen(JSON_PROTOCOL_VERSION));
    strcat( CLIENT_FIRST_MESSAGE_JSON, JSON_AUTH_METHOD);//, strlen(JSON_PROTOCOL_VERSION));
    sprintf(CLIENT_FIRST_MESSAGE_JSON + strlen(CLIENT_FIRST_MESSAGE_JSON), "\"authentication\":\"n,,%s\"}", SCRAM_AUTH_MESSAGE);
    //printf("CLIENT_FIRST_MESSAGE_JSON = \n\n%s\n\n", CLIENT_FIRST_MESSAGE_JSON);
    
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
    CTSend(r, CLIENT_FIRST_MESSAGE_JSON, strlen(CLIENT_FIRST_MESSAGE_JSON)+1);  //Note:  Raw JSON messages sent to ReQL Server always needs the extra null character to determine EOF!!!
                                                                                  //       However, Reql Query Messages must NOT contain the additional null character
    //yield();
    
    //  Synchronously Wait for magic number response and SCRAM server-first-message
    //  --For now we just care about getting 'success: true' from the magic number response
    //  --The server-first-message contains salt, iteration count and a server nonce appended to our nonce for use in SCRAM HMAC SHA-256 Auth
    mnResponsePtr = (char*)CTRecv(r, MAGIC_NUMBER_RESPONSE_JSON, &magicNumberResponseLength);
    sFirstMessagePtr = (char*)CTRecv(r, SERVER_FIRST_MESSAGE_JSON, &readLength);
    printf("MAGIC_NUMBER_RESPONSE_JSON = \n\n%s\n\n", mnResponsePtr);
	printf("SERVER_FIRST_MESSAGE_JSON = \n\n%s\n\n", sFirstMessagePtr);
    
	if (!mnResponsePtr || !sFirstMessagePtr)
		return CTSASLHandshakeError;
    //  Parse the magic number json response buffer to see if magic number exchange was successful
    //  Note:  Here we are counting on the fact that rdb server json does not send whitespace!!!
    //bool magicNumberSuccess = false;
    //const char * successKey = "\"success\"";
    //char * successVal;
    if( (successVal = strstr(mnResponsePtr, successKey)) != NULL )
    {
        successVal += strlen(successKey) + 1;
        //if( *successVal == ':') successVal += 1;
        if( *successVal == '"') successVal+=1;
        if( strncmp( successVal, "true", MIN(4, strlen(successVal) ) ) == 0 )
            magicNumberSuccess = true;
    }
    if( !magicNumberSuccess )
    {
        printf("RDB Magic Number Response Failure!\n");
        return CTSASLHandshakeError;
    }
    
    //  Parse the server-first-message json response buffer to see if client-first-message exchange was successful
    //  Note:  Here we are counting on the fact that rdb server json does not send whitespace!!!
    //bool sfmSuccess = false;
    if( (successVal = strstr(sFirstMessagePtr, successKey)) != NULL )
    {
        successVal += strlen(successKey) + 1;
        //if( *successVal == ':') successVal += 1;
        if( *successVal == '"') successVal+=1;
        if( strncmp( successVal, "true", MIN(4, strlen(successVal) ) ) == 0 )
            sfmSuccess = true;
    }
    if( !sfmSuccess )//|| !authValue || authLength < 1 )
    {
        printf("RDB server-first-message response failure!\n");
        return CTSASLHandshakeError;
    }
    
    //  The nonce, salt, and iteration count (ie server-first-message) we need for SCRAM Auth are in the 'authentication' field
    //const char* authKey = "\"authentication\"";
    authValue = strstr(sFirstMessagePtr, authKey);
    //size_t authLength = 0;
    if( authValue )
    {
        authValue += strlen(authKey) + 1;
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
    //printf("serverNonce = %s\n", serverNonce);
    //printf("base64Salt = %s\n", base64Salt);
    //printf("saltIterations = %d\n", atoi(saltIterations));
    //printf("password = %s\n", service->password);
    
    //  While the nonce(s) is (are) already in ASCII encoding sans , and " characters,
    //  the salt is base64 encoded, so we have to decode it to UTF8
   // size_t saltLength = 0;
    salt = (char*)cr_base64_to_utf8(base64Salt, strlen(base64Salt), &saltLength );
   // assert( salt && saltLength > 0 );
	
    //  Run iterative hmac algorithm n times and concatenate the result in an XOR buffer in order to salt the password
    //      SaltedPassword  := Hi(Normalize(password), salt, i)
    //  Note:  Not really any way to tell if ct_scram_... fails
	memset(saltedPassword, 0, CC_SHA256_DIGEST_LENGTH);
    ct_scram_salt_password( service->password, strlen(service->password), salt, saltLength, atoi(saltIterations), saltedPassword );
    //printf("salted password = %.*s\n", CC_SHA256_DIGEST_LENGTH, saltedPassword);
    
    //  Generate Client and Server Key buffers
    //  by encrypting the verbatim strings "Client Key" and "Server Key, respectively,
    //  using keyed HMAC SHA 256 with the salted password as the secret key
    //      ClientKey       := HMAC(SaltedPassword, "Client Key")
    //      ServerKey       := HMAC(SaltedPassword, "Server Key")
    ct_scram_hmac(saltedPassword, CC_SHA256_DIGEST_LENGTH, clientKeyData, strlen(clientKeyData), clientKey );
    ct_scram_hmac(saltedPassword, CC_SHA256_DIGEST_LENGTH, serverKeyData, strlen(serverKeyData), serverKey );
    //printf("clientKey = %.*s\n", CC_SHA256_DIGEST_LENGTH, clientKey);
    //printf("serverKey = %.*s\n", CC_SHA256_DIGEST_LENGTH, serverKey);
    
    //  Calculate the "Normal" SHA 256 Hash of the clientKey
    //      storedKey := H(clientKey)
    ct_scram_hash(clientKey, CC_SHA256_DIGEST_LENGTH, storedKey);
    //printf("storedKey = %.*s\n", CC_SHA256_DIGEST_LENGTH, storedKey);
    
    //  Populate SCRAM client-final-message (ie channel binding and random nonce WITHOUT client proof)
    //  E.g.  "authentication": "c=biws,r=rOprNGfwEbeRWgbNEkqO%hvYDpWUa2RaTCAfuxFIlj)hNlF$k0,p=dHzbZapWIk4jUhN+Ute9ytag9zjfMHgsqmmiz7AndVQ="
    sprintf(CLIENT_FINAL_MESSAGE, "c=biws,r=%s", serverNonce);
    //printf("CLIENT_FINAL_MESSAGE = \n\n%s\n\n", CLIENT_FINAL_MESSAGE);
    
    //Copy the client-final-message (without client proof) to the AuthMessage buffer
    strncat( SCRAM_AUTH_MESSAGE, CLIENT_FINAL_MESSAGE, strlen(CLIENT_FINAL_MESSAGE));
    AuthMessageLength += (unsigned long)strlen(CLIENT_FINAL_MESSAGE);
    //printf("SCRAM_AUTH_MESSAGE = \n\n%s\n\n", SCRAM_AUTH_MESSAGE);
    
    //Now that we have the complete Auth Message we can use it in the SCRAM procedure
    //to calculate client and server signatures
    // ClientSignature := HMAC(StoredKey, AuthMessage)
    // ServerSignature := HMAC(ServerKey, AuthMessage)
    ct_scram_hmac(storedKey, CC_SHA256_DIGEST_LENGTH, SCRAM_AUTH_MESSAGE, AuthMessageLength, clientSignature );
    ct_scram_hmac(serverKey, CC_SHA256_DIGEST_LENGTH, SCRAM_AUTH_MESSAGE, AuthMessageLength, serverSignature );
    
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
    //printf("Base64 Server Signature:  %s\n", base64SS);

    //Add the client proof to the end of the client-final-message
    sprintf(CLIENT_FINAL_MESSAGE_JSON, "{\"authentication\":\"%s,p=%.*s\"}", CLIENT_FINAL_MESSAGE, (int)base64ProofLength, (char*)base64Proof);
    //printf("CLIENT_FINAL_MESSAGE_JSON = \n\n%s", CLIENT_FINAL_MESSAGE_JSON);
    
    //Send the client-final-message wrapped in json
	//CLIENT_FINAL_MESSAGE_JSON[strlen(CLIENT_FINAL_MESSAGE_JSON)] = '\0';
    CTSend(r, CLIENT_FINAL_MESSAGE_JSON, strlen(CLIENT_FINAL_MESSAGE_JSON)+1);  //Note:  JSON always needs the extra null character to determine EOF!!!
    
    //yield();

    //Wait for SERVER_FINAL_MESSAGE
    readLength = 1024;
    sFinalMessagePtr = (char*)CTRecv(r, SERVER_FINAL_MESSAGE_JSON, &readLength);
    printf("SERVER_FINAL_MESSAGE_JSON = \n\n%s\n", sFinalMessagePtr);
    
    //Validate Server Signature
    authLength = 0;
    if( (authValue = strstr(sFinalMessagePtr, authKey)) != NULL )
    {
        authValue += strlen(authKey) + 1;
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
        printf("Failed to retrieve server signature from SCRAM server-final-message.\n");
        return CTSASLHandshakeError;
    }
    
    //  Compare server sent signature to client generated server signature
    if( strncmp((char*)base64SS,  authValue, authLength) != 0)
    {
        printf("Failed to authenticate server signature: %s\n", authValue);
        return CTSASLHandshakeError;
    }
    
    //free Base64 <-> UTF8 string conversion allocations
    free( salt );
    free( base64Proof );
    free( base64SS );
    
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
#if defined(CTRANSPORT_USE_MBED_TLS) || defined(__APPLE__)
	if (caPath)
	{
		if ((rootCertRef = CTSSLCertificate(caPath)) == NULL)
		{
			printf("ReqlSSLCertificate failed to generate SecCertificateRef\n");
			CTCloseSSLSocket(conn->sslContext, conn->socket);
			return CTSSLCertificateError;
		}
	}
#endif
	//ReqlSSLInit();
	
	printf("----- Credentials Initialized\n");

	//ReqlSSLContextRef sslContext = NULL;
	//  Create an SSL/TLS Context using a 3rd Party Framework such as Apple SecureTransport, OpenSSL or mbedTLS
	if( (conn->sslContext = CTSSLContextCreate(&(conn->socket), &rootCertRef, hostname)) == NULL )
	{
		//ReqlSSLCertificateDestroy(&rootCertRef);
        return CTSecureTransportError;
	}	
	printf("----- SSL Context Initialized\n");
#ifdef __APPLE__   
    
    //7.#  Elevate Trust Settings on the CA (Not on iOS, mon!)
    //const void *ctq_keys[] =    { kSecTrustSettingsResult };
    //const void *ctq_values[] =  { kSecTrustSettingsResultTrustRoot };
    //CFDictionaryRef newTrustSettings = CFDictionaryCreate(kCFAllocatorDefault, ctq_keys, ctq_values, 3, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    
    //status = SecTrustSettingsSetTrustSettings(rootCert, kSecTrustSettingsDomainAdmin, newTrustSettings);
    //if (status != errSecSuccess) {
    //    printf("Could not change the admin trust setting for a certificate. Error: %d", status);
    //    return -1;
    //}
    
    //  Add the CA Root Certificate to the device keychain
    //  (thus letting the system determine for us that it is a valid certificate in a SecCertificateRef, but not SecIdentityRef)
    if( (status = ReqlSaveCertToKeychain( rootCertRef )) != noErr )
    {
        printf("Failed to save root ca cert to device keychain!\n");
		CTCloseSSLSocket(conn->sslContext, conn->socket);
        CFRelease(rootCertRef);
        return CTSSLKeychainError;
    }
    //Note:  P12 stuff used to be sandwiched here
    //SecIdentityRef certIdentity = ReqlSSLCertficateIdentity();
#endif 

	
    //  Perform the SSL Layer Auth Handshake over TLS
	if( (ret = CTSSLHandshake(conn->socket, conn->sslContext, rootCertRef, hostname)) != noErr )
	//if( (status = ReqlSSLHandshake(conn, rootCert)) != noErr )
    {
		//ReqlSSLCertificateDestroy(&rootCertRef);
        printf("ReqlSLLHandshakeFailed with status:  %d\n", ret);
        CTCloseSSLSocket(conn->sslContext, conn->socket);
        return CTSSLHandshakeError;
    }

	if ( (ret = CTVerifyServerCertificate(conn->sslContext, hostname)) != noErr)
	{
		//ReqlSSLCertificateDestroy(&rootCertRef);
        printf("ReqlVerifyServerCertificate with status:  %d\n", ret);
        CTCloseSSLSocket(conn->sslContext, conn->socket);
        return CTSSLHandshakeError;
	}
	

	//mbedtls_ssl_flush_output(&(conn->sslContext->ctx));

	//printf("conn->Socket = %d\n", conn->socket);
	//printf("conn->SocketContext.socket = %d\n", conn->socketContext.socket);
	//printf("conn->SocketContext.ctx.fd = %d\n", conn->socketContext.ctx.fd);

	//printf("conn->sslContext = %llu\n", conn->sslContext);

	/*
	//char REQL_MAGIC_NUMBER_BUF[5] = "\xc3\xbd\xc2\x34";
	size_t bytesToWrite = 4;
	int errOrBytesWritten = mbedtls_ssl_write(&(conn->sslContext->ctx), (const unsigned char *)REQL_MAGIC_NUMBER_BUF, bytesToWrite);
	if (errOrBytesWritten < 0) {
		if (errOrBytesWritten != MBEDTLS_ERR_SSL_WANT_READ &&
			errOrBytesWritten != MBEDTLS_ERR_SSL_WANT_WRITE) {

			printf("ReqlSSLWrite::mbedtls_ssl_write = %d\n", errOrBytesWritten);

		}
}
	printf("ReqlSSLWrite::erroOrBytesWritten = %d\n", errOrBytesWritten);
	//printf("ReqlSSLWrite::erroOrBytesWritten = %d\n", errOrBytesWritten);

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
	printf(TEXT("CTPageSize() = %d.\n"), systemInfo.dwPageSize);
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

	conn.socketContext.host = service->host;
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
        printf("Failed to create run loop observer!\n");
        error.id = ReqlRunLoopError;
        goto CONN_CALLBACK;
    }

    //  Install a repeating run loop observer so we can call coroutine API yield
    CFRunLoopAddObserver(runLoop, runLoopObserver, kCFRunLoopDefaultMode);
    */

#ifdef _WIN32//CTRANSPORT_USE_MBED_TLS
	printf("Before CTSocketCreate()\n");
    //  Create a [non-blocking] TCP socket and return a socket fd
    if( (conn.socket = CTSocketCreate()) < 0)
    {
        error.id = (int)conn.socket; //Note: hope that 64 bit socket handle is within 32 bit range?!
        goto CONN_CALLBACK;
        //return conn.socket;
    }

		printf("Before CTSocketCreateEventQueue(0)\n");

	if( CTSocketCreateEventQueue(&(conn.socketContext)) < 0)
	{
		printf("ReqlSocketCreateEventQueue failed\n");
		error.id = (int)conn.event_queue;
        goto CONN_CALLBACK;
	}

			printf("Before ReqlSocketConnect\n");

    //  Connect the [non-blocking] socket to the server asynchronously and return a kqueue fd
    //  associated with the socket so we can listen/wait for the connection to complete
    if( (conn.event_queue = CTSocketConnect(conn.socket, service)) < 0 )
    {
        error.id = (int)conn.socket;
        goto CONN_CALLBACK;
        //return conn.event_queue;
    }
	//printf("ReqlSocketConnect End\n");
#else
	itoa((int)service->port, port, 10);
	mbedtls_net_init(&(conn.socketContext.ctx));
	if ( (ret = mbedtls_net_connect(&(conn.socketContext.ctx), service->host, port, MBEDTLS_NET_PROTO_TCP)) != 0)
	{
		Reql_printf("ReqlConnectRoutine::mbedtls_net_connect failed and returned %d\n\n", ret);
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

	printf("Before ReqlSocketWait\n");

    //Wait for socket connection to complete
    if( CTSocketWait(conn.socket, conn.event_queue, EVFILT_WRITE) != conn.socket )
    {
        printf("ReqlSocketWait failed\n");
        error.id = CTSocketEventError;
        CTSocketClose(conn.socket);
        goto CONN_CALLBACK;
        //return ReqlEventError;
    }

		printf("Before ReqlGetSocketError\n");

    //Check for socket error to ensure that the connection succeeded
    if( (error.id = CTSocketGetError(conn.socket)) != 0 )
    {
        // connection failed; error code is in 'result'
        printf("socket async connect failed with result: %d", error.id );
        CTSocketClose(conn.socket);
        error.id = CTSocketConnectError;
        goto CONN_CALLBACK;
        //return ReqlConnectError;
    }
    
    //There is some short order work that would be good to parallelize regarding loading the cert and saving it to keychain
    //However, the SSLHandshake loop has already been optimized with yields for every asynchronous call

	printf("Before ReqlSSLRoutine Host = %s\n", conn.socketContext.host);
	printf("Before ReqlSSLRoutine Host = %s\n", service->host);

	
    if( (status = CTSSLRoutine(&conn, conn.socketContext.host, service->ssl.ca)) != 0 )
    {
        printf("ReqlSSL failed with error: %d\n", (int)status );
        error.id = CTSSLHandshakeError;
        goto CONN_CALLBACK;
        //return status;
    }
	
    //The connection completed successfully, allocate memory to return to the client
    error.id = CTSuccess;
    
    //generate a client side token for the connection
    //conn.token = ++REQL_NUM_CONNECTIONS;
    //conn.query_buf[0] = (void*)malloc( REQL_QUERY_BUFF_SIZE);
    //conn.query_buf[1] = (void*)malloc( REQL_QUERY_BUFF_SIZE);
    //conn.response_buf[0] = (void*)malloc( REQL_RESPONSE_BUFF_SIZE);
    //conn.response_buf[1] = (void*)malloc( REQL_RESPONSE_BUFF_SIZE);
    
CONN_CALLBACK:
    //printf("before callback!\n");

	conn.target = service;
    callback(&error, &conn);
    //printf("After callback!\n");

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



/***
 *  CTTargetResolveHost
 *
 *  DNS Lookups can take a long time on iOS, so we need an async DNS resolve mechanism.
 *  However, getaddrinfo_a is not available for Darwin, so we use CFHost API to schedule
 *  the result of the async DNS lookup on internal CFNetwork thread which will post our result
 *  callback to the calling runloop
 *
 ***/
coroutine int CTTargetResolveHost(CTTarget * target, CTConnectionClosure callback)
{
 
#ifndef _WIN32
    //Launch asynchronous query using modified version of Sustrik's libdill DNS implementation
    struct dill_ipaddr addr[1];
    struct dns_addrinfo *ai = NULL;
    if( dill_ipaddr_dns_query_ai(&ai, addr, 1, service->host, service->port, DILL_IPADDR_IPV4, service->dns.resconf, service->dns.nssconf) != 0 || ai == NULL )
    {
        printf("service->dns.resconf = %s", service->dns.resconf);
        printf("dill_ipaddr_dns_query_ai failed with errno:  %d", errno);
        ReqlError err = {ReqlDriverErrorClass, ReqlDNSError, NULL};
        callback(&err, NULL);
        return ReqlDNSError;
    }
    
    //TO DO:  We could use this opportunity to do work that validates ReqlService properties, like loading SSL Certificate
    //CFRunLoopWakeUp(CFRunLoopGetCurrent());
    //CFRunLoopSourceSignal(runLoopSource);
    CFRunLoopWakeUp(CFRunLoopGetCurrent());

    //printf("Before DNS wait\n");
    yield();

    int numResolvedAddresses = 0;
    if( (numResolvedAddresses = dill_ipaddr_dns_query_wait_ai(ai, addr, 1, service->port, DILL_IPADDR_IPV4, -1)) < 1 )
    {
        //printf("dill_ipaddr_dns_query_wait_ai failed to resolve any IPV4 addresses!");
        ReqlError err = {ReqlDriverErrorClass, ReqlDNSError, NULL};
        callback(&err, NULL);
        return ReqlDNSError;
    }
 
    //printf("DNS resolved %d ip address(es)\n", numResolvedAddresses);
    struct sockaddr_in * ptr = (struct sockaddr_in*)addr;//(addr[0]);
    service->host_addr = *ptr;//(struct sockaddr_in*)addresses;
#else

//TODO:
	//--------------------------------------------------------------------
//  ConnectAuthSocket establishes an authenticated socket connection 
//  with a server and initializes needed security package resources.

	DWORD dwRetval;
	char port[6];
	struct addrinfo *result = NULL;
    struct addrinfo *ptr = NULL;
    struct addrinfo hints;
	int i = 0;

    struct sockaddr_in  *sockaddr_ipv4;
    //unsigned long  ulAddress;
	struct hostent *pHost = NULL;
    //sockaddr_in    sin;

    //--------------------------------------------------------------------
    //  Lookup the server's address.
		printf("ReqlServiceResolveHost host = %s\n", target->host);

    target->host_addr.sin_addr.s_addr = inet_addr (target->host);
    if (INADDR_NONE == target->host_addr.sin_addr.s_addr ) 
    {	
		
		/*
		pHost = gethostbyname (service->host);
		if ( pHost==NULL) {
			printf("gethostbyname failed\n");
			//WSACleanup();
			return -1;
		}
		memcpy((void*)&(service->host_addr.sin_addr.s_addr) , (void*)(pHost->h_addr), (size_t)(strlen(pHost->h_addr)) );		 

		//memcpy((void*)&(service->addr) , (void*)*(pHost->h_addr_list), (size_t)(pHost->h_length));		 
		*/
		
		//set port param
		_itoa((int)target->port, port, 10);
		//
		memset(&hints, 0, sizeof(struct addrinfo));
		dwRetval = getaddrinfo(target->host, port, &hints, &result);
		if ( dwRetval != 0 ) {
			printf("getaddrinfo failed with error: %d\n", dwRetval);
			//WSACleanup();
			return -1;
		}
		
		 // Retrieve each address and print out the hex bytes
		for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) 
		{



			printf("getaddrinfo response %d\n", i++);
			printf("\tFlags: 0x%x\n", ptr->ai_flags);
			printf("\tFamily: ");
			printf("\tLength of this sockaddr: %d\n", (int)(ptr->ai_addrlen));
			printf("\tCanonical name: %s\n", ptr->ai_canonname);
		
			if( ptr->ai_family != AF_INET )
			{
				continue;
			}
			else
			{
				sockaddr_ipv4 = (struct sockaddr_in*)result->ai_addr;
					printf("ReqlServiceResolveHost host before copy = %s\n", target->host);

				memcpy(&(target->host_addr) , sockaddr_ipv4, result->ai_addrlen);		 
						printf("ReqlServiceResolveHost host after copy = %s\n", target->host);

				break;
			}
		
		 }
	    freeaddrinfo(result);
		

	}
	
#endif    
    //Perform the socket connection
	printf("ReqlServiceResolveHost host = %s\n", target->host);
    CTConnectRoutine(target, callback);

    /*
    //Remove observer that yields to coroutines
    CFRunLoopRemoveObserver(CFRunLoopGetCurrent(), runLoopObserver, kCFRunLoopDefaultMode);
    //Remove dummy source that keeps run loop alive
    CFRunLoopRemoveSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);
    
    //release runloop object memory
    CFRelease(runLoopObserver);
    CFRelease(runLoopSource);
    */
    
    return CTSuccess;
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
    //  Initialize the socket and the SSP security package.
	//CTSSLInit();

	//-------------------------------------------------------------------
    //  Initialize the scram SSP security package.
	ct_scram_init();
	
    //  Issue a request to resolve Host DNS to a sockaddr asynchonrously using CFHost API
    //  It will perform the DNS request asynchronously on a bg thread internally maintained by CFHost API
    //  and return the result via callback to the current run loop
    //go(ReqlServiceResolveHost(service, callback));
	CTTargetResolveHost(target, callback);  
    return CTSuccess;
}


//A helpef function to close an SSL Context and a socket in one shot
int CTCloseSSLSocket(CTSSLContextRef sslContextRef, CTSocket socketfd)
{
	//CTSSLContextDestroy(sslContextRef);
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
	/*
	if( conn->query_buf[0] )
		free(conn->query_buf[0]);
	if( conn->query_buf[1] )
		free(conn->query_buf[1]);
	if( conn->response_buf[0] )
		free(conn->response_buf[0]);
	if( conn->response_buf[1] )
		free(conn->response_buf[1]);
	*/
	
    return CTCloseSSLSocket(conn->sslContext, conn->socket);
}
