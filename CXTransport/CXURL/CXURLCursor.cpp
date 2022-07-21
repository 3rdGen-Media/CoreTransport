#include "../CXURL.h"

using namespace CoreTransport;
char * CXURLCursor::ProcessResponseHeader(CTCursor * cursor, char * buffer, unsigned long bufferLength)
{
	assert(_cursor == cursor);
	char* endOfHeader = strstr(buffer, "\r\n\r\n");

	if (!endOfHeader)
	{
		cursor->headerLength = 0;
		cursor->contentLength = 0;
		return NULL;
	}

	//set ptr to end of http header
	endOfHeader += sizeof("\r\n\r\n") - 1;

	//calculate size of header
	//_cursor->headerLength = endOfHeader - buffer; //why am I doing this?

	//The client *must* set the cursor's contentLength property
	//to aid CoreTransport in knowing when to stop reading from the socket
	//_cursor->contentLength = 0;// 1641845;
	char* contentLengthPtr = strstr(buffer, "Content-Length: ");

	if (contentLengthPtr)
	{
		char* contentLengthStart = contentLengthPtr + 16;
		char* contentLengthEnd = strstr(contentLengthStart, "\r\n");
		assert(contentLengthEnd);

		char cLength[32] = "\0";
		memcpy(cLength, contentLengthStart, contentLengthEnd - contentLengthStart);
		cLength[contentLengthEnd - contentLengthStart] = '\0';

		cursor->contentLength = (unsigned long)atoi((const char*)cLength);
	}
	else assert(1 == 0);
	
	return endOfHeader;
}