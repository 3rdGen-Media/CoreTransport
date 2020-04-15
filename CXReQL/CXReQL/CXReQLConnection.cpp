#include "../CXReQL.h"


//#include "CXReQLConnection.h"

using namespace CoreTransport;

void CXReQLConnection::reserveConnectionMemory()
{
	// Reserve pages in the virtual address space of the process.	
	unsigned long pageSize = CTPageSize();

	//Calculate query (send) buffer size based on system page size
	CX_REQL_QUERY_BUFF_SIZE = (CX_REQL_QUERY_BUFF_PAGES * pageSize);

	//Calculate response (receive) buffer size based on system page size
	CX_REQL_RESPONSE_BUFF_SIZE = (CX_REQL_RESPONSE_BUFF_PAGES * pageSize);

#ifdef _DEBUG
	printf("CX_ReQLConnection::reserveConnectionMemory CX_REQL_QUERY_BUFF_SIZE = %d\n", (int)CX_REQL_RESPONSE_BUFF_SIZE);
	printf("CX_ReQLConnection::reserveConnectionMemory CX_REQL_RESPONSE_BUFF_SIZE = %d\n", (int)CX_REQL_RESPONSE_BUFF_SIZE);
#endif

#ifdef _WIN32
	//recv
	LPVOID queryPtr, responsePtr;
	
	//(Allocate Query Buffer Memory) First Reserve, then commit
	queryPtr = VirtualAlloc(NULL, CX_REQL_MAX_INFLIGHT_QUERIES * CX_REQL_QUERY_BUFF_SIZE, MEM_RESERVE, PAGE_READWRITE);       // Protection = no access
	_conn.query_buffers = (char*)VirtualAlloc(queryPtr, CX_REQL_MAX_INFLIGHT_QUERIES*CX_REQL_QUERY_BUFF_SIZE, MEM_COMMIT, PAGE_READWRITE);       // Protection = no access

	ZeroMemory(_conn.query_buffers, 0, CX_REQL_MAX_INFLIGHT_QUERIES * CX_REQL_QUERY_BUFF_SIZE);

	//(Allocate Response Buffer Memory) First Reserve, then commit
	responsePtr = VirtualAlloc(NULL, CX_REQL_MAX_INFLIGHT_QUERIES * CX_REQL_RESPONSE_BUFF_SIZE, MEM_RESERVE, PAGE_READWRITE);       // Protection = no access
	_conn.response_buffers = (char*)VirtualAlloc(responsePtr, CX_REQL_MAX_INFLIGHT_QUERIES*CX_REQL_RESPONSE_BUFF_SIZE, MEM_COMMIT, PAGE_READWRITE);       // Protection = no access

	ZeroMemory(_conn.response_buffers, 0, CX_REQL_MAX_INFLIGHT_QUERIES * CX_REQL_RESPONSE_BUFF_SIZE);
	//memset(_conn.response_buffers, 0, CX_REQL_MAX_INFLIGHT_QUERIES * CX_REQL_RESPONSE_BUFF_SIZE );
#elif defined(__APPLE__)
	//Reserve memory for query (transmit) buffers
	_conn.query_buffers = (char*)malloc(CX_REQL_MAX_INFLIGHT_QUERIES * CX_REQL_QUERY_BUFF_SIZE);
	_conn.response_buffers = (char*)malloc(CX_REQL_MAX_INFLIGHT_QUERIES * CX_REQL_RESPONSE_BUFF_SIZE);
	memset(_conn.query_buffers, 0, CX_REQL_MAX_INFLIGHT_QUERIES * CX_REQL_QUERY_BUFF_SIZE);
	memset(_conn.response_buffers, 0, CX_REQL_MAX_INFLIGHT_QUERIES * CX_REQL_RESPONSE_BUFF_SIZE);
#endif

#ifdef _DEBUG
	assert(_conn.query_buffers);
	assert(_conn.response_buffers);
#endif
}

CXReQLConnection::CXReQLConnection(CTConnection *conn, CTThreadQueue rxQueue, CTThreadQueue txQueue)
{
	//copy the from CTConnection ReqlC memory to CXReQLConnection managed memory (because it will go out of scope)
	memcpy(&_conn, conn, sizeof(CTConnection));

	//Reserve/allocate memory for the connection's query and response buffers 
	reserveConnectionMemory();

	//create a dispatch source / handler for the CTConnection's socket
	if (!rxQueue)
		_rxQueue = _conn.socketContext.rxQueue;
	else 
		_rxQueue = rxQueue;
	
	if(!txQueue)
		_txQueue = _conn.socketContext.txQueue;
	else
		_txQueue = txQueue;

	startConnectionThreadQueues(_rxQueue, _txQueue);
	
}

