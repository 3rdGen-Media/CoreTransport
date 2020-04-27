#include "../CXReQL.h"
#include <memory>

using namespace CoreTransport;

void CXReQLInterface::connect(ReqlService * service, CXConnectionClosure callback)
{
	CXReQLSession * sharedSession = sharedSession->getInstance();
	sharedSession->connect(service, callback);
	//return CXReQLInterfaceSession.connect;
}

CXReQLQuery& CXReQLInterface::dbQuery(const char * name)
{
	//because rapidjson requires passing an allocator, and our allocator lives on the CXReQLQuery object
	//we must construct the object first, instead of constructing with its arguments
	std::shared_ptr<CXReQLQuery> query(new CXReQLQuery(REQL_DB));//, &jsstr(name), NULL) );

	Value dbNameJsonStr;
	dbNameJsonStr = StringRef(name);
	Value dbQueryArgs(kArrayType);
	dbQueryArgs.PushBack(dbNameJsonStr, *query->getAllocator());
	//std::shared_ptr<CXReQLQuery> query(new CXReQLQuery(REQL_DB, &dbNameJsonStr, NULL));//, &jsstr(name), NULL) );
	query->setQueryArgs(&dbQueryArgs);
	//query->setOptions(&dbQueryOptions);

	//CXReQLQuerySetContext(query);
	return *query;
}
