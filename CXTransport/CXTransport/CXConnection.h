#pragma once
#ifndef CXCONNECTION_H
#define CXCONNECTION_H

//#include <CoreTransport/CTSockets.h>
#include "CXCursor.h"
#include <map>
#include <functional>
#include <iostream>

namespace CoreTransport
{

//#define CX_PAGE_SIZE				4096L //only use this to hard code or test changes in page size
#define	CX_MAX_INFLIGHT_QUERIES	8L

#define CX_MAX_INFLIGHT_CONNECT_PACKETS 1L
#define CX_MAX_INFLIGHT_DECRYPT_PACKETS 2L
#define CX_MAX_INFLIGHT_ENCRYPT_PACKETS 1L

#define CX_QUERY_BUFF_PAGES		8L	//the number of system pages that will be used to calculate the size of a single query buffer
#define CX_REQUEST_BUFF_PAGE	CX_QUERY_BUFF_PAGES
#define CX_RESPONSE_BUFF_PAGES		8L	//the number of system pages that will be used to calculate the size of a single response buffer

//Note:  If you're buffer size is anywher close to MAX UNSIGNED LONG, you're network requests are too large...

//#define CX_QUERY_BUFF_SIZE		CX_PAGE_SIZE * 256L
//#define CX_RESPONSE_BUFF_SIZE	CX_PAGE_SIZE * 256L
static unsigned long					CX_QUERY_BUFF_SIZE		= 0;	//Query buffer size will be calculated dynamically based on the above macros
static unsigned long					CX_RESPONSE_BUFF_SIZE  = 0;	//Response buffer size will be calculated dynamically based on the above macros



	// Main entry point for our app. Connects the app with the Windows shell and handles application lifecycle events.
	class CXTRANSPORT_API CXConnection// : public Microsoft::WRL::RuntimeClass<ABI::Windows::ApplicationModel::Core::IFrameworkView>
	{
		public:
			CXConnection::CXConnection(CTConnection *conn, CTThreadQueue rxQueue, CTThreadQueue txQueue);
			~CXConnection(void);
			CTThreadQueue queryQueue();
			CTThreadQueue responseQueue();

			//template<typename CXRequestClosure>
			void addRequestCallbackForKey(std::function<void(CTError* error, std::shared_ptr<CXCursor> cxCursor)> const &callback, uint64_t requestToken) {_queries.insert(std::make_pair(requestToken, callback));}
			void removeRequestCallbackForKey(uint64_t requestToken) {_queries.erase(requestToken);}
			std::function<void(CTError* error, std::shared_ptr<CXCursor> cxCursor)> getRequestCallbackForKey(uint64_t requestToken) { return _queries.at(requestToken); }
			
			void addRequestCursorForKey(std::shared_ptr<CXCursor> &cursor, uint64_t requestToken) {_cursors.insert(std::make_pair(requestToken, cursor));}
			void removeRequestCursorForKey(uint64_t requestToken) {_cursors.erase(requestToken);}
			std::shared_ptr<CXCursor> getRequestCursorForKey(uint64_t requestToken) { return _cursors.size() > 0 ? _cursors.at(requestToken) : NULL; }
			std::shared_ptr<CXCursor> createRequestCursor(uint64_t queryToken);

			void printQueries()
			{

				std::map<uint64_t, std::function<void(CTError* error, std::shared_ptr<CXCursor> cxCursor)>>::iterator it = _queries.begin();
				std::map<uint64_t, std::function<void(CTError* error, std::shared_ptr<CXCursor> cxCursor)>>::iterator endIt = _queries.end();
				for( it; it!=endIt; ++it)
				{
					std::cout << "Existing Key (" << it->first << ')\n';
				}
				std::cout << "Existing Key (" << endIt->first << ')\n';
			}
			
			void distributeResponseWithCursorForToken(uint64_t requestToken);
			
			void close();
		//protected:

			CTConnection* connection() { return &_conn; };
						HANDLE			_hFile;
						HANDLE			_mFile;
						char *			_mFilePtr;
						off_t			_mFileSize;

	private:
			CTConnection	_conn;
			//void reserveConnectionMemory();
			CTDispatchSource startConnectionThreadQueues(CTThreadQueue rxQueue, CTThreadQueue txQueue);
			//unsigned long __stdcall EncryptQueryCallback(LPVOID lpParameter);
			//unsigned long __stdcall DecryptResponseCallback(LPVOID lpParameter);


			CTThreadQueue _rxQueue, _txQueue;
			//dispatch_source_t _source;

			//template<typename CXQueryClosure>
			//union{
				std::map<uint64_t, std::shared_ptr<CXCursor>>													_cursors;
				std::map<uint64_t, std::function<void(CTError* error, std::shared_ptr<CXCursor> cxCursor)>>    _queries;

					//std::map<uint64_t, std::function<void(CTError* error, CXCursor* cursor)>>    _requests;
			//};
			
			//overload virtual functions	
				virtual void ProcessProtocolHeader() { printf("CXConnection::ProcessProtocolHeader() Base Class Implementation should be overridden!\n"); }
	};

	typedef int(*CXConnectionClosure)(CTError *error, CXConnection *connection);
	//typedef void(*CTConnectionCallback)(CTError * err, CTConnection * conn);
	typedef void(*CXConnectFunc)(CTTarget *service, CXConnectionClosure *callback);
}

#endif