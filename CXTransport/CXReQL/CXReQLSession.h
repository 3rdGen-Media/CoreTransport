#pragma once
#ifndef CXREQL_SESSION_H
#define CXREQL_SESSION_H

#include <map>

namespace CoreTransport
{

	//A Session Object manages a collection of connections
	class CXREQL_API CXReQLSession
	{
		//int _CXReQLSessionConnectionCallback(CTError* err, CTConnection* conn);

	public:
		static CXReQLSession *getInstance()
		{
#ifdef _WIN32 

#elif  defined(__GNUC__) && (__GNUC__ > 3)
			// You are OK
#else
#error Add Critical Section for your platform
#endif
			static CXReQLSession  instance;// = NULL;
			//if (instance == NULL)
			//	instance = new CXReQLSession();

#ifdef _WIN32
			//END Critical Section Here
#endif 

			return &instance;
		}
	private:
		CXReQLSession();// {}                    // Constructor? (the {} brackets) are needed here.
		~CXReQLSession();// {}                    // Constructor? (the {} brackets) are needed here.

		// C++ 03
		// ========
		// Don't forget to declare these two. You want to make sure they
		// are unacceptable otherwise you may accidentally get copies of
		// your singleton appearing.
		CXReQLSession(CXReQLSession const&);              // Don't Implement
		void operator=(CXReQLSession const&);			  // Don't implement

		//static int _CXReQLSessionConnectionCallback(ReqlError* err, CTConnection * conn);

		// C++ 11
		// =======
		// We can use the better technique of deleting the methods
		// we don't want.

		void addPendingConnection(ReqlService* service, std::pair<CXConnectionClosureFunc, void*> serviceCallbackContext);
		void removeConnection(ReqlService *service);

		std::map<ReqlService*, std::pair<CXConnectionClosureFunc, void*>>		_pendingConnections;
		std::map<ReqlService*, CXConnection*>									_connections;
	public:
		//CXReQLSession(CXReQLSession const&) = delete;
		//void operator=(CXReQLSession const&) = delete;

		// Note: Scott Meyers mentions in his Effective Modern
		//       C++ book, that deleted functions should generally
		//       be public as it results in better error messages
		//       due to the compilers behavior to check accessibility
		//       before deleted status


		//these have to be public so they can be called from the static ReQLC PI callback
		//but they should be updated to be handled for outside calls from the client, which should be allowed...
		void removePendingConnection(ReqlService* service);
		void addConnection(ReqlService *service, CXConnection * cxConnection);

		std::pair<CXConnectionClosureFunc, void*> pendingConnectionForKey(ReqlService* service);


		//template<typename CXConnectionClosure>
		CTConnectionClosure _CXReQLSessionHandshakeCallback = ^ int(CTError * err, ReqlConnection * conn)
			//static int _CXReQLSessionHandshakeCallback(CTError* err, ReqlConnection* conn)
		{
			//conn should always exist, so it can contain the input CTTarget pointer
			//which contains the client context
			assert(conn);

			//Return variables
			CXConnectionClosureFunc callback;
			CXConnection* cxConnection = NULL;

			//Retrieve CXReQLSession caller object from ReQLService input client context property
			CXReQLSession* cxReQLSession = (CXReQLSession*)(conn->target->ctx);

			//Parse any errors
			if (err->id != CTSuccess)
			{
				//The handshake failed
				//parse the Error and bail out
				fprintf(stderr, "_CXReQLSessionHandshakeCallback::CTReQLAsyncLHandshake failed!\n");
				err->id = CTSASLHandshakeError;
				assert(1 == 0);

			}
			else //Handle successfull connection!!!
			{
				//For WIN32 each CTConnection gets its own pair of tx/rx iocp queues internal to the ReQL C API (but the client must assign thread(s) to these)
				//For Darwin + GCD, these queue handles are currently owned by NSReQLSession and still need to be moved into ReQLConnection->socketContext	
				//*Note:  On platforms where the SSL Context is not thread safe, such as Darwin w/ SecureTransport, we need to manually specify the same [GCD] thread queue for send and receive
				//because the SSL Context API (SecureTransport) is not thread safe when reading and writing simultaneously

				//Wrap the CTConnection in a C++ CXConnection Object
				//A CXConnection gets created with a CTConnection (socket + ssl context) and a dedicated thread queue for reading from the connection's socket
				cxConnection = new CXConnection(conn, conn->socketContext.rxQueue, conn->socketContext.txQueue);

				//  Store CXConnection in internal NXReQLSession connections array/map/hash or whatever
				if (cxConnection) cxReQLSession->addConnection(conn->target, cxConnection);
				//  Add to error connections for retry?
			}

			//retrieve the client connection callback and target ctx (replace target context on target)
			std::pair<CXConnectionClosureFunc, void*> targetCallbackContext = cxReQLSession->pendingConnectionForKey(conn->target);
			callback = targetCallbackContext.first;
			conn->target->ctx = targetCallbackContext.second;

			//  Always remove pending connections
			cxReQLSession->removePendingConnection(conn->target);

			//issue the client callback
			callback(err, cxConnection);

			return err->id;

		};

