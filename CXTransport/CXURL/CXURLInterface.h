#pragma once
#ifndef CXURL_INTERFACE_H
#define CXURL_INTERFACE_H

//#include <CoreTransport/CoReQL.h>
//#include "CXURLRequest.h"
//#include "CXURLSession.h"
//#include "CXURLInterfaceMessageChannel.h"

//#define CXURLInterfaceShared CXURLInterface

namespace CoreTransport
{
	class CXURL_API CXURLInterface
	{
		public:
			static CXURLInterface *getInstance()
			{
#ifdef _WIN32 

#elif  defined(__GNUC__) && (__GNUC__ > 3)
				// You are OK
#else
#error Add Critical Section for your platform
#endif
				static CXURLInterface instance;// = NULL;
				//if (instance == NULL)
				//	instance = new CXURLInterface();

#ifdef _WIN32
				//END Critical Section Here
#endif 

				return &instance;
			}
		private:
			CXURLInterface() {}                    // Constructor? (the {} brackets) are needed here.

			// C++ 03
			// ========
			// Don't forget to declare these two. You want to make sure they
			// are unacceptable otherwise you may accidentally get copies of
			// your singleton appearing.
			CXURLInterface(CXURLInterface const&);              // Don't Implement
			void operator=(CXURLInterface const&);		// Don't implement

			// C++ 11
			// =======
			// We can use the better technique of deleting the methods
			// we don't want.
		public:
			//CXURLInterface(CXURLInterface const&) = delete;
			//void operator=(CXURLInterface const&) = delete;

			// Note: Scott Meyers mentions in his Effective Modern
			//       C++ book, that deleted functions should generally
			//       be public as it results in better error messages
			//       due to the compilers behavior to check accessibility
			//       before deleted status

			void doNothing() { return; }

			template<typename CXConnectionClosure>
			int connect(CTTarget* target, CXConnectionClosure callback)
			{
				CXURLSession* sharedSession = sharedSession->getInstance();
				return sharedSession->connect(target, callback);
			}


			//CXURLQuery& dbQuery(const char * name);

			//HTTP PROXY
			std::shared_ptr<CXURLRequest> OPTIONS(const char* requestPath, const char* responseFilePath) { std::shared_ptr<CXURLRequest> request(new CXURLRequest(URL_OPTIONS, requestPath, NULL, NULL, 0, responseFilePath)); return request; }
			std::shared_ptr<CXURLRequest> OPTIONS(const char* requestPath) { std::shared_ptr<CXURLRequest> request(new CXURLRequest(URL_OPTIONS, requestPath, NULL, NULL, 0)); return request; }

			//HTTP PROXY + TLS UPGRADE
			std::shared_ptr<CXURLRequest> CONNECT(const char* requestPath, const char* responseFilePath) { std::shared_ptr<CXURLRequest> request(new CXURLRequest(URL_CONNECT, requestPath, NULL, NULL, 0, responseFilePath)); return request; }
			std::shared_ptr<CXURLRequest> CONNECT(const char* requestPath) { std::shared_ptr<CXURLRequest> request(new CXURLRequest(URL_CONNECT, requestPath, NULL, NULL, 0)); return request; }

			//GET
			std::shared_ptr<CXURLRequest> GET(const char * requestPath, const char * responseFilePath) { std::shared_ptr<CXURLRequest> request(new CXURLRequest(URL_GET, requestPath, NULL, NULL, 0, responseFilePath )); return request; }
			std::shared_ptr<CXURLRequest> GET(const char * requestPath) { std::shared_ptr<CXURLRequest> request(new CXURLRequest(URL_GET, requestPath, NULL, NULL, 0 )); return request; }

			//POST
			std::shared_ptr<CXURLRequest> POST(const char* requestPath, const char* responseFilePath) { std::shared_ptr<CXURLRequest> request(new CXURLRequest(URL_POST, requestPath, NULL, NULL, 0, responseFilePath)); return request; }
			std::shared_ptr<CXURLRequest> POST(const char* requestPath) { std::shared_ptr<CXURLRequest> request(new CXURLRequest(URL_POST, requestPath, NULL, NULL, 0)); return request; }

	};

//#define GET(path) doNothing();\
//std::shared_ptr<CXURLRequest> request(new CXURLRequest(CTURL_GET, path, NULL, NULL, 0));\
//(*query)


static CXURLInterface * cxURL;
#define CXURL (*(cxURL->getInstance()))


}


#endif