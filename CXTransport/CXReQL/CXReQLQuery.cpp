#include "../CXReQL.h"
//#include "CXReQLQuery.h"

using namespace CoreTransport;

CXReQLQuery::CXReQLQuery()
{
	_command = ReqlTermType(-1);
	_args = NULL;
	_options = NULL;

	//Create a memory pool allocator for the RapidJSON top level DOM object
	//Every CXReQLQuery Object gets allocated with a pool of static memory for this purpose (_domValueMemoryPool)
	_domValueAllocator = new MemoryPoolAllocator<>(_domValueMemoryPool, CX_REQL_QUERY_STATIC_BUFF_SIZE, 0, 0);
	//MemoryPoolAllocator<> parseAllocator(parseBuffer, sizeof(parseBuffer));
	//MemoryPoolAllocator<> parseAllocator2(parseBuffer2, sizeof(parseBuffer));

	_queryObject = new ReqlQueryDocumentType(_domValueAllocator, CX_REQL_QUERY_STATIC_BUFF_SIZE);
	_queryObject->SetObject();

	//Create a top Level RapidJSON DOM object for the ReQL Query
	//_queryArray = new ReqlQueryDocumentType(_domValueAllocator, CX_REQL_QUERY_STATIC_BUFF_SIZE, _domValueAllocator);
	_queryArray = new ReqlQueryDocumentType(_domValueAllocator, CX_REQL_QUERY_STATIC_BUFF_SIZE);
	_queryArray->SetArray();
	//d.Parse(json);
	//Document d;

	//TO DO:  handle REQL_EXPR
	//_queryArray->PushBack(command, *_domValueAllocator);

	_filepath = NULL;

}

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
	
	_queryObject = new ReqlQueryDocumentType(_domValueAllocator, CX_REQL_QUERY_STATIC_BUFF_SIZE);
	_queryObject->SetObject();

	//Create a top Level RapidJSON DOM object for the ReQL Query
	//_queryArray = new ReqlQueryDocumentType(_domValueAllocator, CX_REQL_QUERY_STATIC_BUFF_SIZE, _domValueAllocator);
	_queryArray = new ReqlQueryDocumentType(_domValueAllocator , CX_REQL_QUERY_STATIC_BUFF_SIZE);
	_queryArray->SetArray();
	//d.Parse(json);
	//Document d;

	//TO DO:  handle REQL_EXPR
	_queryArray->PushBack(command, *_domValueAllocator );

	_filepath = NULL;

}


CXReQLQuery::CXReQLQuery(ReqlTermType command, char * responsePath)
{
	_command = command;
	_args = NULL;
	_options = NULL;

	//Create a memory pool allocator for the RapidJSON top level DOM object
	//Every CXReQLQuery Object gets allocated with a pool of static memory for this purpose (_domValueMemoryPool)
	_domValueAllocator = new MemoryPoolAllocator<>(_domValueMemoryPool, CX_REQL_QUERY_STATIC_BUFF_SIZE, 0 , 0);
	//MemoryPoolAllocator<> parseAllocator(parseBuffer, sizeof(parseBuffer));
	//MemoryPoolAllocator<> parseAllocator2(parseBuffer2, sizeof(parseBuffer));
	
	_queryObject = new ReqlQueryDocumentType(_domValueAllocator, CX_REQL_QUERY_STATIC_BUFF_SIZE);
	_queryObject->SetObject();

	//Create a top Level RapidJSON DOM object for the ReQL Query
	//_queryArray = new ReqlQueryDocumentType(_domValueAllocator, CX_REQL_QUERY_STATIC_BUFF_SIZE, _domValueAllocator);
	_queryArray = new ReqlQueryDocumentType(_domValueAllocator , CX_REQL_QUERY_STATIC_BUFF_SIZE);
	_queryArray->SetArray();
	//d.Parse(json);
	//Document d;

	//TO DO:  handle REQL_EXPR
	_queryArray->PushBack(command, *_domValueAllocator );

	if( responsePath )
	{
		_filepath = responsePath;
	}
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
	
	_queryObject = new ReqlQueryDocumentType(_domValueAllocator, CX_REQL_QUERY_STATIC_BUFF_SIZE);
	_queryObject->SetObject();

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

	_filepath = NULL;
	//d.ParseInsitu(parseBuffer);
}


