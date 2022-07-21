#include "../CXTransport.h"
//#include "CXReQLCursor.h"

using namespace CoreTransport;

//This consturctor is not used
CXCursor::CXCursor(CXConnection * conn)
{
	_cxConnection = conn;
	//This will set CTCursor header lenght to 0 for us
	_cursor = CTGetNextPoolCursor();
	memset(_cursor, 0, sizeof(CTCursor));
	_cursor->conn = _cxConnection->connection();
	//_cursor.headerLength = 0;
	//TO DO:  create a random string name for the file based on the request
	createResponseBuffers("map.jpg\0");
}

CXCursor::CXCursor(CXConnection * conn, const char * filepath)
{
	_cxConnection = conn;
	//This will set CTCursor header lenght to 0 for us
	_cursor = CTGetNextPoolCursor();
	memset(_cursor, 0, sizeof(CTCursor));
	_cursor->conn = _cxConnection->connection();
	//_cursor.headerLength = 0;
	createResponseBuffers(filepath);
}

CXConnection* CXCursor::connection()
{
	return _cxConnection;
}

void CXCursor::createResponseBuffers(const char * filepath)
{	
	if( !filepath )
		filepath = "map.jpg\0";
	_cursor->file.path = (char*)filepath;
	
	//file path to open
	CTCursorCreateMapFileW(_cursor, _cursor->file.path, ct_system_allocation_granularity() * 256UL);

	memset(_cursor->file.buffer, 0, ct_system_allocation_granularity() * 256UL);

	//_cursor->file.buffer = _cursor->requestBuffer;

#ifdef _DEBUG
	assert(_cursor->file.buffer);
#endif
}

CXCursor::~CXCursor(void)
{

}