CXReQLConnection::~CXReQLConnection()
{
	close();
}

void CXReQLConnection::close()
{
	CTCloseConnection(&_conn);
}

CTThreadQueue CXReQLConnection::queryQueue()
{
	return _txQueue;
}

CTThreadQueue CXReQLConnection::responseQueue()
{
	return _rxQueue;
}

void CXReQLConnection::distributeMessageWithCursor(char * msg, unsigned long msgLength)
{
	
		ReqlQueryMessageHeader * header = (ReqlQueryMessageHeader*)msg;
		printf("CXReQLConnection::distributeMessageWithCursor::Header Query Token = %llu\n", header->token);
		printf("CXReQLConnection::distributeMessageWithCursor::Header Query Length = %d\n", header->length);

		//Create a ReqlCursor (assume CXReQLConnection internal CTConnection was used
		struct ReqlCursor cursor = {header, header->length + sizeof(ReqlQueryMessageHeader), &_conn};
		//Wrap the ReqlCursor in a CXReQLCursor object
		CXReQLCursor cxCursor( cursor );

		int responseType = (int)ReqlCursorGetResponseType(&cursor);
		auto callback = getQueryCallbackForKey(header->token);

		if( callback )
		{
			//remove any callbacks that have been completed
			printf("CXReQLConnection::distributeMessageWithCursor::Query Response Type = %d\n", responseType);
			if( responseType == REQL_CLIENT_ERROR )
			{
				printf("CXReQLConnection::distributeMessageWithCursor::REQL_CLIENT_ERROR\n");
				printf("CXReQLConnection::distributeMessageWithCursor::Removing callback for query token: %d\n", (int)cursor.header->token);
				removeQueryCallbackForKey(cursor.header->token);
			}
			else if( responseType == REQL_SUCCESS_PARTIAL )
			{
				//get the run options from the query

				cxCursor.continueQuery(NULL);
			}
			else
			{
				printf("CXReQLConnection::distributeMessageWithCursor::Removing callback for query token: %d\n", (int)cursor.header->token);
				removeQueryCallbackForKey(cursor.header->token);
			}

			callback( NULL, &cxCursor);

		}
		else
		{
			if( responseType == REQL_SUCCESS_PARTIAL )
			{
				printf("CXReQLConnection::distributeMessageWithCursor::Sending cursor stop query: %d\n", (int)cursor.header->token);
				ReqlStopQuery(cursor.conn, cursor.header->token, NULL);
			}
			printf("CXReQLConnection::distributeMessageWithCursor::No Callback!");

			printQueries();

		}
}

/*****************************************************************************/
static void PrintText(DWORD length, PBYTE buffer) // handle unprintable charaters
{
	int i; //

	printf("\n"); // "length = %d bytes \n", length);
	for (i = 0; i < (int)length; i++)
	{
		if (buffer[i] == 10 || buffer[i] == 13)
			printf("%c", (char)buffer[i]);
		else if (buffer[i] < 32 || buffer[i] > 126 || buffer[i] == '%')
			continue;//printf("%c", '.');
		else
			printf("%c", (char)buffer[i]);
	}
	printf("\n\n");
}