CXReQLQuery::CXReQLQuery(ReqlTermType command, Value *args, Value *options, char * responsePath)
{
	_command = command;
	_args = args;
	_options = options;

	//Create a memory pool allocator for the RapidJSON top level DOM object
	//Every CXReQLQuery Object gets allocated with a pool of static memory for this purpose (_domValueMemoryPool)
	_domValueAllocator = new MemoryPoolAllocator<>(_domValueMemoryPool, CX_REQL_QUERY_STATIC_BUFF_SIZE, 0 , 0);
	//MemoryPoolAllocator<> parseAllocator(parseBuffer, sizeof(parseBuffer));
	//MemoryPoolAllocator<> parseAllocator2(parseBuffer2, sizeof(parseBuffer));
	
	_queryObject = new ReqlQueryDocumentType(_domValueAllocator, CX_REQL_QUERY_STATIC_BUFF_SIZE);
	_queryObject->SetObject();

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

	if( responsePath )
	{
		_filepath = responsePath;
	}
}


CXReQLQuery::~CXReQLQuery()
{

	//delete rapidjson objects in reverse order
	fprintf(stderr, "CXReQLQuery::~CXReQLQuery\n");
	//delete rapidjson objects in reverse order
	delete _queryArray;
	delete _queryObject;
	delete _domValueAllocator;

}

MemoryPoolAllocator<>* CXReQLQuery::getAllocator() { return _domValueAllocator; }

ReqlQueryDocumentType* CXReQLQuery::getQueryArray() { return _queryArray; }

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

CXReQLQuery& CXReQLQuery::get(const char* primaryKeyValue)
{
	//TO DO:  get needs to be able to handle intentional null input
	
	//Create table name json string
	Value primaryKeyJsonStr;// = StringRef(dbName,8);
	primaryKeyJsonStr = StringRef(primaryKeyValue);          // shortcut, same as above

	//put the previous query and the table name json string in an array of arguments
	Value getQueryArgs(kArrayType);
	getQueryArgs.PushBack(*_queryArray, *_domValueAllocator).PushBack(primaryKeyJsonStr, *_domValueAllocator);

	//REQL_TABLE Command (15)
	Value getCommand(REQL_GET);
	//Value newQueryArray(kArrayType);

	//put the table command and its array of args in a new top level DOM object array
	ReqlQueryDocumentType* newQueryArray = new ReqlQueryDocumentType(_domValueAllocator, CX_REQL_QUERY_STATIC_BUFF_SIZE);
	newQueryArray->SetArray();
	newQueryArray->PushBack(getCommand, *_domValueAllocator).PushBack(getQueryArgs, *_domValueAllocator);

	delete _queryArray;			  //delete the old top level dom object
	_queryArray = newQueryArray; //store pointer to new top level dom object
	return *this;
}

CXReQLQuery& CXReQLQuery::gt(int value)
{
	//Create table name json string
	//Value primaryKeyJsonStr;// = StringRef(dbName,8);
	//primaryKeyJsonStr = StringRef(primaryKeyValue);          // shortcut, same as above

	//put the previous query and the table name json string in an array of arguments
	Value gtQueryArgs(kArrayType);
	gtQueryArgs.PushBack(*_queryArray, *_domValueAllocator).PushBack(Value(value), *_domValueAllocator);

	//REQL_TABLE Command (15)
	Value gtCommand(REQL_GT);
	//Value newQueryArray(kArrayType);

	//put the table command and its array of args in a new top level DOM object array
	ReqlQueryDocumentType* newQueryArray = new ReqlQueryDocumentType(_domValueAllocator, CX_REQL_QUERY_STATIC_BUFF_SIZE);
	newQueryArray->SetArray();
	newQueryArray->PushBack(gtCommand, *_domValueAllocator).PushBack(gtQueryArgs, *_domValueAllocator);

	delete _queryArray;			  //delete the old top level dom object
	_queryArray = newQueryArray; //store pointer to new top level dom object
	return *this;
}



