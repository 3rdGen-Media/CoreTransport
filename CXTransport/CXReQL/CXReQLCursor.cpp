#include "../CXReQL.h"

using namespace CoreTransport;
char * CXReQLCursor::ProcessResponseHeader(char * buffer, unsigned long bufferLength)
{
	_cursor.headerLength = sizeof(ReqlQueryMessageHeader);
	char * endOfHeader = buffer + _cursor.headerLength;
	return endOfHeader;
}