		CTConnectionClosure _CXReQLSessionConnectionCallback = ^int(CTError * err, ReqlConnection * conn)
			//static int _CXReQLSessionConnectionCallback(CTError* err, CTConnection* conn)
		{
			//conn should always exist, so it can contain the input CTTarget pointer
			//which contains the client context
			assert(conn);

			int status = CTSuccess;

			//Return variables
			CXConnectionClosureFunc callback;
			CXConnection* cxConnection = NULL;

			//Retrieve CXReQLSession caller object from ReQLService input client context property
			CXReQLSession* cxReQLSession = (CXReQLSession*)(conn->target->ctx);

			//Parse any errors
			if (err->id != CTSuccess)
			{
				//The connection failed
				//parse the Error and bail out
				assert(1 == 0);

			}
			else //Handle successfull connection!!!
			{
				//Do the CTransport C-Style ReQL Handshake 
				// Complete the RethinkDB 2.3+ connection: Perform RethinkDB ReQL SASL Layer Auth Handshake (using SCRAM HMAC SHA-256 Auth) over TCP
				if (conn->target->cxQueue)//&& ((status = CTReQLAsyncHandshake(conn, conn->target, _CXReQLSessionHandshakeCallback)) != CTSuccess))
				{
					if ((status = CTReQLAsyncHandshake(conn, conn->target, _CXReQLSessionHandshakeCallback)) != CTSuccess)
						assert(1 == 0);

					return err->id;
				}
				else if ((status = CTReQLHandshake(conn, conn->target)) == CTSuccess)
				{
					//For WIN32 each CTConnection gets its own pair of tx/rx iocp queues internal to the ReQL C API (but the client must assign thread(s) to these)
					//For Darwin + GCD, these queue handles are currently owned by NSReQLSession and still need to be moved into ReQLConnection->socketContext	
					//*Note:  On platforms where the SSL Context is not thread safe, such as Darwin w/ SecureTransport, we need to manually specify the same [GCD] thread queue for send and receive
					//because the SSL Context API (SecureTransport) is not thread safe when reading and writing simultaneously

					//Wrap the CTConnection in a C++ CXConnection Object
					//A CXConnection gets created with a CTConnection (socket + ssl context) and a dedicated thread queue for reading from the connection's socket
					cxConnection = new CXConnection(conn, conn->socketContext.rxQueue, conn->socketContext.txQueue);

					//  Store CXConnection in internal NXReQLSession connections array/map/hash or whatever
					if (cxConnection) cxReQLSession->addConnection(conn->target, cxConnection);
					//  Add to error connections for retry?

				}
				else
				{
					fprintf(stderr, "_CXReQLSessionConnectionCallback::CTReQLSASLHandshake failed!\n");
					//CTCloseConnection(&_reqlConn);
					//CTSSLCleanup();
					err->id = CTSASLHandshakeError;
					assert(1 == 0);
				}



			}

			//retrieve the client connection callback and target ctx (replace target context on target)
			std::pair<CXConnectionClosureFunc, void*> targetCallbackContext = cxReQLSession->pendingConnectionForKey(conn->target);
			callback = targetCallbackContext.first;
			conn->target->ctx = targetCallbackContext.second;

			//  Always remove pending connections
			cxReQLSession->removePendingConnection(conn->target);

			//issue the client callback
			callback(err, cxConnection);

			return err->id;
		};


		template<typename CXConnectionClosure>
		void connect(ReqlService* service, CXConnectionClosure callback)
		{
#ifdef _WIN32
			//Create a connection key for caching the connection and add to hash as a pending connection

			//convert port short to c string
			char port[6];
			_itoa((int)service->url.port, port, 10);

			//hijack the input target ctx variable
			void* clientCtx = service->ctx;
			service->ctx = this;

			//create connection key
			std::string connectionKey(service->url.host);
			connectionKey.append(":");
			connectionKey.append(port);

			//wrap ReQLService c struct ptr in CXReQLService object
			//CXReQLService * cxService = new CXReQLService(target);

			//Add target to map of pending connections
			std::function< void(CTError* err, CXConnection* conn) > callbackFunc = callback;
			addPendingConnection(service, std::make_pair(callbackFunc, clientCtx));


			//Issue the connection using CTC API 
			CTransport.connect(service, _CXReQLSessionConnectionCallback);

#elif defined(__APPLE__ )


#endif
			return;
		}
	};

}


#endif