CXReQLQuery& CXReQLQuery::coerceTo(const char* dataTypeStr)
{
	//Create table name json string
	Value dataTypeJsonStr;// = StringRef(dbName,8);
	dataTypeJsonStr = StringRef(dataTypeStr);          // shortcut, same as above

	//put the previous query and the table name json string in an array of arguments
	Value coerceToQueryArgs(kArrayType);
	coerceToQueryArgs.PushBack(*_queryArray, *_domValueAllocator).PushBack(dataTypeJsonStr, *_domValueAllocator);

	//REQL_TABLE Command (15)
	Value coerceToCommand(REQL_COERCE_TO);
	//Value newQueryArray(kArrayType);

	//put the table command and its array of args in a new top level DOM object array
	ReqlQueryDocumentType* newQueryArray = new ReqlQueryDocumentType(_domValueAllocator, CX_REQL_QUERY_STATIC_BUFF_SIZE);
	newQueryArray->SetArray();
	newQueryArray->PushBack(coerceToCommand, *_domValueAllocator).PushBack(coerceToQueryArgs, *_domValueAllocator);

	delete _queryArray;			  //delete the old top level dom object
	_queryArray = newQueryArray; //store pointer to new top level dom object
	return *this;

}

CXReQLQuery& CXReQLQuery::getAll(Value * secondaryKeyValue, const char* secondaryKeyIndex)
{
	//Create key value json string
	//Value secondaryKeyValueJsonStr;// = StringRef(dbName,8);
	//secondaryKeyValueJsonStr = StringRef(secondaryKeyValue);          // shortcut, same as above

	//Create index name json string
	Value secondaryKeyIndexJsonStr;// = StringRef(dbName,8);
	secondaryKeyIndexJsonStr = StringRef(secondaryKeyIndex);          // shortcut, same as above


	 //put the previous query and the table name json string in an array of arguments
	Value getQueryArgs(kArrayType);
	getQueryArgs.PushBack(*_queryArray, *_domValueAllocator).PushBack(*secondaryKeyValue, *_domValueAllocator);

	Value getQueryIndexOptions(kObjectType);
	getQueryIndexOptions.AddMember("index", secondaryKeyIndexJsonStr, *_domValueAllocator);

	//REQL_TABLE Command (15)
	Value getAllCommand(REQL_GET_ALL);
	//Value newQueryArray(kArrayType);

	//put the table command and its array of args in a new top level DOM object array
	ReqlQueryDocumentType* newQueryArray = new ReqlQueryDocumentType(_domValueAllocator, CX_REQL_QUERY_STATIC_BUFF_SIZE);
	newQueryArray->SetArray();
	newQueryArray->PushBack(getAllCommand, *_domValueAllocator).PushBack(getQueryArgs, *_domValueAllocator).PushBack(getQueryIndexOptions, *_domValueAllocator);

	delete _queryArray;			  //delete the old top level dom object
	_queryArray = newQueryArray; //store pointer to new top level dom object
	return *this;

}

CXReQLQuery& CXReQLQuery::getAll(const char* secondaryKeyValue, const char* secondaryKeyIndex)
{
	//Create key value json string
	Value secondaryKeyValueJsonStr;// = StringRef(dbName,8);
	secondaryKeyValueJsonStr = StringRef(secondaryKeyValue);          // shortcut, same as above

	//Create index name json string
	Value secondaryKeyIndexJsonStr;// = StringRef(dbName,8);
	secondaryKeyIndexJsonStr = StringRef(secondaryKeyIndex);          // shortcut, same as above


	 //put the previous query and the table name json string in an array of arguments
	Value getQueryArgs(kArrayType);
	getQueryArgs.PushBack(*_queryArray, *_domValueAllocator).PushBack(secondaryKeyValueJsonStr, *_domValueAllocator);

	Value getQueryIndexOptions(kObjectType);
	getQueryIndexOptions.AddMember("index", secondaryKeyIndexJsonStr, *_domValueAllocator);

	//REQL_TABLE Command (15)
	Value getAllCommand(REQL_GET_ALL);
	//Value newQueryArray(kArrayType);

	//put the table command and its array of args in a new top level DOM object array
	ReqlQueryDocumentType* newQueryArray = new ReqlQueryDocumentType(_domValueAllocator, CX_REQL_QUERY_STATIC_BUFF_SIZE);
	newQueryArray->SetArray();
	newQueryArray->PushBack(getAllCommand, *_domValueAllocator).PushBack(getQueryArgs, *_domValueAllocator).PushBack(getQueryIndexOptions, *_domValueAllocator);

	delete _queryArray;			  //delete the old top level dom object
	_queryArray = newQueryArray; //store pointer to new top level dom object
	return *this;

}

