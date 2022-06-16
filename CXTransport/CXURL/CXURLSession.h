#pragma once
#ifndef CXURLSESSION_H
#define CXURLSESSION_H

//#include "CXReQLConnection.h"
#include <map>

namespace CoreTransport
{
	//A Session Object manages a collection of connections
	class CXURLSession
	{
	public:
		static CXURLSession *getInstance()
		{
#ifdef _WIN32 

#elif  defined(__GNUC__) && (__GNUC__ > 3)
			// You are OK
#else
#error Add Critical Section for your platform
#endif
			static CXURLSession  instance;// = NULL;
			//if (instance == NULL)
			//	instance = new CXURLSession();

#ifdef _WIN32
			//END Critical Section Here
#endif 

			return &instance;
		}
	private:
		CXURLSession();// {}                    // Constructor? (the {} brackets) are needed here.
		~CXURLSession();// {}                    // Constructor? (the {} brackets) are needed here.

		// C++ 03
		// ========
		// Don't forget to declare these two. You want to make sure they
		// are unacceptable otherwise you may accidentally get copies of
		// your singleton appearing.
		CXURLSession(CXURLSession const&);              // Don't Implement
		void operator=(CXURLSession const&);			  // Don't implement

		//static int _CXURLSessionConnectionCallback(CTError* err, CTConnection * conn);

		// C++ 11
		// =======
		// We can use the better technique of deleting the methods
		// we don't want.

		void addPendingConnection(CTTarget* target, std::pair<CXConnectionClosureFunc, void*> targetCallbackContext);
		void removeConnection(CTTarget *target);
		
		std::map<CTTarget*, std::pair<CXConnectionClosureFunc, void*>>		_pendingConnections;
		std::map<CTTarget*, CXConnection*>										_connections;
	public:
		//CXURLSession(CXURLSession const&) = delete;
		//void operator=(CXURLSession const&) = delete;

		// Note: Scott Meyers mentions in his Effective Modern
		//       C++ book, that deleted functions should generally
		//       be public as it results in better error messages
		//       due to the compilers behavior to check accessibility
		//       before deleted status


		//these have to be public so they can be called from the static ReQLC PI callback
		//but they should be updated to be handled for outside calls from the client, which should be allowed...
		void removePendingConnection(CTTarget* target);
		void addConnection(CTTarget *target, CXConnection * cxConnection);

		std::pair<CXConnectionClosureFunc, void*> pendingConnectionForKey(CTTarget * target);


		static int _CXURLSessionConnectionCallback(CTError* err, CTConnection* conn)
		{
			//conn should always exist, so it can contain the input CTTarget pointer
			//which contains the client context
			assert(conn);

			//Return variables
			CXConnectionClosureFunc callback;
			CXConnection* cxConnection = NULL;

			//Retrieve CXURLSession caller object from ReQLService input client context property
			CXURLSession* cxURLSession = (CXURLSession*)(conn->target->ctx);

			//retrieve the client connection callback and target ctx (replace target context on target)
			std::pair<CXConnectionClosureFunc, void*> targetCallbackContext = cxURLSession->pendingConnectionForKey(conn->target);
			callback = targetCallbackContext.first;
			conn->target->ctx = targetCallbackContext.second;

			//  Complete the RethinkDB 2.3+ connection: Perform RethinkDB ReQL SASL Layer Auth Handshake (using SCRAM HMAC SHA-256 Auth) over TCP
			//err->id = CTReQLSASLHandshake(conn, conn->target);

			//Parse any errors
			if (err->id != CTSuccess)
			{
				//The connection failed
				//parse the Error and bail out
				assert(1 == 0);

			}
			else //Handle successfull connection!!!
			//if (conn)
			{
				//For WIN32 each CTConnection gets its own pair of tx/rx iocp queues internal to the ReQL C API (but the client must assign thread(s) to these)
				//For Darwin + GCD, these queue handles are currently owned by NSReQLSession and still need to be moved into ReQLConnection->socketContext	
				//*Note:  On platforms where the SSL Context is not thread safe, such as Darwin w/ SecureTransport, we need to manually specify the same [GCD] thread queue for send and receive
				//because the SSL Context API (SecureTransport) is not thread safe when reading and writing simultaneously

				//if the client didn't supply tx/rx thread queues, create them now because CXConnection object requires them for initialization
				if (!conn->socketContext.txQueue)
				{
					if (CTSocketCreateEventQueue(&(conn->socketContext)) < (CTKernelQueue)0)
					{
						fprintf(stderr, "_CXURLSessionConnectionCallback::CTSocketCreateEventQueue failed\n");
						err->id = (int)conn->event_queue;
					}
				}

				assert(conn->socketContext.rxQueue);
				assert(conn->socketContext.txQueue);

				//Wrap the CTConnection in a C++ CXConnection Object
				//A CXConnection gets created with a CTConnection (socket + ssl context) and a dedicated thread queue for reading from the connection's socket
				cxConnection = new CXConnection(conn, conn->socketContext.rxQueue, conn->socketContext.txQueue);

				//  Store CXConnection in internal NXReQLSession connections array/map/hash or whatever
				if (cxConnection) cxURLSession->addConnection(conn->target, cxConnection);
				//  Add to error connections for retry?
				//else

			}

			//  Always remove pending connections
			cxURLSession->removePendingConnection(conn->target);

			//issue the client callback
			callback(err, cxConnection);

			return err->id;
		}

		template<typename CXConnectionClosure>
		void connect(CTTarget* target, CXConnectionClosure callback)
		{
#ifdef _WIN32
			//Create a connection key for caching the connection and add to hash as a pending connection

			//convert port short to c string
			char port[6];
			_itoa((int)target->url.port, port, 10);

			//hijack the input target ctx variable
			void* clientCtx = target->ctx;
			target->ctx = this;

			//create connection key
			std::string connectionKey(target->url.host);
			connectionKey.append(":");
			connectionKey.append(port);

			//wrap ReQLService c struct ptr in CXReQLService object
			//CXReQLService * cxService = new CXReQLService(target);

			//Add target to map of pending connections
			std::function< void(CTError* err, CXConnection* conn) > callbackFunc = callback;
			addPendingConnection(target, std::make_pair(callbackFunc, clientCtx));


			//Issue the connection using CTC API 
			CTransport.connect(target, _CXURLSessionConnectionCallback);

#elif defined(__APPLE__ )


#endif
			return;
		}

	};

}

#endif