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

		void addPendingConnection(CTTarget* target, std::pair<CXConnectionClosure, void*> &targetCallbackContext);
		void removeConnection(CTTarget *target);

		std::map<CTTarget*, std::pair<CXConnectionClosure, void*>>		_pendingConnections;
		std::map<CTTarget*, CXConnection*>								_connections;
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

		std::pair<CXConnectionClosure, void*> pendingConnectionForKey(CTTarget * target);

		void connect(CTTarget* target, CXConnectionClosure callback);
	};

}

#endif