CXReQLQuery& CXReQLQuery::count()
{
	//put the previous query and the table name json string in an array of arguments
	Value countQueryArgs(kArrayType);
	countQueryArgs.PushBack(*_queryArray, *_domValueAllocator);// .PushBack(primaryKeyJsonStr, *_domValueAllocator);

	//REQL_TABLE Command (15)
	Value getCommand(REQL_COUNT);
	//Value newQueryArray(kArrayType);

	//put the table command and its array of args in a new top level DOM object array
	ReqlQueryDocumentType* newQueryArray = new ReqlQueryDocumentType(_domValueAllocator, CX_REQL_QUERY_STATIC_BUFF_SIZE);
	newQueryArray->SetArray();
	newQueryArray->PushBack(getCommand, *_domValueAllocator).PushBack(countQueryArgs, *_domValueAllocator);

	delete _queryArray;			  //delete the old top level dom object
	_queryArray = newQueryArray; //store pointer to new top level dom object
	return *this;

}


CXReQLQuery& CXReQLQuery::changes()
{
	//put the previous query and the table name json string in an array of arguments
	Value changeQueryArgs(kArrayType);
	changeQueryArgs.PushBack(*_queryArray, *_domValueAllocator);// .PushBack(primaryKeyJsonStr, *_domValueAllocator);

	Value changesCommand(REQL_CHANGES);

	//put the table command and its array of args in a new top level DOM object array
	ReqlQueryDocumentType* newQueryArray = new ReqlQueryDocumentType(_domValueAllocator, CX_REQL_QUERY_STATIC_BUFF_SIZE);
	newQueryArray->SetArray();
	newQueryArray->PushBack(changesCommand, *_domValueAllocator).PushBack(changeQueryArgs, *_domValueAllocator);

	delete _queryArray;			  //delete the old top level dom object
	_queryArray = newQueryArray; //store pointer to new top level dom object
	return *this;

}


CXReQLQuery& CXReQLQuery::changes(Value * options)
{
	//put the previous query and the table name json string in an array of arguments
	Value changeQueryArgs(kArrayType);
	changeQueryArgs.PushBack(*_queryArray, *_domValueAllocator);// .PushBack(primaryKeyJsonStr, *_domValueAllocator);

	Value changesCommand(REQL_CHANGES);

	//put the table command and its array of args in a new top level DOM object array
	ReqlQueryDocumentType* newQueryArray = new ReqlQueryDocumentType(_domValueAllocator, CX_REQL_QUERY_STATIC_BUFF_SIZE);
	newQueryArray->SetArray();
	newQueryArray->PushBack(changesCommand, *_domValueAllocator).PushBack(changeQueryArgs, *_domValueAllocator);

	if (options)
		newQueryArray->PushBack(*options, *_domValueAllocator);

	delete _queryArray;			  //delete the old top level dom object
	_queryArray = newQueryArray; //store pointer to new top level dom object
	return *this;

}

CXReQLQuery& CXReQLQuery::bland(CXReQLQuery* chainQuery)
{
	//put the previous query and the table name json string in an array of arguments
	Value andQueryArgs(kArrayType);
	andQueryArgs.PushBack(*_queryArray, *_domValueAllocator).PushBack(*chainQuery->getQueryArray(), *_domValueAllocator);

	//REQL_TABLE Command (15)
	Value getCommand(REQL_AND);
	//Value newQueryArray(kArrayType);

	//put the table command and its array of args in a new top level DOM object array
	ReqlQueryDocumentType* newQueryArray = new ReqlQueryDocumentType(_domValueAllocator, CX_REQL_QUERY_STATIC_BUFF_SIZE);
	newQueryArray->SetArray();
	newQueryArray->PushBack(getCommand, *_domValueAllocator).PushBack(andQueryArgs, *_domValueAllocator);

	delete _queryArray;			  //delete the old top level dom object
	_queryArray = newQueryArray; //store pointer to new top level dom object
	return *this;

}

