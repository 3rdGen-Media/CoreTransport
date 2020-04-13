#include "../CXReQL.h"
//#include "CXReQLCursor.h"

using namespace CoreTransport;

CXReQLCursor::CXReQLCursor(ReqlCursor cursor)
{
	_cursor = cursor;
}
CXReQLCursor::~CXReQLCursor(void)
{

}

void CXReQLCursor::continueQuery(void * options)
{
	ReqlCursorContinue(&_cursor, options);
}