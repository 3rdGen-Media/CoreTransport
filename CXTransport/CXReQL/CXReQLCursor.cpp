#include "../CXReQL.h"

using namespace CoreTransport;
char * CXReQLCursor::ProcessResponseHeader(CTCursor * cursor, char * buffer, unsigned long bufferLength)
{

	assert(_cursor == cursor);
	char* endOfHeader = buffer + sizeof(ReqlQueryMessageHeader);

	
	uint64_t recv_token = ((ReqlQueryMessageHeader*)buffer)->token;
	uint64_t cursor_token = _cursor->queryToken;
	//assert(token == _cursor.queryToken);	
	if (recv_token != cursor_token)
	{
		//try to find the cursor we actually need for this response on the current thread's queue
		MSG targetCursorMsg = { 0 };
		if (PeekMessage(&targetCursorMsg, NULL, WM_USER + recv_token, WM_USER + recv_token, PM_REMOVE)) //(nextCursor)
		{
			CTCursor* targetCursor = (CTCursor*)(targetCursorMsg.lParam);
#ifdef _DEBUG
			assert(targetCursorMsg.message == WM_USER + (UINT)recv_token);
			assert(targetCursor);
#endif									

			//std::shared_ptr<CXCursor> cxCursor = connection()->getRequestCursorForKey(cursor_token);
			//std::shared_ptr<CXCursor> cxRecvCursor = connection()->getRequestCursorForKey(recv_token);

			//assert(cxCursor.get() == this);
			//assert(cxCursor->_cursor == _cursor);
			//assert(cxRecvCursor->_cursor == targetCursor);
			//connection()->removeRequestCursorForKey(recv_token);
			//connection()->removeRequestCursorForKey(cursor_token);

			_cursor->queryToken = recv_token;
			targetCursor->queryToken = cursor_token;
			connection()->SwapCursors(cursor_token, recv_token);

			//connection()->addRequestCursorForKey(cxRecvCursor, cursor_token);
			//connection()->addRequestCursorForKey(cxCursor, recv_token);

			
			//targetCursor->headerLength = targetCursor->contentLength = 0;


			//CTCursorHeaderLengthFunc	cursorHeaderLengthCallback = _cursor.headerLengthCallback;
			//CTCursorCompletionFunc		cursorResponseCallback = _cursor.responseCallback;
			//_cursor.headerLengthCallback = targetCursor->headerLengthCallback;
			//targetCursor->headerLengthCallback = cursorHeaderLengthCallback;
			//_cursor.responseCallback = targetCursor->responseCallback;
			//targetCursor->responseCallback = cursorResponseCallback;


			//put the unused cursor associated with the swapped query token back into the thread's queue
			if (PostThreadMessage(GetCurrentThreadId(), WM_USER + cursor_token, 0, (LPARAM)targetCursor) < 1)
			{
				fprintf(stderr, "CXReQLCursor::ProcessResponseHeader::PostThreadMessage failed with error: %d", GetLastError());
				assert(1 == 0);
			}

			fprintf(stderr, "CXReQLCursor::ProcessResponseHeader::Swap Removed (%d) and Reinserted (%d)", recv_token, cursor_token);

		}
		else assert(1 == 0); //could not find the cursor we need on the current thread's queue

	}
	
	//The client *must* set the cursor's contentLength property
	//to aid CoreTransport in knowing when to stop reading from the socket
	_cursor->contentLength = ((ReqlQueryMessageHeader*)buffer)->length;

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