CXReQLQuery& CXReQLQuery::insert(Value* jsonObj)
{
	//put the previous query and the table name json string in an array of arguments
	Value insertQueryArgs(kArrayType);
	insertQueryArgs.PushBack(*_queryArray, *_domValueAllocator).PushBack(*jsonObj, *_domValueAllocator);
	//REQL_TABLE Command (15)
	Value insertCommand(REQL_INSERT);
	//Value newQueryArray(kArrayType);

	//put the table command and its array of args in a new top level DOM object array
	ReqlQueryDocumentType* newQueryArray = new ReqlQueryDocumentType(_domValueAllocator, CX_REQL_QUERY_STATIC_BUFF_SIZE);
	newQueryArray->SetArray();
	newQueryArray->PushBack(insertCommand, *_domValueAllocator).PushBack(insertQueryArgs, *_domValueAllocator);

	delete _queryArray;			  //delete the old top level dom object
	_queryArray = newQueryArray; //store pointer to new top level dom object

	return *this;

}


CXReQLQuery& CXReQLQuery::insert(char* jsonObjStr)
{
	_queryObject->SetObject();
	_queryObject->Parse((char*)jsonObjStr);

	//put the previous query and the table name json string in an array of arguments
	Value insertQueryArgs(kArrayType);
	insertQueryArgs.PushBack(*_queryArray, *_domValueAllocator).PushBack(*_queryObject, *_domValueAllocator);
	//REQL_TABLE Command (15)
	Value insertCommand(REQL_INSERT);
	//Value newQueryArray(kArrayType);

	//put the table command and its array of args in a new top level DOM object array
	ReqlQueryDocumentType* newQueryArray = new ReqlQueryDocumentType(_domValueAllocator, CX_REQL_QUERY_STATIC_BUFF_SIZE);
	newQueryArray->SetArray();
	newQueryArray->PushBack(insertCommand, *_domValueAllocator).PushBack(insertQueryArgs, *_domValueAllocator);

	delete _queryArray;			  //delete the old top level dom object
	_queryArray = newQueryArray; //store pointer to new top level dom object

	return *this;
}


CXReQLQuery& CXReQLQuery::update(Value* jsonObj)
{
	//put the previous query and the table name json string in an array of arguments
	Value insertQueryArgs(kArrayType);
	insertQueryArgs.PushBack(*_queryArray, *_domValueAllocator).PushBack(*jsonObj, *_domValueAllocator);
	//REQL_TABLE Command (15)
	Value updateCommand(REQL_UPDATE);
	//Value newQueryArray(kArrayType);

	//put the table command and its array of args in a new top level DOM object array
	ReqlQueryDocumentType* newQueryArray = new ReqlQueryDocumentType(_domValueAllocator, CX_REQL_QUERY_STATIC_BUFF_SIZE);
	newQueryArray->SetArray();
	newQueryArray->PushBack(updateCommand, *_domValueAllocator).PushBack(insertQueryArgs, *_domValueAllocator);

	delete _queryArray;			  //delete the old top level dom object
	_queryArray = newQueryArray; //store pointer to new top level dom object

	return *this;

}


CXReQLQuery& CXReQLQuery::update(char* jsonObjStr)
{
	_queryObject->SetObject();
	_queryObject->Parse((char*)jsonObjStr);

	//put the previous query and the table name json string in an array of arguments
	Value insertQueryArgs(kArrayType);
	insertQueryArgs.PushBack(*_queryArray, *_domValueAllocator).PushBack(*_queryObject, *_domValueAllocator);
	//REQL_TABLE Command (15)
	Value updateCommand(REQL_UPDATE);
	//Value newQueryArray(kArrayType);

	//put the table command and its array of args in a new top level DOM object array
	ReqlQueryDocumentType* newQueryArray = new ReqlQueryDocumentType(_domValueAllocator, CX_REQL_QUERY_STATIC_BUFF_SIZE);
	newQueryArray->SetArray();
	newQueryArray->PushBack(updateCommand, *_domValueAllocator).PushBack(insertQueryArgs, *_domValueAllocator);

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
ReqlDriverError CXReQLQuery::CXReQLSendWithQueue(CTConnection* conn, void * msg, unsigned long * msgLength)
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
	overlappedQuery->wsaBuf.buf = (char*)msg;
	overlappedQuery->wsaBuf.len = *msgLength;
	//overlappedQuery->buf = (char*)msg;
	//overlappedQuery->len = *msgLength;
	overlappedQuery->conn = conn;
	overlappedQuery->queryToken = *(uint64_t*)msg;

	fprintf(stderr, "CXReQLSendWithQueue::overlapped->queryBuffer = %s\n", (char*)msg + sizeof(ReqlQueryMessageHeader));

	//Post the overlapped object message asynchronously to the socket transmit thread queue using Win32 Overlapped IO and IOCP
	if (!PostQueuedCompletionStatus(conn->socketContext.txQueue, *msgLength, dwCompletionKey, &(overlappedQuery->Overlapped)))
	{
		fprintf(stderr, "\nCXReQLSendWithQueue::PostQueuedCompletionStatus failed with error:  %d\n", GetLastError());
		return (ReqlDriverError)GetLastError();
	}

	fprintf(stderr, "CXReQLSendWithQueue::after PostQueuedCompletionStatus\n");


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
	return ReqlSuccess;

}


