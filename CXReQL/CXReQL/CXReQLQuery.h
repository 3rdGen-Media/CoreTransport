#pragma once
#ifndef CXREQL_QUERY_H
#define CX_REQL_QUERY_H

#include <CoreTransport/ctReQL.h>
#include "CXReQLCursor.h"
#include "CXReQLConnection.h"

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


	static void UWP_printf(char *format, ...)
	{
		char REQL_PRINT_BUFFER[4096];

		/*
			va_list args;
			va_start(args, format);
		#if defined(_WIN32)//WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
			vsprintf_s(REQL_PRINT_BUFFER, 1, format, args);
		#else
			vprintf(format, args);
		#endif
			va_end(args);

		#if defined(_WIN32)//WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
			OutputDebugStringA(REQL_PRINT_BUFFER);
		#endif
		*/

		va_list args;
		va_start(args, format);
		int nBuf;
		//TCHAR szBuffer[512]; // get rid of this hard-coded buffer
		nBuf = _vsnprintf_s(REQL_PRINT_BUFFER, 4095, format, args);
		OutputDebugStringA(REQL_PRINT_BUFFER);
		va_end(args);
	}

	#define CXReQL_printf(f_, ...) printf((f_), __VA_ARGS__)

	//typedef void(*CXReQLQueryClosure) (ReqlError * error, CXReQLCursor * cursor);


	class CXREQL_API CXReQLQuery
	{
	public:
		CXReQLQuery(ReqlTermType command);
		CXReQLQuery(ReqlTermType command, Value* args, Value* options);
		~CXReQLQuery(void);

		void doNothing() { return; }

		//initializers
		void setQueryArgs(Value * args);
		//void setOptions(Value * args);

		//Queries
		CXReQLQuery& table(const char * name);

		//Send Query Methods
		ReqlDriverError CXReQLQuery::CXReQLSendWithQueue(ReqlConnection* conn, void * msg, unsigned long * msgLength);
		uint64_t CXReQLRunQueryWithTokenOnQueue(ReqlConnection * conn, uint64_t queryToken);//, void * options)//, ReqlQueryClosure callback)
		
		
		template<typename CXReQLQueryClosure>
		uint64_t run(CXReQLConnection* conn, Value* options, CXReQLQueryClosure callback)
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

			conn->addQueryCallbackForKey(callback, expectedQueryToken);

			CXReQLRunQueryWithTokenOnQueue(conn->connection(), expectedQueryToken);

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
	};


	typedef CXReQLQuery* (*CXReQLStringQueryFunc) (const char * name);

//#define run(conn, options, callback) .runQuery(conn, options, [](void* arg){ (*static_cast<decltype(callback)*>(arg))(); }, &callback)

}



#endif