#pragma once
#ifndef CXREQLQUERY_H
#define CXREQLQUERY_H

//#include <CoreTransport/CTSockets.h>
//#include "CXReQLCursor.h"

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include <rapidjson/writer.h>

using namespace rapidjson;
//define the type of document and allocators that rapidjson will use for building a DOM object
typedef GenericDocument<UTF8<>, MemoryPoolAllocator<>> ReqlQueryDocumentType;
typedef GenericStringBuffer<UTF8<>, MemoryPoolAllocator<>> ReqlQueryStringBufferType;


#define CX_REQL_QUERY_STATIC_BUFF_SIZE 32768L
//cpprestsdk json
//#include "cpprest/json.h"
//#define jsval json::value
//#define jsint(x) json::value::number(x) 
//#define jsstr(x) json::value::string(x) 
//#define jsarr(x) json::value::array(x)
//#define jsobj(x) json::value::object(x)
//#define jsparse(x) json::value::parse(x)
//#define jsdict std::vector< std::pair<std::string, jsval> >
//using namespace web;


namespace CoreTransport
{

	/*
	static void UWP_printf(char *format, ...)
	{
		char REQL_PRINT_BUFFER[4096];

	
		va_list args;
		va_start(args, format);
		int nBuf;
		//TCHAR szBuffer[512]; // get rid of this hard-coded buffer
		nBuf = _vsnprintf_s(REQL_PRINT_BUFFER, 4095, format, args);
		OutputDebugStringA(REQL_PRINT_BUFFER);
		va_end(args);
	}
	*/
	#define CXReQL_printf(f_, ...) printf((f_), __VA_ARGS__)

	//typedef void(*CXReQLQueryClosure) (ReqlError * error, CXReQLCursor * cursor);


	class CXREQL_API CXReQLQuery
	{
	public:
		CXReQLQuery(ReqlTermType command);
		CXReQLQuery(ReqlTermType command, char * responsePath);
		CXReQLQuery(ReqlTermType command, Value* args, Value* options);
		CXReQLQuery(ReqlTermType command, Value *args, Value *options, char * responsePath);

		~CXReQLQuery(void);

		void doNothing() { return; }

		//initializers
		void setQueryArgs(Value * args);
		//void setOptions(Value * args);

		//Queries
		CXReQLQuery& table(const char * name);

		//Send Query Methods
		//uint64_t CXReQLRunQueryWithTokenOnQueue(CTConnection * conn, uint64_t queryToken);//, void * options)//, ReqlQueryClosure callback)
		
		//Need to convert the above methods ot the follwoing:
		ReqlDriverError CXReQLSendWithQueue(CTConnection* conn, void * msg, unsigned long * msgLength);
		//uint64_t SendRequestWithTokenOnQueue(CTConnection * conn, uint64_t requestToken);//, void * options)//, ReqlQueryClosure callback)
		uint64_t RunQueryWithCursorOnQueue(std::shared_ptr<CXCursor> cxCursor, uint64_t queryToken);//, void * options)//, ReqlQueryClosure callback)
		


		template<typename CXReQLQueryClosure>
		uint64_t run(CXConnection* conn, Value* options, CXReQLQueryClosure callback)
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

			
			if( !_filepath )
			{
				_filepath = "ReQLQuery.txt\0";//ct_file_name((char*)requestPath);
				printf("CXReQLQUery::run filepath = %s\n", _filepath);
			}

			//create a cursor (for the query response, but also it holds our query buffer)
			//std::shared_ptr<CXCursor> cxCursor = conn->createRequestCursor(expectedQueryToken);
			std::shared_ptr<CXReQLCursor> cxReQLCursor( new CXReQLCursor(conn->connection(), _filepath) );
			conn->addRequestCursorForKey(std::static_pointer_cast<CXCursor>(cxReQLCursor), expectedQueryToken);
			conn->addRequestCallbackForKey(callback, expectedQueryToken);
			//conn->addQueryCallbackForKey(callback, expectedQueryToken);
			
			//return SendRequestWithCursorOnQueue(cxURLCursor, expectedQueryToken);
			return RunQueryWithCursorOnQueue(cxReQLCursor, expectedQueryToken);

			//CXReQLRunQueryWithTokenOnQueue(conn->connection(), expectedQueryToken);

			//delete after so we don't hold up the async send with a deallocation
			delete oldQueryArray;
			
			return expectedQueryToken;
		}


		MemoryPoolAllocator<>* getAllocator() {return _domValueAllocator; }

	protected:


	private:
		//ReqlService * _service;
		ReqlTermType						_command;
		ReqlQueryDocumentType*				_queryArray;
		ReqlQueryStringBufferType*			_parseStringBuffer;
		Writer<ReqlQueryStringBufferType>*	_domWriter;//(parseStringBuffer);

		char								_domValueMemoryPool[CX_REQL_QUERY_STATIC_BUFF_SIZE];
		MemoryPoolAllocator<>*				_domValueAllocator;//(_queryBuffer, CX_REQL_QUERY_STATIC_BUFF_SIZE);

		Value *_args;
		Value *_options;

		
		//CTOverlappedResponse	_overlappedResponse;
		const char *					_filepath;

	};


	typedef CXReQLQuery* (*CXReQLStringQueryFunc) (const char * name);

//#define run(conn, options, callback) .runQuery(conn, options, [](void* arg){ (*static_cast<decltype(callback)*>(arg))(); }, &callback)

}



#endif