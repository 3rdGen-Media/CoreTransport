#include "../CXReQL.h"
//#include "CXReQLQuery.h"

using namespace CoreTransport;

CXReQLQuery::CXReQLQuery(ReqlTermType command)
{
	_command = command;
	_args = NULL;
	_options = NULL;

	//Create a memory pool allocator for the RapidJSON top level DOM object
	//Every CXReQLQuery Object gets allocated with a pool of static memory for this purpose (_domValueMemoryPool)
	_domValueAllocator = new MemoryPoolAllocator<>(_domValueMemoryPool, CX_REQL_QUERY_STATIC_BUFF_SIZE, 0 , 0);
	//MemoryPoolAllocator<> parseAllocator(parseBuffer, sizeof(parseBuffer));
	//MemoryPoolAllocator<> parseAllocator2(parseBuffer2, sizeof(parseBuffer));
	
	//Create a top Level RapidJSON DOM object for the ReQL Query
	//_queryArray = new ReqlQueryDocumentType(_domValueAllocator, CX_REQL_QUERY_STATIC_BUFF_SIZE, _domValueAllocator);
	_queryArray = new ReqlQueryDocumentType(_domValueAllocator , CX_REQL_QUERY_STATIC_BUFF_SIZE);
	_queryArray->SetArray();
	//d.Parse(json);
	//Document d;

	//TO DO:  handle REQL_EXPR
	_queryArray->PushBack(command, *_domValueAllocator );
}

void CXReQLQuery::setQueryArgs(Value * args)
{
	if( _options )
	{
		_queryArray->PopBack();
		_queryArray->PopBack();
		_queryArray->PushBack(*args, *_domValueAllocator);
		_queryArray->PushBack(*_options, *_domValueAllocator);

	}
	else if( _args )
	{
		_queryArray->PopBack();
		_queryArray->PushBack(*args, *_domValueAllocator);
	}
	else //assume command is already set
		_queryArray->PushBack(*args, *_domValueAllocator);

	_args = args;

}


CXReQLQuery::CXReQLQuery(ReqlTermType command, Value *args, Value *options)
{
	_command = command;
	_args = args;
	_options = options;

	//Create a memory pool allocator for the RapidJSON top level DOM object
	//Every CXReQLQuery Object gets allocated with a pool of static memory for this purpose (_domValueMemoryPool)
	_domValueAllocator = new MemoryPoolAllocator<>(_domValueMemoryPool, CX_REQL_QUERY_STATIC_BUFF_SIZE, 0 , 0);
	//MemoryPoolAllocator<> parseAllocator(parseBuffer, sizeof(parseBuffer));
	//MemoryPoolAllocator<> parseAllocator2(parseBuffer2, sizeof(parseBuffer));
	
	//Create a top Level RapidJSON DOM object for the ReQL Query
	//_queryArray = new ReqlQueryDocumentType(_domValueAllocator, CX_REQL_QUERY_STATIC_BUFF_SIZE, _domValueAllocator);
	_queryArray = new ReqlQueryDocumentType(_domValueAllocator , CX_REQL_QUERY_STATIC_BUFF_SIZE);
	_queryArray->SetArray();
	//d.Parse(json);
	//Document d;

	//TO DO:  handle REQL_EXPR
	_queryArray->PushBack(command, *_domValueAllocator );
	if(args)
		_queryArray->PushBack(*args, *_domValueAllocator );
	if( options )
	{
		assert(1==0);
		_queryArray->PushBack(*options, *_domValueAllocator );
	}
	//d.ParseInsitu(parseBuffer);
}

CXReQLQuery::~CXReQLQuery()
{

	//delete rapidjson objects in reverse order
	delete _queryArray;
	delete _domValueAllocator;
}

	//A Custom String class to provide char* buffer to rapidjson::writer
	class CXReQLString
	{
		public:
		typedef char Ch;
		CXReQLString(char ** buffer, unsigned long bufSize) : _buffer(buffer) { _bufferStart = *_buffer; _bufSize = bufSize; }
		void Put(char c) { **_buffer=c;(*_buffer)++; }
		void Clear() { memset(_bufferStart, 0, _bufSize); }
		void Flush() { return; }
		size_t Size() const { return *_buffer - _bufferStart; }
		private:
		char * _bufferStart;
		char ** _buffer ;
		unsigned long _bufSize;
	};