uint64_t CXReQLQuery::RunQueryWithCursorOnQueue(std::shared_ptr<CXCursor> cxCursor, uint64_t queryToken)//, void * options)//, ReqlQueryClosure callback)
{

	//a string/stream for serializing the json query to char*	
	unsigned long queryHeaderLength, queryMessageLength;
	int32_t queryStrLength;

	//serialize the query array to string

	char * queryBuffer = cxCursor->_cursor->requestBuffer;//&(conn->query_buffers[(queryToken % CX_REQL_MAX_INFLIGHT_QUERIES) * CX_REQL_QUERY_STATIC_BUFF_SIZE]);
	char * queryMsgBuffer = queryBuffer + sizeof(ReqlQueryMessageHeader);
	
	//Serialize to buffer
	//fprintf(stderr, "CXREQL_QUERY_BUF_SIZE = %d\n", CX_REQL_QUERY_STATIC_BUFF_SIZE);
	CXRapidJsonString parseDomToJsonString(&queryMsgBuffer, CX_REQL_QUERY_STATIC_BUFF_SIZE - sizeof(ReqlQueryMessageHeader));
	rapidjson::Writer<CXRapidJsonString> writer(parseDomToJsonString);
	_queryArray->Accept(writer);

	//*queryMsgBuffer='\0';
	//queryMsgBuffer++;

	//fprintf(stderr, "parseDomToJsonBuffer = \n\n%s\n\n", queryBuffer + sizeof(ReqlQueryMessageHeader));
	queryStrLength = queryMsgBuffer - queryBuffer - sizeof(ReqlQueryMessageHeader);// (int32_t)strlen(queryMsgBuffer);// jsonQueryStr.length();

	//now we know the size of the query string we can define the header
	ReqlQueryMessageHeader * queryHeader = (ReqlQueryMessageHeader*)queryBuffer;// queryBuffer;// { queryToken, (int32_t)queryStrLength };
	queryHeader->token = queryToken;
	queryHeader->length = (int32_t)queryStrLength;

#ifdef _DEBUG
	fprintf(stderr, "CXReQLRunQueryWithTokenOnQueue::jsonQueryStream (%d) = %.*s\n", (int)queryStrLength, (int)queryStrLength, queryBuffer + sizeof(ReqlQueryMessageHeader));
#endif	
	queryMessageLength = sizeof(ReqlQueryMessageHeader) + queryStrLength;

	//Send
	//ReqlSend(conn, (void*)&queryHeader, queryHeaderLength);
	//CXReQLSendWithQueue(conn, (void*)queryBuffer, &queryMessageLength);
	//return CTSendOnQueue2(cxCursor->_cursor.conn, (char **)&queryBuffer, queryMessageLength, queryToken, &(cxCursor->_cursor.overlappedResponse));//, &overlappedResponse);
	cxCursor->_cursor->queryToken = queryToken;
	return CTCursorSendOnQueue((cxCursor->_cursor), (char**)&queryBuffer, queryMessageLength);
	//return queryHeader->token;
}





/*
uint64_t CXReQLQuery::run(CXReQLConnection* conn, Value* options, std::function<void(char * buffer)> const &callback)
{
	uint64_t expectedQueryToken = (conn->connection()->queryCount++);
	//CXReQL_fprintf(stderr, "CXReQLQuery::Run ExpectedQueryToken = %llu\n", expectedQueryToken);

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