static unsigned long __stdcall CXReQLDecryptResponseCallback(LPVOID lpParameter)
{
	int i;
	ULONG CompletionKey;

	OVERLAPPED_ENTRY ovl_entry[CX_REQL_MAX_INFLIGHT_QUERIES];
	ULONG nPaquetsDequeued;
	
	ReqlQueryMessageHeader * header = NULL;
	ReqlOverlappedResponse * overlappedResponse = NULL;

	unsigned long NumBytesRecv = 0;

	CTSSLStatus scRet = 0;
	CTClientError reqlError = CTSuccess;

	CXReQLConnection * cxConn = (CXReQLConnection*)lpParameter;
	HANDLE hCompletionPort = cxConn->responseQueue();// (HANDLE)lpParameter;

	printf("CXReQLConnection::DecryptResponseCallback Begin\n");

	//TO DO:  Optimize number of packets dequeued at once
	while (GetQueuedCompletionStatusEx(hCompletionPort, ovl_entry, CX_REQL_MAX_INFLIGHT_QUERIES, &nPaquetsDequeued, INFINITE, TRUE))
	{
		printf("CXReQLConnection::ReqlDecryptResponseCallback::nPaquetsDequeued = %d\n", (int)nPaquetsDequeued);
		for (i = 0; i < nPaquetsDequeued; i++)
		{
			//When to use CompletionKey?
			//CompletionKey = *(ovl_entry[i].lpCompletionKey);
			NumBytesRecv = ovl_entry[i].dwNumberOfBytesTransferred;
			overlappedResponse = (ReqlOverlappedResponse*)ovl_entry[i].lpOverlapped;

			if (!overlappedResponse)
			{
				printf("CXReQLConnection::DecryptResponseCallback::GetQueuedCompletionStatus continue\n");
				continue;
			}

			//TO DO:  if using SCHANNEL non-zero read path
			//what does this conditional mean in that case?
			if (NumBytesRecv == 0) //
			{
				//NOTE:  In the examples, NumBytesRecv == 0 indicates that the server disconnected
				//However, if we intentionally in order to "wake up" the the iocp notification
				//when data is available on the socket rather than after it has already been read
				//this is NOT the case
				
				unsigned long numBytesToRecv = CX_REQL_RESPONSE_BUFF_SIZE;

				//Use the ReQLC API to synchronously
				//Issue a blocking call to read and decrypt from the socket using CoReQL's internal SSL platform provider
				printf("CXReQLConnection::DecryptResponseCallback::Starting ReqlSSLRead blocking socket read + decrypt...\n");
				NumBytesRecv = CTSSLRead(overlappedResponse->conn->socket, overlappedResponse->conn->sslContext, (void*)overlappedResponse, &numBytesToRecv);

				if (NumBytesRecv > 0)
				{

					printf("CXReQLConnection::DecryptResponseCallback::ReqlSSLRead NumBytesRecv = %d\n", (int)NumBytesRecv);

					//PrintText(NumBytesRecv, (char*)overlappedResponse);
					printf("ReqlDecryptRespnonseCallback::ReqlSSLRead Buffer = \n\n%s\n\n", (char*)overlappedResponse + sizeof(ReqlQueryMessageHeader));
					cxConn->distributeMessageWithCursor(  (char*)overlappedResponse, NumBytesRecv);
				}
				else
				{
					printf("CXReQLConnection::DecryptResponseCallback::ReqlSSLRead failed with error (%d)\n", (int)NumBytesRecv);
				}

			}
			else //IOCP already read the data from the socket into the buffer we provided to Overlapped, we have to decrypt it
			{
				//printf("\nQuery Response:\n\n%s", responseMsgPtr );
				printf("CXReQLConnection::DecryptResponseCallback B::NumBytesRecv = %d\n", (int)NumBytesRecv);

				//Get the connection
#ifdef _DEBUG
				assert(overlappedResponse->conn);
#endif
				//Issue a blocking call to decrypt the message (again, only if using non-zero read path, which is exclusively used with MBEDTLS at current)
#ifndef CTRANSPORT_USE_MBED_TLS
				scRet = CTSSLDecryptMessage(overlappedResponse->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);
				//printf("Decrypted data: %d bytes", NumBytesRecv); PrintText(NumBytesRecv, (PBYTE)(overlappedResponse->wsaBuf.buf + overlappedResponse->conn->sslContext->Sizes.cbHeader));
				cxConn->distributeMessageWithCursor(  overlappedResponse->wsaBuf.buf + overlappedResponse->conn->sslContext->Sizes.cbHeader, NumBytesRecv);
#endif
				//TO DO:: check return value of ReqlSSLDecryptMessage
				//return scRet;
				//if( scRet == ReqlSSLContextExpired ) break; // Server signaled end of session
				//if( scRet == ReqlSuccess ) break;

				//TO DO:  notify the caller of the query using the C++ equivalent of blocks

				//TO DO:  Enable SSL negotation.  Determine when to appropriately do so...here?
				// The server wants to perform another handshake sequence.
				/*
				if(scRet == ReqlSSLRenegotiate)
				{
					printf("ReqlDecryptResponseCallback::Server requested renegotiate!\n");
					//scRet = ClientHandshakeLoop( Socket, phCreds, phContext, FALSE, &ExtraBuffer);
					scRet = ReqlSSLHandshake(overlappedResponse->conn->socket, overlappedResponse->conn->sslContext, NULL);
					if(scRet != SEC_E_OK) return scRet;

					if(overlappedResponse->conn->sslContext->ExtraData.pvBuffer) // Move any "extra" data to the input buffer.
					{
						printf("\nReqlDecryptResponseCallback::Unhandled Extra Data\n");
						//MoveMemory(overlappedConn->conn->response_buf, overlappedConn->conn->sslContextRef->ExtraData.pvBuffer, overlappedConn->conn->sslContextRef->ExtraData.cbBuffer);
						//cbIoBuffer = sslContextRef->ExtraData.cbBuffer;
					}
				}
				*/
			}
		}

	}

	//if( overlappedConn )
	//	free(overlappedConn);
	//overlappedConn = NULL;

	printf("\nReqlDecryptResponseCallback::End\n");
	ExitThread(0);
	return 0;
}


