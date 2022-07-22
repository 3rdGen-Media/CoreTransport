#pragma once
#ifndef CXREQL_INTERFACE_H
#define CXREQL_INTERFACE_H

//#include <CoreTransport/CoReQL.h>
//#include "CXReQLQuery.h"
//#include "CXReQLSession.h"
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

			template<typename CXConnectionClosure>
			int connect(CTTarget* target, CXConnectionClosure callback)
			{
				CXReQLSession* sharedSession = sharedSession->getInstance();
				return sharedSession->connect(target, callback);
			}

			//REQL_DB
			CXReQLQuery& dbQuery(const char* name);
			//std::shared_ptr<VTReQLQuery> dbQuery(const char * name);

			//REQL_DO
			//VTReQLQuery& doQuery(VTReQLQuery& args);
			CXReQLQuery& doQuery(Value* args);

			//REQL_FUNC
			CXReQLQuery& funcQuery(Value* args);

			//REQL_BRANCH
			CXReQLQuery& branchQuery(Value* args);

			//REQL_EQ
			CXReQLQuery& eq(unsigned int varIndex, int* value);
			CXReQLQuery& eq(unsigned int varIndex, const char* bracketField, int* value);
			CXReQLQuery& eq(Value* eqObj, const char* value, const char * value2);

			//REQL_COUNT
			CXReQLQuery& count(unsigned int varIndex);

	};

	//doNothing(); Value dbNameJsonStr; dbNameJsonStr = StringRef(name); std::shared_ptr<CXReQLQuery> query(new CXReQLQuery(REQL_DB, &dbNameJsonStr, NULL)); (*query)

	/*
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
*/
static CXReQLInterface * cxReQL;
#define CXReQL (*(cxReQL->getInstance()))

//static CXReQLInterface* cxReQL;
//static inline CXReQLInterface& CXReQLSharedInterface() { return *(cxReQL->getInstance()); }

//#define CXReQL CXReQLSharedInterface()
	//#define connect(service, callback) asyncConnect(service, callback))
	//#define connect(service, callback, options) asyncConnect(service, callback))
	//#define db(name) dbQuery(name)


#define db(name) doNothing();\
std::shared_ptr<CXReQLQuery> query(new CXReQLQuery(REQL_DB));\
Value dbNameJsonStr; \
dbNameJsonStr = StringRef(name); \
Value dbQueryArgs(kArrayType); \
dbQueryArgs.PushBack(dbNameJsonStr, *query->getAllocator()); \
query->setQueryArgs(&dbQueryArgs); \
(*query)


#define do(args) doNothing();std::shared_ptr<CXReQLQuery> query(new CXReQLQuery(REQL_DO, args, NULL));(*query)





}


#endif