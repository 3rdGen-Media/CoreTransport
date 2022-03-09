#include "../CXTransport.h"

using namespace CoreTransport;

CXConnection::CXConnection(CTConnection *conn, CTThreadQueue rxQueue, CTThreadQueue txQueue)
{
	//copy the from CTConnection CTC memory to CXConnection managed memory (because it will go out of scope)
	//followup note:  this is no longer the case with connection pool feature + the nonblocking path
	// but is still the case for the blocking connection path (though it doesn't have to be)
	memcpy(&_conn, conn, sizeof(CTConnection));

	//Reserve/allocate memory for the connection's query and response buffers 
	//reserveConnectionMemory();

	//create a dispatch source / handler for the CTConnection's socket
	if (!rxQueue)
		_rxQueue = _conn.socketContext.rxQueue;
	else 
		_rxQueue = rxQueue;
	
	if(!txQueue)
		_txQueue = _conn.socketContext.txQueue;
	else
		_txQueue = txQueue;

	
	//IMPORTANT:  Point CTransport connection at it's CXTransport wrapper object for use by CXRoutines
	_conn.object_wrapper = this;

	//startConnectionThreadQueues(_rxQueue, _txQueue);

}

CXConnection::~CXConnection()
{
	printf("CXConnection::Destructor()\n");
	close();
}

void CXConnection::close()
{
	CTCloseConnection(&_conn);
}

CTThreadQueue CXConnection::queryQueue()
{
	return _txQueue;
}

CTThreadQueue CXConnection::responseQueue()
{
	return _rxQueue;
}

void CXConnection::distributeResponseWithCursorForToken(uint64_t requestToken)
{
		std::shared_ptr<CXCursor> cxCursor = getRequestCursorForKey(requestToken);
		auto callback = getRequestCallbackForKey(requestToken);

		if( callback )
		{
			//TO DO:  The header and or body needs to be parsed for the error to be populated
			//appropriately based on the various connection protocols
			callback( NULL, cxCursor);
			removeRequestCallbackForKey(requestToken);
		}
		else
		{
			printf("CXConnection::distributeResponseWithCursorForToken::No Callback!");
			printQueries();
		}

		//We are done with the cursor
		//Do the manual cleanup we need to do
		//and let the rest get cleaned up when the sharedptr to the CXCursor goes out of scope
		removeRequestCursorForKey(requestToken);
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


std::shared_ptr<CXCursor> CXConnection::createRequestCursor(uint64_t queryToken)
{
	std::shared_ptr<CXCursor> cxCursor( new CXCursor(&_conn) );
	addRequestCursorForKey(cxCursor, queryToken);
	return cxCursor;
}
	
/*
CTDispatchSource CXConnection::startConnectionThreadQueues(CTThreadQueue rxQueue, CTThreadQueue txQueue)
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
		hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CXDequeue_Recv_Decrypt, rxQueue, 0, NULL);
		CloseHandle(hThread);

		hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CXDequeue_Encrypt_Send, txQueue, 0, NULL);
		CloseHandle(hThread);
	}


#elif defined(__APPLE__)
	//set up the disptach_source_t of type DISPATCH_SOURCE_TYPE_READ using gcd to issue a callback callback/closure for the connection's socket here

#endif
}
*/