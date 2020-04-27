#include "../CXURL.h"
//#include "CXURLRequest.h"

using namespace CoreTransport;

CXURLRequest::CXURLRequest(URLRequestType command, const char * requestPath, std::map<char*, char*> *headers, char * body, unsigned long contentLength)
{
	_command = command;
	_requestPath = (char*)requestPath;

	//Is this a deep copy?
	if( headers )
		_headers = *headers;

	_body = body;
	_contentLength = contentLength;

	_filepath = ct_file_name((char*)requestPath);
}

CXURLRequest::CXURLRequest(URLRequestType command, const char * requestPath, std::map<char*, char*> *headers, char * body, unsigned long contentLength, const char * responsePath)
{
	_command = command;
	_requestPath = (char*)requestPath;

	//Is this a deep copy?
	if( headers )
		_headers = *headers;

	_body = body;
	_contentLength = contentLength;

	if( responsePath )
	{
		_filepath = responsePath;
	}
}



CXURLRequest::~CXURLRequest()
{

}



/***
 *	ReqlSendWithQueue
 *
 *  Sends a message buffer to a dedicated platform "Queue" mechanism
 *  for encryption and then transmission over socket
 ***/
CTClientError CXURLRequest::SendWithQueue(CTConnection* conn, void * msg, unsigned long * msgLength)
{

	
#ifdef _WIN32

	ULONG_PTR dwCompletionKey = (ULONG_PTR)NULL;
	//Send Query Asynchronously with Windows IOCP
	//Create an overlapped connetion object to pass what we need to the iocp callback
	//Instead of allocating memory we will assume the send buffer is large enough and tack it on the end
	ReqlOverlappedQuery * overlappedQuery = (ReqlOverlappedQuery*)((char*)msg + *msgLength + 1); //+1 for null terminator needed for encryption
	//overlappedQuery = (ReqlOverlappedQuery *)malloc( sizeof(ReqlOverlappedQuery) );
	ZeroMemory(overlappedQuery, sizeof(ReqlOverlappedQuery)); //this is critical for proper overlapped/iocp operation!
	overlappedQuery->Overlapped.hEvent = CreateEvent(NULL, 0, 0, NULL); //Manual vs Automatic Reset Events Affect GetQueued... Operation!!!
	overlappedQuery->buf = (char*)msg;
	overlappedQuery->len = *msgLength;
	overlappedQuery->conn = conn;
	overlappedQuery->queryToken = *(uint64_t*)msg;

	printf("CXReQLSendWithQueue::overlapped->queryBuffer = %s\n", (char*)msg + sizeof(ReqlQueryMessageHeader));

	//Post the overlapped object message asynchronously to the socket transmit thread queue using Win32 Overlapped IO and IOCP
	if (!PostQueuedCompletionStatus(conn->socketContext.txQueue, *msgLength, dwCompletionKey, &(overlappedQuery->Overlapped)))
	{
		printf("\nCXReQLSendWithQueue::PostQueuedCompletionStatus failed with error:  %d\n", GetLastError());
		return (ReqlDriverError)GetLastError();
	}

	printf("CXReQLSendWithQueue::after PostQueuedCompletionStatus\n");


#elif defined(__APPLE__)
	//TO DO?
#endif
	


	//ReqlSuccess will indicate the async operation finished immediately
	return ReqlSuccess;

}

	//void addQueryCallbackForKey(std::function<void(CTError* error, CXReQLCursor* cursor)> const &callback, uint64_t queryToken) {_queries.insert(std::make_pair(queryToken, callback));}
	//		void removeQueryCallbackForKey(uint64_t queryToken) {_queries.erase(queryToken);}
	//		std::function<void(CTError* error, CXReQLCursor* cursor)> getQueryCallbackForKey(uint64_t queryToken) { return _queries.at(queryToken); }


void CXURLRequest::setValueForHTTPHeaderField(char * value, char * field)
{
	_headers.insert(std::make_pair(value, field));
}

