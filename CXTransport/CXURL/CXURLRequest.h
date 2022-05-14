#pragma once
#ifndef CXURLREQUEST_H
#define CXURLREQUEST_H

//#include <CoreTransport/CTSockets.h>
//#include "CXReQLCursor.h"
//#include "CXReQLConnection.h"


#include <map>


//#include "rapidjson/document.h"
//#include "rapidjson/error/en.h"
//#include "rapidjson/stringbuffer.h"
//#include <rapidjson/writer.h>

//using namespace rapidjson;
//define the type of document and allocators that rapidjson will use for building a DOM object
//typedef GenericDocument<UTF8<>, MemoryPoolAllocator<>> ReqlQueryDocumentType;
//typedef GenericStringBuffer<UTF8<>, MemoryPoolAllocator<>> ReqlQueryStringBufferType;


#define CXURLREQUEST_STATIC_BUFF_SIZE 32768L
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

	#define CXURL_printf(f_, ...) printf((f_), __VA_ARGS__)
	//typedef void(*CXURLRequestClosure) (ReqlError * error, CXReQLCursor * cursor);

	class CXURL_API CXURLRequest
	{
	public:

		CXURLRequest(URLRequestType command, const char * requestPath, std::map<char*, char*> *headers, char * body, unsigned long contentLength);		//CXURLRequest(ReqlTermType command, Value* args, Value* options);
		CXURLRequest(URLRequestType command, const char * requestPath, std::map<char*, char*> *headers, char * body, unsigned long contentLength, const char * responsePath);		//CXURLRequest(ReqlTermType command, Value* args, Value* options);

		~CXURLRequest(void);

		void doNothing() { return; }

		//initializers
		//void setQueryArgs(Value * args);
		//void setOptions(Value * args);



		//Send Query Methods
		CTClientError SendWithQueue(CTConnection* conn, void * msg, unsigned long * msgLength);
		uint64_t SendRequestWithCursorOnQueue(std::shared_ptr<CXCursor> cxCursor, uint64_t requestToken);//, void * options)//, ReqlQueryClosure callback)
		
		
		//MemoryPoolAllocator<>* getAllocator() {return _domValueAllocator; }

		void setValueForHTTPHeaderField(char * value, char * field);
		void setContentValue(char* value, unsigned long contentLength);

		template<typename CXRequestClosure>
		uint64_t send(CXConnection * conn, CXRequestClosure callback)
		{ 
			uint64_t expectedQueryToken = conn->connection()->requestCount++;
			
			//create a CXURL cursor (for the response, but also it holds our request buffer)
			//std::shared_ptr<CXURLCursor> cxURLCursor = conn->createRequestCursor(expectedQueryToken);
			std::shared_ptr<CXURLCursor> cxURLCursor( new CXURLCursor(conn, _filepath) );
			conn->addRequestCursorForKey(std::static_pointer_cast<CXCursor>(cxURLCursor), expectedQueryToken);
			conn->addRequestCallbackForKey(callback, expectedQueryToken);
			return SendRequestWithCursorOnQueue(cxURLCursor, expectedQueryToken);
			//return SendRequestWithTokenOnQueue(conn->connection(), expectedQueryToken ); 
		}
	protected:


	private:
		//ReqlService * _service;
		//ReqlTermType						_command;
		//ReqlQueryDocumentType*				_queryArray;
		//ReqlQueryStringBufferType*			_parseStringBuffer;
		//Writer<ReqlQueryStringBufferType>*	_domWriter;//(parseStringBuffer);

		//char								_domValueMemoryPool[CXURLREQUEST_QUERY_STATIC_BUFF_SIZE];
		//MemoryPoolAllocator<>*				_domValueAllocator;//(_queryBuffer, CX_REQL_QUERY_STATIC_BUFF_SIZE);

		//Value *_args;
		//Value *_options;

		URLRequestType			_command;
		char *					_requestPath;
		std::map<char*, char*>	_headers;
		char *					_body;
		unsigned long			_contentLength;

		//CTOverlappedResponse	_overlappedResponse;

		const char *					_filepath;
	};


	typedef CXURLRequest* (*CXURLStringRequestFunc) (const char * name);

//#define run(conn, options, callback) .runQuery(conn, options, [](void* arg){ (*static_cast<decltype(callback)*>(arg))(); }, &callback)

}



#endif