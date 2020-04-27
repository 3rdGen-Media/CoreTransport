#include "../CXTransport.h"
//#include "CXReQLCursor.h"

using namespace CoreTransport;


//CXCursor::CXCursor(CTCursor *cursor)
//{
//	_cursor = *cursor;
//}

//This consturctor is not used
CXCursor::CXCursor(CTConnection * conn)
{
	//This will set CTCursor header lenght to 0 for us
	memset(&_cursor, 0, sizeof(CTCursor));
	_cursor.conn = conn;
	//_cursor.headerLength = 0;
	//TO DO:  create a random string name for the file based on the request
	createResponseBuffers("map.jpg\0");
}

CXCursor::CXCursor(CTConnection * conn, const char * filepath)
{
	//This will set CTCursor header lenght to 0 for us
	memset(&_cursor, 0, sizeof(CTCursor));
	_cursor.conn = conn;
	//_cursor.headerLength = 0;
	createResponseBuffers(filepath);
}

void CXCursor::createResponseBuffers(const char * filepath)
{
	// Reserve pages in the virtual address space of the process.	
	//unsigned long pageSize = CTPageSize();

//#ifdef _DEBUG
	//printf("CX_ReQLConnection::reserveConnectionMemory CX_QUERY_BUFF_SIZE = %d\n", (int)CX_RESPONSE_BUFF_SIZE);
	//printf("CX_ReQLConnection::reserveConnectionMemory CX_RESPONSE_BUFF_SIZE = %d\n", (int)CX_RESPONSE_BUFF_SIZE);
//#endif

#ifdef _WIN32
	//recv
	LPVOID queryPtr, responsePtr;
	unsigned long cxBufferSize = ct_system_allocation_granularity(); //+ sizeof(CTOverlappedResponse); 
	
	if( !filepath )
		filepath = "map.jpg\0";
	_cursor.file.path = (char*)filepath;
	
	//(Allocate Response Buffer Memory) First Reserve, then commit
	/*
	responsePtr = VirtualAlloc(NULL, CX_MAX_INFLIGHT_QUERIES * cBufferSize, MEM_RESERVE, PAGE_READWRITE);       // Protection = no access
	_conn.response_buffers = (char*)VirtualAlloc(responsePtr, CX_MAX_INFLIGHT_QUERIES*cBufferSize, MEM_COMMIT, PAGE_READWRITE);       // Protection = no access

	ZeroMemory(_conn.response_buffers, 0, CX_MAX_INFLIGHT_QUERIES * cBufferSize);
	//memset(_conn.response_buffers, 0, CX_MAX_INFLIGHT_QUERIES * CX_RESPONSE_BUFF_SIZE );
	*/

	
	//open file handle
	//HANDLE fh;
	//HANDLE hFile;
	

	//file path to open
	CTCursorCreateMapFileW(&_cursor, _cursor.file.path, ct_system_allocation_granularity()*256L);



	_cursor.overlappedResponse.buf = _cursor.file.buffer;
	_cursor.overlappedResponse.len = cxBufferSize;
	_cursor.overlappedResponse.wsaBuf.buf = _cursor.file.buffer;
	_cursor.overlappedResponse.wsaBuf.len = cxBufferSize;
#endif

#ifdef _DEBUG
	assert(_cursor.file.buffer);
#endif

#ifndef _WIN32
    if (madvise(alembic_archive.filebuffer, (size_t)alembic_archive.size, MADV_SEQUENTIAL | MADV_WILLNEED ) == -1) {
        printf("\nread madvise failed\n");
    }
#endif


}

CXCursor::~CXCursor(void)
{

}
