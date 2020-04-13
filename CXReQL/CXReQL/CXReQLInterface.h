#pragma once
#ifndef CXREQL_INTERFACE_H
#define CXREQL_INTERFACE_H

//#include <CoreTransport/CoReQL.h>
#include "CXReQLConnection.h"
#include "CXReQLQuery.h"
#include "CXReQLSession.h"
//#include "CXReQLInterfaceMessageChannel.h"

//#define CXReQLInterfaceShared CXReQLInterface

namespace CoreTransport
{
	class CXREQL_API CXReQLInterface
	{
		public:
			static CXReQLInterface *getInstance()
			{
#ifdef _WIN32 

#elif  defined(__GNUC__) && (__GNUC__ > 3)
				// You are OK
#else
#error Add Critical Section for your platform
#endif
				static CXReQLInterface instance;// = NULL;
				//if (instance == NULL)
				//	instance = new CXReQLInterface();

#ifdef _WIN32
				//END Critical Section Here
#endif 

				return &instance;
			}
		private:
			CXReQLInterface() {}                    // Constructor? (the {} brackets) are needed here.

			// C++ 03
			// ========
			// Don't forget to declare these two. You want to make sure they
			// are unacceptable otherwise you may accidentally get copies of
			// your singleton appearing.
			CXReQLInterface(CXReQLInterface const&);              // Don't Implement
			void operator=(CXReQLInterface const&);		// Don't implement

			// C++ 11
			// =======
			// We can use the better technique of deleting the methods
			// we don't want.
		public:
			//CXReQLInterface(CXReQLInterface const&) = delete;
			//void operator=(CXReQLInterface const&) = delete;

			// Note: Scott Meyers mentions in his Effective Modern
			//       C++ book, that deleted functions should generally
			//       be public as it results in better error messages
			//       due to the compilers behavior to check accessibility
			//       before deleted status

			void doNothing() { return; }
			void connect(ReqlService * service, CXReQLConnectionClosure callback);

			CXReQLQuery& dbQuery(const char * name);
	};

	//doNothing(); Value dbNameJsonStr; dbNameJsonStr = StringRef(name); std::shared_ptr<CXReQLQuery> query(new CXReQLQuery(REQL_DB, &dbNameJsonStr, NULL)); (*query)
#define db(name) doNothing();\
std::shared_ptr<CXReQLQuery> query(new CXReQLQuery(REQL_DB));\
Value dbNameJsonStr;\
dbNameJsonStr = StringRef(name);\
Value dbQueryArgs(kArrayType);\
dbQueryArgs.PushBack(dbNameJsonStr, *query->getAllocator());\
query->setQueryArgs(&dbQueryArgs);\
(*query)

static CXReQLInterface * cxReQL;
#define CXReQL (*(cxReQL->getInstance()))


}


#endif