static unsigned long __stdcall CXReQLEncryptQueryCallback(LPVOID lpParameter)
{
	int i;
	ULONG_PTR CompletionKey;	//when to use completion key?
	uint64_t queryToken = 0;
	OVERLAPPED_ENTRY ovl_entry[CX_REQL_MAX_INFLIGHT_QUERIES];
	ULONG nPaquetsDequeued;

	ReqlOverlappedQuery * overlappedQuery = NULL;

	unsigned long NumBytesSent = 0;
	unsigned long recvMsgLength = 0;

	ReqlSSLStatus scRet = 0;
	ReqlDriverError reqlError = ReqlSuccess;

	CXReQLConnection * cxConn = (CXReQLConnection*)lpParameter;
	HANDLE hCompletionPort = cxConn->queryQueue();// (HANDLE)lpParameter;

	printf("CXReQLConnection::EncryptQueryCallback Begin\n");

	//TO DO:  Optimize number of packets dequeued at once
	while (GetQueuedCompletionStatusEx(hCompletionPort, ovl_entry, CX_REQL_MAX_INFLIGHT_QUERIES, &nPaquetsDequeued, INFINITE, TRUE))
	{
		printf("CXReQLConnection::EncryptQueryCallback::nPaquetsDequeued = %d\n", (int)nPaquetsDequeued);
		for (i = 0; i < nPaquetsDequeued; i++)
		{
			CompletionKey = (ULONG_PTR)ovl_entry[i].lpCompletionKey;
			NumBytesSent = ovl_entry[i].dwNumberOfBytesTransferred;
			overlappedQuery = (ReqlOverlappedQuery*)ovl_entry[i].lpOverlapped;

			if (!overlappedQuery)
			{
				printf("CXReQLConnection::EncryptQueryCallback::GetQueuedCompletionStatus continue\n");
				continue;
			}

			if (NumBytesSent == 0)
			{
				printf("CXReQLConnection::EncryptQueryCallback::Server Disconnected!!!\n");
			}
			else
			{
				//Get the connection
#ifdef _DEBUG
				assert(overlappedQuery->conn);
#endif
				NumBytesSent = overlappedQuery->len;
				printf("CXReQLConnection::EncryptQueryCallback::NumBytesSent = %d\n", (int)NumBytesSent);
				printf("CXReQLConnection::EncryptQueryCallback::msg = %s\n", overlappedQuery->buf + sizeof(ReqlQueryMessageHeader));

				//Use the ReQLC API to synchronously
				//a)	encrypt the query message in the query buffer
				//b)	send the encrypted query data over the socket
				scRet = CTSSLWrite(overlappedQuery->conn->socket, overlappedQuery->conn->sslContext, overlappedQuery->buf, &NumBytesSent);

#ifdef CTRANSPORT_USE_MBED_TLS
				//Issuing a zero buffer read will tell iocp to let us know when there is data available for reading from the socket
				//so we can issue our own blocking read
				recvMsgLength = 0;
#else
				//However, the zero buffer read doesn't seem to scale very well if Schannel is encrypting/decrypting for us 
				recvMsgLength = CX_REQL_RESPONSE_BUFF_SIZE;
#endif
				queryToken = overlappedQuery->queryToken;
				printf("CXReQLConnection::EncryptQueryCallback::queryTOken = %llu\n", queryToken);

				//Issue a blocking read for debugging purposes
				//ReqlRecv(overlappedQuery->conn, (void*)&(overlappedQuery->conn->response_buffers[ (queryToken%REQL_MAX_INFLIGHT_QUERIES) * REQL_RESPONSE_BUFF_SIZE]), &recvMsgLength);

				//Use WSASockets + IOCP to asynchronously do one of the following (depending on the usage of our underlying ssl context):
				//a)	notify us when data is available for reading from the socket via the rxQueue/thread (ie MBEDTLS path)
				//b)	read the encrypted response message from the socket into the response buffer and then notify us on the rxQueue/thread (ie SCHANNEL path)
				if ((reqlError = CTAsyncRecv(overlappedQuery->conn, (void*)&(overlappedQuery->conn->response_buffers[(queryToken%CX_REQL_MAX_INFLIGHT_QUERIES) * CX_REQL_RESPONSE_BUFF_SIZE]), &recvMsgLength)) != ReqlIOPending)
				{
					//the async operation returned immediately
					if (reqlError == ReqlSuccess)
					{
						printf("CXReQLConnection::EncryptQueryCallback::ReqlAsyncRecv returned immediate with %lu bytes\n", recvMsgLength);
					}
					else //failure
					{
						printf("CXReQLConnection::EncryptQueryCallback::ReqlAsyncRecv failed with ReqlDriverError = %d\n", reqlError);
					}
				}

				//Manual encryption is is left here commented out as an example
				//scRet = ReqlSSLEncryptMessage(overlappedConn->conn->sslContext, overlappedConn->conn->response_buf, &NumBytesRecv);
				//if( scRet == ReqlSSLContextExpired ) break; // Server signaled end of session
				//if( scRet == ReqlSuccess ) break;

				//TO DO:  Enable renogotiate, but how would the send thread/queue know if the server wants to perform another handshake?
				// The server wants to perform another handshake sequence.
				/*
				if(scRet == ReqlSSLRenegotiate)
				{
					printf("ReqlEncryptQueryCallback::Server requested renegotiate!\n");
					//scRet = ClientHandshakeLoop( Socket, phCreds, phContext, FALSE, &ExtraBuffer);
					scRet = ReqlSSLHandshake(overlappedQuery->conn->socket, overlappedQuery->conn->sslContext, NULL);
					if(scRet != SEC_E_OK) return scRet;

					if(overlappedQuery->conn->sslContext->ExtraData.pvBuffer) // Move any "extra" data to the input buffer.
					{
						printf("\nReqlEncryptQueryCallback::Unhandled Extra Data\n");
						//MoveMemory(overlappedConn->conn->response_buf, overlappedConn->conn->sslContextRef->ExtraData.pvBuffer, overlappedConn->conn->sslContextRef->ExtraData.cbBuffer);
						//cbIoBuffer = sslContextRef->ExtraData.cbBuffer;
					}
				}
				*/
			}
		}

	}

	printf("\nCXReQLConnectin::EncryptQueryCallback::End\n");
	ExitThread(0);
	return 0;

}