CXReQLQuery& CXReQLQuery::table(const char * name)
{
	//Create table name json string
	Value tableNameJsonStr;// = StringRef(dbName,8);
	tableNameJsonStr = StringRef(name);          // shortcut, same as above

	//put the previous query and the table name json string in an array of arguments
	Value tableQueryArgs(kArrayType);
	tableQueryArgs.PushBack(*_queryArray, *_domValueAllocator).PushBack(tableNameJsonStr, *_domValueAllocator);

	//REQL_TABLE Command (15)
	Value tableCommand(REQL_TABLE);
	//Value newQueryArray(kArrayType);
	
	//put the table command and its array of args in a new top level DOM object array
	ReqlQueryDocumentType * newQueryArray = new ReqlQueryDocumentType(_domValueAllocator , CX_REQL_QUERY_STATIC_BUFF_SIZE);
	newQueryArray->SetArray();
	newQueryArray->PushBack( tableCommand, *_domValueAllocator).PushBack(tableQueryArgs, *_domValueAllocator); 

	delete _queryArray;			  //delete the old top level dom object
	_queryArray = newQueryArray; //store pointer to new top level dom object
	return *this;
}



/***
 *	ReqlSendWithQueue
 *
 *  Sends a message buffer to a dedicated platform "Queue" mechanism
 *  for encryption and then transmission over socket
 ***/
ReqlDriverError CXReQLQuery::CXReQLSendWithQueue(ReqlConnection* conn, void * msg, unsigned long * msgLength)
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
	return ReqlSuccess;

}


uint64_t CXReQLQuery::CXReQLRunQueryWithTokenOnQueue(ReqlConnection * conn, uint64_t queryToken)//, void * options)//, ReqlQueryClosure callback)
{

	//a string/stream for serializing the json query to char*	
	unsigned long queryHeaderLength, queryMessageLength;
	int32_t queryStrLength;

	//serialize the query array to string

	char * queryBuffer = &(conn->query_buffers[(queryToken % CX_REQL_MAX_INFLIGHT_QUERIES) * CX_REQL_QUERY_STATIC_BUFF_SIZE]);
	char * queryMsgBuffer = queryBuffer + sizeof(ReqlQueryMessageHeader);
	
	//Serialize to buffer
	//printf("CXREQL_QUERY_BUF_SIZE = %d\n", CX_REQL_QUERY_STATIC_BUFF_SIZE);
	CXReQLString parseDomToJsonString(&queryMsgBuffer, CX_REQL_QUERY_STATIC_BUFF_SIZE - sizeof(ReqlQueryMessageHeader));
	rapidjson::Writer<CXReQLString> writer(parseDomToJsonString);
	_queryArray->Accept(writer);

	//*queryMsgBuffer='\0';
	//queryMsgBuffer++;

	//printf("parseDomToJsonBuffer = \n\n%s\n\n", queryBuffer + sizeof(ReqlQueryMessageHeader));
	queryStrLength = queryMsgBuffer - queryBuffer - sizeof(ReqlQueryMessageHeader);// (int32_t)strlen(queryMsgBuffer);// jsonQueryStr.length();

	//now we know the size of the query string we can define the header
	ReqlQueryMessageHeader * queryHeader = (ReqlQueryMessageHeader*)queryBuffer;// queryBuffer;// { queryToken, (int32_t)queryStrLength };
	queryHeader->token = queryToken;
	queryHeader->length = (int32_t)queryStrLength;

	printf("CXReQLRunQueryWithTokenOnQueue::jsonQueryStream (%d) = %s\n", (int)queryStrLength, queryBuffer + sizeof(ReqlQueryMessageHeader));
	queryMessageLength = sizeof(ReqlQueryMessageHeader) + queryStrLength;

	//Send
	//ReqlSend(conn, (void*)&queryHeader, queryHeaderLength);
	CXReQLSendWithQueue(conn, (void*)queryBuffer, &queryMessageLength);

	return queryHeader->token;
}
/*
uint64_t CXReQLQuery::run(CXReQLConnection* conn, Value* options, std::function<void(char * buffer)> const &callback)
{
	uint64_t expectedQueryToken = (conn->connection()->queryCount++);
	//CXReQL_printf("CXReQLQuery::Run ExpectedQueryToken = %llu\n", expectedQueryToken);

	Value runCommand(1);
	//Value newQueryArray(kArrayType);
	
	ReqlQueryDocumentType * newQueryArray = new ReqlQueryDocumentType(_domValueAllocator , CX_REQL_QUERY_STATIC_BUFF_SIZE);
	newQueryArray->SetArray();
	newQueryArray->PushBack( runCommand, *_domValueAllocator).PushBack(*_queryArray, *_domValueAllocator); 
	if( options )
		newQueryArray->PushBack( *options, *_domValueAllocator);

	ReqlQueryDocumentType * oldQueryArray = _queryArray;
	_queryArray = newQueryArray;

	CXReQLRunQueryWithTokenOnQueue(conn->connection(), expectedQueryToken);
			
	callback(NULL);

	//delete after so we don't hold up the async send with a deallocation
	delete oldQueryArray;
			
	return expectedQueryToken;
}
*/