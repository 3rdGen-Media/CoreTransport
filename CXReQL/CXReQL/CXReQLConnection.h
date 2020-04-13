#pragma once
#ifndef CXREQL_CONNECTION_H
#define CXREQL_CONNECTION_H

//#include <CoreTransport/ctReQL.h>
#include "CXReQLCursor.h"
#include <map>
#include <functional>
#include <iostream>

namespace CoreTransport
{

//#define CX_REQL_PAGE_SIZE				4096L //only use this to hard code or test changes in page size
#define	CX_REQL_MAX_INFLIGHT_QUERIES	8L

#define CX_REQL_QUERY_BUFF_PAGES		8L	//the number of system pages that will be used to calculate the size of a single query buffer
#define CX_REQL_RESPONSE_BUFF_PAGES		8L	//the number of system pages that will be used to calculate the size of a single response buffer

//Note:  If you're buffer size is anywher close to MAX UNSIGNED LONG, you're network requests are too large...

//#define CX_REQL_QUERY_BUFF_SIZE		CX_REQL_PAGE_SIZE * 256L
//#define CX_REQL_RESPONSE_BUFF_SIZE	CX_REQL_PAGE_SIZE * 256L
static unsigned long					CX_REQL_QUERY_BUFF_SIZE		= 0;	//Query buffer size will be calculated dynamically based on the above macros
static unsigned long					CX_REQL_RESPONSE_BUFF_SIZE  = 0;	//Response buffer size will be calculated dynamically based on the above macros



	// Main entry point for our app. Connects the app with the Windows shell and handles application lifecycle events.
	class CXREQL_API CXReQLConnection// : public Microsoft::WRL::RuntimeClass<ABI::Windows::ApplicationModel::Core::IFrameworkView>
	{
		public:
			CXReQLConnection::CXReQLConnection(ReqlConnection *conn, ReqlThreadQueue rxQueue, ReqlThreadQueue txQueue);
			~CXReQLConnection(void);
			ReqlThreadQueue queryQueue();
			ReqlThreadQueue responseQueue();

			//template<typename CXReQLQueryClosure>
			void addQueryCallbackForKey(std::function<void(ReqlError* error, CXReQLCursor* cursor)> const &callback, uint64_t queryToken) {_queries.insert(std::make_pair(queryToken, callback));}
			void removeQueryCallbackForKey(uint64_t queryToken) {_queries.erase(queryToken);}
			std::function<void(ReqlError* error, CXReQLCursor* cursor)> getQueryCallbackForKey(uint64_t queryToken) { return _queries.at(queryToken); }

			void printQueries()
			{

				std::map<uint64_t, std::function<void(ReqlError* error, CXReQLCursor* cursor)>>::iterator it = _queries.begin();
				std::map<uint64_t, std::function<void(ReqlError* error, CXReQLCursor* cursor)>>::iterator endIt = _queries.end();
				for( it; it!=endIt; ++it)
				{
					std::cout << "Existing Key (" << it->first << ')\n';
				}
				std::cout << "Existing Key (" << endIt->first << ')\n';
			}

			void distributeMessageWithCursor(char * msg, unsigned long msgLength);
			void close();
		//protected:

			ReqlConnection* connection() { return &_conn; };
		private:
			ReqlConnection	_conn;

			void reserveConnectionMemory();
			ReqlDispatchSource startConnectionThreadQueues(ReqlThreadQueue rxQueue, ReqlThreadQueue txQueue);
			//unsigned long __stdcall EncryptQueryCallback(LPVOID lpParameter);
			//unsigned long __stdcall DecryptResponseCallback(LPVOID lpParameter);


			ReqlThreadQueue _rxQueue, _txQueue;
			//dispatch_source_t _source;

			//template<typename CXReQLQueryClosure>
			std::map<uint64_t, std::function<void(ReqlError* error, CXReQLCursor* cursor)>>    _queries;
	};

	typedef int(*CXReQLConnectionClosure)(ReqlError *error, CXReQLConnection *connection);
	//typedef void(*ReqlConnectionCallback)(ReqlError * err, ReqlConnection * conn);
	typedef void(*CXReQLConnectFunc)(ReqlService *service, CXReQLConnectionClosure *callback);
}

#endif