ReqlDispatchSource CXReQLConnection::startConnectionThreadQueues(ReqlThreadQueue rxQueue, ReqlThreadQueue txQueue)
{
#ifdef _WIN32
	//set up the win32 iocp handler for the connection's socket here
	int i;
	HANDLE hThread;

	//1  Create a single worker thread, respectively, for the CTConnection's (Socket) Tx and Rx Queues
	//   We associate our associated processing callbacks for each by associating the callback with the thread(s) we associate with the queues
	//   Note:  On Win32, "Queues" are actually IO Completion Ports
	for (i = 0; i < 1; ++i)
	{
		//On Darwin Platforms we utilize a thread pool implementation through GCD that listens for available data on the socket
		//and provides us with a dispatch_queue on a dedicated pool thread for reading
		//On Win32 platforms we create a thread/pool of threads to associate an io completion port
		//When Win32 sees that data is available for read it will read for us and provide the buffers to us on the completion port thread/pool
		hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CXReQLDecryptResponseCallback, this, 0, NULL);
		CloseHandle(hThread);

		hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CXReQLEncryptQueryCallback, this, 0, NULL);
		CloseHandle(hThread);
	}


#elif defined(__APPLE__)
	//set up the disptach_source_t of type DISPATCH_SOURCE_TYPE_READ using gcd to issue a callback callback/closure for the connection's socket here

#endif
}
