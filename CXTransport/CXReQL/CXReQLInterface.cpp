#include "../CXReQL.h"
#include <memory>

using namespace CoreTransport;


/*
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
*/


CXReQLQuery& CXReQLInterface::funcQuery(Value* args)
{
	//std::shared_ptr<CXReQLQuery> query(new CXReQLQuery(REQL_FUNC, args, NULL)); 
	CXReQLQuery* query = new  CXReQLQuery(REQL_FUNC, args, NULL);//, &jsstr(name), NULL) );
	return *query;
}

CXReQLQuery& CXReQLInterface::branchQuery(Value* args)
{
	//std::shared_ptr<CXReQLQuery> query(new CXReQLQuery(REQL_FUNC, args, NULL)); 
	CXReQLQuery* query = new  CXReQLQuery(REQL_BRANCH, args, NULL);//, &jsstr(name), NULL) );
	return *query;
}

CXReQLQuery& CXReQLInterface::dbQuery(const char* name)
{
	CXReQLQuery* query = new CXReQLQuery(REQL_DB);//, &jsstr(name), NULL) );

	//CXReQLQuery query(REQL_DB);// new CXReQLQuery(REQL_DB);

	Value dbNameJsonStr; \
		dbNameJsonStr = StringRef(name); \
		Value dbQueryArgs(kArrayType); \
		dbQueryArgs.PushBack(dbNameJsonStr, *query->getAllocator()); \
		query->setQueryArgs(&dbQueryArgs); \

		return *query;
}

CXReQLQuery& CXReQLInterface::eq(unsigned int varIndex, const char* bracketField, int* value)
{
	//[17, 
	//	[ [170, [[10, [3]], "inserted"] ], 1 ] 
	//] 

	CXReQLQuery* query = new CXReQLQuery(REQL_EQ);// , & eqArgs, NULL);// (*query)

	Value varValues(kArrayType);
	varValues.PushBack(Value(varIndex), *query->getAllocator());

	Value varsArray(kArrayType);
	varsArray.PushBack(Value(REQL_VAR), *query->getAllocator()).PushBack(varValues, *query->getAllocator());

	assert(bracketField);


	Value bracketArrayArgs(kArrayType);
	Value bracketArray(kArrayType);
	bracketArrayArgs.PushBack(varsArray, *query->getAllocator()).PushBack(StringRef(bracketField), *query->getAllocator());
	bracketArray.PushBack(Value(REQL_BRACKET), *query->getAllocator()).PushBack(bracketArrayArgs, *query->getAllocator());

	Value eqArgs(kArrayType);
	eqArgs.PushBack(bracketArray, *query->getAllocator());
	if (value)
		eqArgs.PushBack(Value(*value), *query->getAllocator());
	else
		eqArgs.PushBack(Value(), *query->getAllocator());
	query->setQueryArgs(&eqArgs);

	return *query;

}


CXReQLQuery& CXReQLInterface::eq(unsigned int varIndex, int* value)
{
	//[17,
	//	[[10,[0]],0]
	//]

	//ReqlQueryDocumentType queryDoc;

	CXReQLQuery* query = new CXReQLQuery(REQL_EQ);// , & eqArgs, NULL);// (*query)

	Value varValues(kArrayType);
	varValues.PushBack(Value(varIndex), *query->getAllocator());

	Value varsArray(kArrayType);
	varsArray.PushBack(Value(REQL_VAR), *query->getAllocator()).PushBack(varValues, *query->getAllocator());

	Value eqArgs(kArrayType);

	eqArgs.PushBack(varsArray, *query->getAllocator());
	if (value)
		eqArgs.PushBack(Value(*value), *query->getAllocator());
	else
		eqArgs.PushBack(Value(), *query->getAllocator());
	query->setQueryArgs(&eqArgs);

	return *query;
}


CXReQLQuery& CXReQLInterface::eq(Value * eqObj, const char * value, const char * value2)
{
	//[17,
	//	[[10,[0]], "value"]
	//]

	//ReqlQueryDocumentType queryDoc;

	CXReQLQuery* query = new CXReQLQuery(REQL_EQ);// , & eqArgs, NULL);// (*query)

	Value eqArgs(kArrayType);
	eqArgs.PushBack(*eqObj, *query->getAllocator());
	if (value)
		eqArgs.PushBack(StringRef(value), *query->getAllocator());
	else
		eqArgs.PushBack(Value(), *query->getAllocator());
	query->setQueryArgs(&eqArgs);

	return *query;
}


CXReQLQuery& CXReQLInterface::count(unsigned int varIndex)
{
	//[43,
	//	[[10,[0]]]
	//]

	//ReqlQueryDocumentType queryDoc;

	CXReQLQuery* query = new CXReQLQuery(REQL_COUNT);// , & eqArgs, NULL);// (*query)

	Value varValues(kArrayType);
	varValues.PushBack(Value(varIndex), *query->getAllocator());

	Value varsArray(kArrayType);
	varsArray.PushBack(Value(REQL_VAR), *query->getAllocator()).PushBack(varValues, *query->getAllocator());

	Value countArgs(kArrayType);

	countArgs.PushBack(varsArray, *query->getAllocator());
	//if (value)
	//	countArgs.PushBack(Value(*value), *query->getAllocator());
	//else
	//	countArgs.PushBack(Value(), *query->getAllocator());
	query->setQueryArgs(&countArgs);

	return *query;
}

/*
std::shared_ptr<CXReQLQuery> CXReQLInterface::dbQuery(const char * name)
{

	//because rapidjson requires passing an allocator, and our allocator lives on the CXReQLQuery object
	//we must construct the object first, instead of constructing with its arguments
	std::shared_ptr<CXReQLQuery> query(new CXReQLQuery(REQL_DB));//, &jsstr(name), NULL) );

	//CXReQLQuery query(REQL_DB);// new CXReQLQuery(REQL_DB);

	Value dbNameJsonStr; \
	dbNameJsonStr = StringRef(name); \
	Value dbQueryArgs(kArrayType); \
	dbQueryArgs.PushBack(dbNameJsonStr, *query->getAllocator()); \
	query->setQueryArgs(&dbQueryArgs); \
	//(*query)

	//CXReQLQuerySetContext(query);
	return query;
}
*/


CXReQLQuery& CXReQLInterface::doQuery(Value* args)
{
	CXReQLQuery* query = new CXReQLQuery(REQL_DO, args, NULL);
	return *query;

}
