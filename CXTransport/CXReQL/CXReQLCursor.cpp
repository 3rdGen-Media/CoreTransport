#include "../CXReQL.h"

using namespace CoreTransport;
char * CXReQLCursor::ProcessResponseHeader(char * buffer, unsigned long bufferLength)
{
	char* endOfHeader = buffer + sizeof(ReqlQueryMessageHeader);

	//The client *must* set the cursor's contentLength property
	//to aid CoreTransport in knowing when to stop reading from the socket
	_cursor.contentLength = ((ReqlQueryMessageHeader*)buffer)->length;
	uint64_t token = ((ReqlQueryMessageHeader*)buffer)->token;
	assert(token == _cursor.queryToken);

	//The cursor headerLength is calculated as follows after this function returns
	//cursor->headerLength = endOfHeader - buffer;
	return endOfHeader;
}


uint64_t CXReQLCursor::Continue()
{
	//CXReQLQuery continueQuery;
	//continueQuery.Continue(this, NULL, callback);

	return 0;
}