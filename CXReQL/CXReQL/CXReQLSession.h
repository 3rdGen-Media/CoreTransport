#pragma once
#ifndef CXREQL_SESSION_H
#define CXREQL_SESSION_H

#include "CXReQLConnection.h"
#include <map>

namespace CoreTransport
{
	//A Session Object manages a collection of connections
	class CXReQLSession
	{
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

		//static int _CXReQLSessionConnectionCallback(ReqlError* err, ReqlConnection * conn);

		// C++ 11
		// =======
		// We can use the better technique of deleting the methods
		// we don't want.

		void addPendingConnection(ReqlService* service, std::pair<CXReQLConnectionClosure, void*> &serviceCallbackContext);
		void removeConnection(ReqlService *service);

		std::map<ReqlService*, std::pair<CXReQLConnectionClosure, void*>>		_pendingConnections;
		std::map<ReqlService*, CXReQLConnection*>								_connections;
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
		void addConnection(ReqlService *service, CXReQLConnection * cxConnection);

		std::pair<CXReQLConnectionClosure, void*> pendingConnectionForKey(ReqlService* service);

		void connect(ReqlService * service, CXReQLConnectionClosure callback);
	};

}

#endif