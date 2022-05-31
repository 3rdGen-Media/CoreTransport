#include "../CXURL.h"

using namespace CoreTransport;
char * CXURLCursor::ProcessResponseHeader(CTCursor * cursor, char * buffer, unsigned long bufferLength)
{
	char* endOfHeader = strstr(buffer, "\r\n\r\n");

	if (!endOfHeader)
		return NULL;// assert(1 == 0);

	//set ptr to end of http header
	endOfHeader += sizeof("\r\n\r\n") - 1;

	//calculate size of header
	_cursor->headerLength = endOfHeader - buffer;
	//_headerLength = endOfHeader - buffer;


	_cursor->contentLength = 0;// 1641845;
	char* contentLengthPtr = strstr(buffer, "Content-Length: ");

	if (contentLengthPtr)
	{
		char* contentLengthStart = contentLengthPtr + 16;
		char* contentLengthEnd  = strstr(contentLengthStart, "\r\n");
		assert(contentLengthEnd);

		char cLength[32] = "\0";
		memcpy(cLength, contentLengthStart, contentLengthEnd - contentLengthStart);
		cLength[contentLengthEnd - contentLengthStart] = '\0';

		_cursor->contentLength = (unsigned long)atoi((const char*)cLength);
	}
	
	return endOfHeader;
}
