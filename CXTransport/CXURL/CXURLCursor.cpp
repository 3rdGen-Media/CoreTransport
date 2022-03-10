#include "../CXURL.h"

using namespace CoreTransport;
char * CXURLCursor::ProcessResponseHeader(char * buffer, unsigned long bufferLength)
{
	char* endOfHeader = strstr(buffer, "\r\n\r\n");

	if (!endOfHeader)
		return NULL;// assert(1 == 0);

	//set ptr to end of http header
	endOfHeader += sizeof("\r\n\r\n") - 1;

	//calculate size of header
	_cursor.headerLength = endOfHeader - buffer;
	//_headerLength = endOfHeader - buffer;


	_cursor.contentLength = 1256;// 1641845;

	return endOfHeader;
}