uint64_t CXURLRequest::SendRequestWithCursorOnQueue(std::shared_ptr<CXCursor> cxCursor, uint64_t requestToken)//, void * options)//, ReqlQueryClosure callback)
{

	//a string/stream for serializing the json query to char*	
	unsigned long requestHeaderLength, requestLength;
	//int32_t requestLength;

	
	//get the buffer for sending with async iocp

	//unsigned long cBufferSize = ct_system_allocation_granularity(); //+ sizeof(CTOverlappedResponse); 
	char * requestBuffer = cxCursor->_cursor.requestBuffer;//&(cxCursor->_cursor.conn->request_buffers[(requestToken % CX_MAX_INFLIGHT_QUERIES) * cBufferSize ]);
	char * baseBuffer = requestBuffer;

	//serialize the http headers and body to a string
	if( _command == URL_GET)
	{
		memcpy( requestBuffer, "GET ", 4);
		requestBuffer+=4;

	}
	if( _requestPath )
	{
		memcpy( requestBuffer, _requestPath, strlen( _requestPath ));
		requestBuffer+= strlen(_requestPath);
	}else
	{
		memcpy( requestBuffer, "/", 1);
		requestBuffer+=1;
	}


	memcpy( requestBuffer, " HTTP/1.1\r\n", strlen( " HTTP/1.1\r\n" ));
	requestBuffer += strlen( " HTTP/1.1\r\n" );
	//Add Host and User Agent Headers
	memcpy( requestBuffer, "Host: ", strlen("Host: "));
	requestBuffer+=strlen("Host: ");

	memcpy( requestBuffer, cxCursor->_cursor.conn->target->host, strlen(cxCursor->_cursor.conn->target->host));
	requestBuffer+=strlen(cxCursor->_cursor.conn->target->host);

	memcpy( requestBuffer, "\r\nUser-Agent: CXTransport\r\n", strlen("\r\nUser-Agent: CXTransport\r\n"));
	requestBuffer += strlen("\r\nUser-Agent: CXTransport\r\n");

	//add user headers

	std::map<char*, char*>::iterator it = _headers.begin();
	std::map<char*, char*>::iterator endIt = _headers.end();
	
	std::cout << "CXURLRequest::SendRequestWithTokenOnQueue Parsing HTTP Headers...\n";
	
	for( it; it!=endIt; ++it)
	{

		std::cout << it->first << ": " << it->second << "\n";

		memcpy( requestBuffer, it->first, strlen(it->first));
		requestBuffer+=strlen(it->first);
		memcpy( requestBuffer, ": ", strlen(": "));
		requestBuffer += strlen(": ");
		memcpy( requestBuffer, it->second, strlen(it->second));
		requestBuffer+= strlen(it->second);
		memcpy( requestBuffer, "\r\n", strlen("\r\n"));
		requestBuffer+= strlen("\r\n");
	}

	//now do the last one
	/*
	std::cout << it->first << ": " << it->second << "\n";
	memcpy( requestBuffer, it->first, strlen(it->first));
	requestBuffer+=strlen(it->first);
	memcpy( requestBuffer, ": ", strlen(": "));
	requestBuffer += strlen(": ");
	memcpy( requestBuffer, it->second, strlen(it->second));
	requestBuffer+= strlen(it->second);
	memcpy( requestBuffer, "\r\n", strlen("\r\n"));
	requestBuffer+= strlen("\r\n");
	*/

	//now add the final return carriage to the HTTP header
	memcpy( requestBuffer, "\r\n\0", strlen("\r\n\0"));
	requestBuffer+= strlen("\r\n\0");

	//calulcate the request length
	requestLength = requestBuffer-baseBuffer;//queryMsgBuffer - queryBuffer - sizeof(ReqlQueryMessageHeader);// (int32_t)strlen(queryMsgBuffer);// jsonQueryStr.length();

	//print the request for debuggin
	printf("CXURLSendRequestWithTokenOnQueue::request (%d) = %.*s\n", (int)requestLength, (int)requestLength, baseBuffer);

	//Send the HTTP(S) request
	//ReqlSend(conn, (void*)&queryHeader, queryHeaderLength);
	//CXURLSendWithQueue(conn, (void*)requestBuffer, &requestLength);
	//CTOverlappedResponse * overlappedResponse = &_overlappedResponse;
	//return CTSendOnQueue2(cxCursor->_cursor.conn, (char **)&baseBuffer, requestLength, requestToken, &(cxCursor->_cursor.overlappedResponse));//, &overlappedResponse);
	
	//CXCursor internal CTCursor conn was already set when CXCursor was created with constructor
	//cxCursor->_cursor->conn = &_httpConn;
	cxCursor->_cursor.queryToken = requestToken;
	return CTCursorSendOnQueue(&(cxCursor->_cursor), (char**)&baseBuffer, requestLength);
}
